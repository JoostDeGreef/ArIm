#include <fcntl.h>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <vector>

#include "GoogleTestWrapper.h"
#include "SQLiteDB.h"

#define private public
#include "Tokenizer.h"
#undef private

class TokenizerTest : public Test
{
protected:
    std::string GetRandomFilename(const std::string& extension)
    {
        const auto* testInfo = UnitTest::GetInstance()->current_test_info();
        const std::string testName = std::string(testInfo->test_suite_name()) + "_" + testInfo->name();
        for(unsigned attempt = 0; attempt < 100000; ++attempt)
        {
            std::ostringstream filename;
            filename << "temp/" << testName << '.' << std::setw(5) << std::setfill('0') << attempt << '.' << extension;
            const std::string path = filename.str();
            const int fd = open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
            if(fd >= 0)
            {
                close(fd);
                return path;
            }
        }
        throw std::runtime_error("Unable to find an unused temporary filename");
    }
};

TEST_F(TokenizerTest, Constructor)
{
    Tokenizer tokenizer;
    EXPECT_TRUE(tokenizer.m_tokenizedInputSequence.Empty()) << "Expected empty tokenizedInputSequence on construction";
    EXPECT_EQ(512,tokenizer.m_nextUnusedToken.m_data) << "Expected 512 reserved tokens for the ASCII characters + separation";
}

TEST_F(TokenizerTest, FeedInput)
{
    Tokenizer tokenizer;
    tokenizer.FeedInput("this is a test");
    EXPECT_EQ(14-3, std::distance(tokenizer.m_tokenizedInputSequence.begin(), tokenizer.m_tokenizedInputSequence.end())) << "Expected 14 characters minus 3 spaces";
    EXPECT_EQ(512, tokenizer.m_nextUnusedToken.m_data) << "Expected 512 tokens.";
    auto position = tokenizer.m_tokenizedInputSequence.begin();
    EXPECT_EQ('t', (position++)->GetToken());
    std::advance(position, 3);
    EXPECT_EQ(Token('i'+256), *position);
}

TEST_F(TokenizerTest, UpdateTokenPairCountAndPositions)
{
    Tokenizer tokenizer;
    tokenizer.FeedInput("ababa");
    EXPECT_EQ(5, std::distance(tokenizer.m_tokenizedInputSequence.begin(), tokenizer.m_tokenizedInputSequence.end())) << "Expected 5 characters";
    EXPECT_EQ(512, tokenizer.m_nextUnusedToken.m_data) << "Expected 512 tokens.";
    tokenizer.UpdateTokenPairCountAndPositions(tokenizer.m_tokenizedInputSequence.begin());
    EXPECT_EQ(2, tokenizer.m_tokenPairCountAndPositions.size()) << "Expected the two pairs ab and ba";
    EXPECT_EQ(2, tokenizer.m_tokenPairCountAndPositions.find(TokenPair('b','a'))->second.count) << "Expected 2 occurrences of the token pair ba";
    EXPECT_EQ(2, tokenizer.m_tokenPairCountAndPositions.find(TokenPair('a','b'))->second.count) << "Expected 2 occurrences of the token pair ab";
}

TEST_F(TokenizerTest, ApplyExistingTokenPairReplacements)
{
    Tokenizer tokenizer;
    tokenizer.FeedInput("ababa");
    EXPECT_EQ(5, std::distance(tokenizer.m_tokenizedInputSequence.begin(), tokenizer.m_tokenizedInputSequence.end())) << "Expected 5 characters";
    EXPECT_EQ(512, tokenizer.m_nextUnusedToken.m_data) << "Expected 512 tokens.";
    tokenizer.UpdateTokenPairCountAndPositions(tokenizer.m_tokenizedInputSequence.begin());
    EXPECT_EQ(2, tokenizer.m_tokenPairCountAndPositions.size()) << "Expected the two pairs ab and ba";
    tokenizer.m_tokenPairReplacements[TokenPair('a','b')] = Token('Y');
    tokenizer.m_tokenPairReplacements[TokenPair('b','a')] = Token('Z');
    tokenizer.ApplyExistingTokenPairReplacements();
    ASSERT_EQ(3, std::distance(tokenizer.m_tokenizedInputSequence.begin(), tokenizer.m_tokenizedInputSequence.end())) << "Expected 3 tokens, YYa";
    EXPECT_EQ(512, tokenizer.m_nextUnusedToken.m_data) << "Expected 512 tokens. XY are not new";
    auto p = tokenizer.m_tokenizedInputSequence.begin();
    EXPECT_EQ('Y', (p++)->GetToken()) << "Expected Y";
    EXPECT_EQ('Y', (p++)->GetToken()) << "Expected Y";
    EXPECT_EQ('a', (p++)->GetToken()) << "Expected a";
}

TEST_F(TokenizerTest, OptimizeTokenLengthMergesMostFrequentPair)
{
    Tokenizer tokenizer;
    tokenizer.FeedInput("xababx");
    tokenizer.UpdateTokenPairCountAndPositions(tokenizer.m_tokenizedInputSequence.begin());

    tokenizer.OptimizeTokenLength();

    const TokenPair mostFrequentPair(Token('a'), Token('b'));
    ASSERT_NE(tokenizer.m_tokenPairReplacements.end(), tokenizer.m_tokenPairReplacements.find(mostFrequentPair));
    EXPECT_EQ(512, tokenizer.m_tokenPairReplacements.at(mostFrequentPair).GetToken());
    EXPECT_LT(std::distance(tokenizer.m_tokenizedInputSequence.begin(), tokenizer.m_tokenizedInputSequence.end()), 6);
}

TEST_F(TokenizerTest, CalculateTokenRelationshipsCountsPrecedingTokensByDepth)
{
    Tokenizer tokenizer;
    tokenizer.FeedInput("ababa");

    tokenizer.CalculateTokenRelationships();

    const Token a('a');
    const Token b('b');
    ASSERT_EQ(2, tokenizer.m_tokenRelationships.size());

    const auto& bRelationships = tokenizer.m_tokenRelationships.at(b);
    EXPECT_EQ(2, bRelationships[0].at(a));
    EXPECT_EQ(1, bRelationships[1].at(b));

    const auto& aRelationships = tokenizer.m_tokenRelationships.at(a);
    EXPECT_EQ(2, aRelationships[0].at(b));
    EXPECT_EQ(2, aRelationships[1].at(a));
}

TEST_F(TokenizerTest, StoreAndLoadRoundTripPreservesInput)
{
    const std::string input = "The tokenizer stores and reloads this input.";
    Tokenizer original;
    original.Process(input);

    auto getInputText = [](const Tokenizer& tokenizer)
    {
        std::string text;
        for(auto position = tokenizer.m_tokenizedInputSequence.begin();
            position != tokenizer.m_tokenizedInputSequence.end(); ++position)
        {
            text += tokenizer.GetTokenTextSequence(*position);
        }
        return text;
    };

    SQLite::DB db;
    db.Open(SQLite::DB::MemoryDB, false);
    original.StoreTokenizerToDB(db);

    Tokenizer loaded;
    loaded.LoadDataFromDB(db);

    EXPECT_EQ(input, getInputText(original));
    EXPECT_EQ(getInputText(original), getInputText(loaded));
    EXPECT_EQ(original.m_tokenPairReplacements, loaded.m_tokenPairReplacements);
    EXPECT_EQ(original.m_tokenReplacements, loaded.m_tokenReplacements);
    EXPECT_EQ(original.m_tokenRelationships, loaded.m_tokenRelationships);
    EXPECT_EQ(original.m_nextUnusedToken, loaded.m_nextUnusedToken);
    size_t originalActivePairCount = 0;
    size_t loadedActivePairCount = 0;
    for(const auto& [tokenPair, countAndPosition] : original.m_tokenPairCountAndPositions)
    {
        if(countAndPosition.count == 0)
        {
            continue;
        }
        ++originalActivePairCount;
        const auto loadedCountAndPosition = loaded.m_tokenPairCountAndPositions.find(tokenPair);
        ASSERT_NE(loaded.m_tokenPairCountAndPositions.end(), loadedCountAndPosition);
        EXPECT_EQ(countAndPosition.count, loadedCountAndPosition->second.count);
        EXPECT_EQ(countAndPosition.positions.size(), loadedCountAndPosition->second.positions.size());
    }
    for(const auto& [tokenPair, countAndPosition] : loaded.m_tokenPairCountAndPositions)
    {
        (void)tokenPair;
        if(countAndPosition.count != 0) ++loadedActivePairCount;
    }
    EXPECT_EQ(originalActivePairCount, loadedActivePairCount);
}

TEST_F(TokenizerTest, StoreAndLoadReadsBlobChunksAcrossRecordBoundaries)
{
    std::string input;
    input.reserve(4096);
    for(size_t i = 0; i < 4096; ++i)
    {
        input.push_back(static_cast<char>('a' + (i * 17) % 26));
    }

    Tokenizer original;
    original.FeedInput(input);
    original.UpdateTokenPairCountAndPositions(original.m_tokenizedInputSequence.begin());
    original.CalculateTokenRelationships();
    // Keep an entry with no connected-token values to check that empty depth
    // maps survive the grouped relationship serialization too.
    original.m_tokenRelationships.emplace(Token(900), decltype(original.m_tokenRelationships)::mapped_type{});
    for(Token::Data value = 256; value < 456; ++value)
    {
        const TokenPair tokenPair(Token('a'), Token(value));
        const Token replacement(static_cast<Token::Data>(value + 256));
        original.m_tokenPairReplacements.emplace(tokenPair, replacement);
        original.m_tokenReplacements.emplace(replacement, tokenPair);
    }
    original.m_nextUnusedToken.SetToken(712);

    SQLite::DB db;
    db.Open(SQLite::DB::MemoryDB, false);
    original.StoreTokenizerToDB(db);
    EXPECT_GT(db.ExecSingleInt64("SELECT SUM(length(Data)) FROM TokenizedInputSequence;"), 1024);
    EXPECT_GT(db.ExecSingleInt64("SELECT SUM(length(Data)) FROM TokenPairReplacements;"), 1024);
    EXPECT_GT(db.ExecSingleInt64("SELECT SUM(length(Data)) FROM TokenRelations;"), 1024);

    size_t expectedRelationshipBytes = 0;
    for(const auto& [token, relationsByDepth] : original.m_tokenRelationships)
    {
        (void)token;
        expectedRelationshipBytes += sizeof(Token::Data) +
            relationsByDepth.size() * sizeof(size_t);
        for(const auto& relations : relationsByDepth)
        {
            expectedRelationshipBytes += relations.size() * (sizeof(Token::Data) + sizeof(size_t));
        }
    }
    EXPECT_EQ(expectedRelationshipBytes,
        db.ExecSingleInt64("SELECT SUM(length(Data)) FROM TokenRelations;"));

    Tokenizer loaded;
    loaded.LoadDataFromDB(db);

    auto getInputText = [](const Tokenizer& tokenizer)
    {
        std::string text;
        for(auto position = tokenizer.m_tokenizedInputSequence.begin();
            position != tokenizer.m_tokenizedInputSequence.end(); ++position)
        {
            text += tokenizer.GetTokenTextSequence(*position);
        }
        return text;
    };
    EXPECT_EQ(input, getInputText(loaded));
    EXPECT_EQ(original.m_tokenPairReplacements, loaded.m_tokenPairReplacements);
    EXPECT_EQ(original.m_tokenReplacements, loaded.m_tokenReplacements);
    EXPECT_EQ(original.m_tokenRelationships, loaded.m_tokenRelationships);
    ASSERT_EQ(original.m_tokenPairCountAndPositions.size(), loaded.m_tokenPairCountAndPositions.size());
}

TEST_F(TokenizerTest, StoreAndLoadHandlesEmptyTokenizer)
{
    SQLite::DB db;
    db.Open(SQLite::DB::MemoryDB, false);

    Tokenizer original;
    original.StoreTokenizerToDB(db);

    Tokenizer loaded;
    EXPECT_NO_THROW(loaded.LoadDataFromDB(db));
    EXPECT_TRUE(loaded.m_tokenizedInputSequence.Empty());
    EXPECT_TRUE(loaded.m_tokenPairReplacements.empty());
    EXPECT_TRUE(loaded.m_tokenRelationships.empty());
    EXPECT_EQ(original.m_nextUnusedToken, loaded.m_nextUnusedToken);
}

TEST_F(TokenizerTest, LoadRejectsIncompatibleConfiguration)
{
    SQLite::DB db;
    db.Open(SQLite::DB::MemoryDB, false);

    Tokenizer original;
    original.FeedInput("configuration check");
    original.StoreTokenizerToDB(db);
    db.ExecDML("UPDATE Settings SET Value = Value + 1 WHERE Setting = 'RelationDepth';");

    Tokenizer loaded;
    EXPECT_THROW(loaded.LoadDataFromDB(db), std::runtime_error);
}

// TEST_F(TokenizerTest, FeedInputStoresCharactersInOneSequence)
// {
//     Tokenizer tokenizer;
//     tokenizer.FeedInput("this is a test");

//     EXPECT_EQ(8, tokenizer.GetUsedTokensCount());
//     EXPECT_EQ(11, tokenizer.sequence.size());
//     EXPECT_EQ('t'+Token::separated, *tokenizer.sequence.begin());
//     auto firstAfterSpace = tokenizer.sequence.begin();
//     for(int i = 0; i < 4; ++i)
//     {
//         ++firstAfterSpace;
//     }
//     EXPECT_NE(0, *firstAfterSpace & Token::separated);

//     tokenizer.FeedInput("!");
//     EXPECT_EQ(12, tokenizer.sequence.size());
//     EXPECT_EQ('!'+Token::separated, *tokenizer.sequence.rbegin());
// }

// TEST_F(TokenizerTest, CreateTopTokenPairsForSequence)
// {
//     Tokenizer tokenizer;
//     tokenizer.FeedInput("ababab");
//     EXPECT_EQ(3, tokenizer.GetUsedTokensCount()); // start+a, b, a

//     const auto replacements = tokenizer.CreateTopTokenPairsForSequence(1);

//     ASSERT_EQ(2, replacements.size());
//     EXPECT_EQ('a', replacements[0].left);
//     EXPECT_EQ('b', replacements[0].right);
//     EXPECT_EQ(replacements[0].replacement, replacements[1].replacement);
//     auto occurrence0 = replacements[0].sequenceIndex;
//     EXPECT_EQ('a', *occurrence0++);
//     EXPECT_EQ('b', *occurrence0);
//     auto occurrence1 = replacements[1].sequenceIndex;
//     EXPECT_EQ('a', *occurrence1++);
//     EXPECT_EQ('b', *occurrence1);
// }

// TEST_F(TokenizerTest, MergeTopTokenPairsUpdatesSequenceAndCounts)
// {
//     Tokenizer tokenizer;
//     tokenizer.FeedInput("qbabax");

//     auto replacements = tokenizer.CreateTopTokenPairsForSequence(1);
//     ASSERT_EQ(2, replacements.size());
//     tokenizer.MergeTopTokenPairs(replacements);

//     EXPECT_EQ((std::vector<Token::Data>{Token::Data('q') | Token::separated, replacements[0].replacement, replacements[0].replacement, 'x'}), Values(tokenizer.sequence));
//     EXPECT_EQ(0, tokenizer.tokens.find('a')->count);
//     EXPECT_EQ(0, tokenizer.tokens.find('b')->count);
//     EXPECT_EQ(2, tokenizer.tokens.find(replacements[0].replacement)->count);
// }

// TEST_F(TokenizerTest, OptimizeTokenLengthMergesRepeatedPair)
// {
//     Tokenizer tokenizer;
//     tokenizer.FeedInput("abab");
//     EXPECT_EQ(3, tokenizer.GetUsedTokensCount());
//     EXPECT_EQ(4, tokenizer.sequence.size());

//     tokenizer.OptimizeTokenLength(4, 1);

//     EXPECT_EQ(1, tokenizer.GetUsedTokensCount());
//     ASSERT_EQ(1, tokenizer.sequence.size());
//     EXPECT_EQ("abab", tokenizer.GetTokenTextSequence(*tokenizer.tokens.find(*tokenizer.sequence.begin())));
// }

// TEST_F(TokenizerTest, CalculateTokenRelationships)
// {
//     Tokenizer tokenizer;
//     tokenizer.FeedInput("abab");

//     tokenizer.CalculateTokenRelationships(2);

//     EXPECT_EQ(3, tokenizer.tokenRelations.size());
//     EXPECT_EQ(1, tokenizer.tokenRelations.at('a')[0][0].count);
//     EXPECT_EQ('b', tokenizer.tokenRelations.at('a')[0][0].token);
// }

// TEST_F(TokenizerTest, DBStoreLoad)
// {
//     const std::string filename = GetRandomFilename("sq3");
//     Tokenizer original;
//     original.FeedInput("This is a test and more data, even more data. much more data");
//     original.OptimizeTokenLength(1024, 1);
//     original.CalculateTokenRelationships();

//     SQLite::DB db;
//     db.Open(filename, false);
//     db.ExecDML("PRAGMA journal_mode = OFF;");
//     db.ExecDML("PRAGMA synchronous = OFF;");
//     db.ExecDML("PRAGMA temp_store = MEMORY;");
//     original.StoreCalculatedDataToDB(db);

//     Tokenizer loaded(db);
//     db.Close();

//     EXPECT_EQ(original.tokens.size(), loaded.tokens.size());
//     EXPECT_EQ(original.tokenRelations, loaded.tokenRelations);
//     EXPECT_EQ(original.unusedTokensCount, loaded.unusedTokensCount);
//     EXPECT_EQ(Values(original.sequence), Values(loaded.sequence));
//     EXPECT_EQ(original.tokens, loaded.tokens);
// }
