#include <fcntl.h>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <vector>

#include "googletest_wrapper.h"
#include "SQLiteDB.h"

#define private public
#include "tokenizer.h"
#undef private

class TokenizerTest : public Test
{
protected:
    // static std::vector<Token::Data> Values(const TokenSequence& sequence)
    // {
    //     return std::vector<Token::Data>(sequence.begin(), sequence.end());
    // }

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
    EXPECT_TRUE(tokenizer.tokenizedInputSequence.empty()) << "Expected empty tokenizedInputSequence on construction";
    EXPECT_EQ(256,tokenizer.nextUnusedToken.token) << "Expected 256 reserved tokens for the ASCII characters";
}

TEST_F(TokenizerTest, FeedInput)
{
    Tokenizer tokenizer;
    tokenizer.FeedInput("this is a test");
    EXPECT_EQ(14-3, std::distance(tokenizer.tokenizedInputSequence.begin(), tokenizer.tokenizedInputSequence.end())) << "Expected 14 characters minus 3 spaces";
    EXPECT_EQ(256, tokenizer.nextUnusedToken.token) << "Expected 256 tokens. separation/start is only a state";
}

TEST_F(TokenizerTest, UpdateTokenPairCountAndPositions)
{
    Tokenizer tokenizer;
    tokenizer.FeedInput("ababa");
    EXPECT_EQ(5, std::distance(tokenizer.tokenizedInputSequence.begin(), tokenizer.tokenizedInputSequence.end())) << "Expected 5 characters";
    EXPECT_EQ(256, tokenizer.nextUnusedToken.token) << "Expected 256 tokens. separation/start is only a state";
    tokenizer.UpdateTokenPairCountAndPositions(tokenizer.tokenizedInputSequence.begin());
    EXPECT_EQ(3, tokenizer.tokenPairCountAndPositions.size()) << "Expected 3 token pairs: *ab, ba and ab";
    EXPECT_EQ(1, tokenizer.tokenPairCountAndPositions.find(TokenPair('a' | Token::separated,'b'))->second.count) << "Expected 1 occurrence of the token pair ab";
    EXPECT_EQ(2, tokenizer.tokenPairCountAndPositions.find(TokenPair('b','a'))->second.count) << "Expected 2 occurrences of the token pair ba";
    EXPECT_EQ(1, tokenizer.tokenPairCountAndPositions.find(TokenPair('a','b'))->second.count) << "Expected 1 occurrence of the token pair ab";
}

TEST_F(TokenizerTest, ApplyExistingTokenPairReplacements)
{
    Tokenizer tokenizer;
    tokenizer.FeedInput("ababa");
    EXPECT_EQ(5, std::distance(tokenizer.tokenizedInputSequence.begin(), tokenizer.tokenizedInputSequence.end())) << "Expected 5 characters";
    EXPECT_EQ(256, tokenizer.nextUnusedToken.token) << "Expected 256 tokens. separation is only a state";
    tokenizer.UpdateTokenPairCountAndPositions(tokenizer.tokenizedInputSequence.begin());
    EXPECT_EQ(3, tokenizer.tokenPairCountAndPositions.size()) << "Expected 3 token pairs: *ab, ba and ab";
    tokenizer.tokenPairReplacements[TokenPair('a' | Token::separated,'b')] = Token('X');
    tokenizer.tokenPairReplacements[TokenPair('a','b')] = Token('Y');
    tokenizer.tokenPairReplacements[TokenPair('b','a')] = Token('Z');
    tokenizer.ApplyExistingTokenPairReplacements();
    ASSERT_EQ(3, std::distance(tokenizer.tokenizedInputSequence.begin(), tokenizer.tokenizedInputSequence.end())) << "Expected 3 tokens, XYa";
    EXPECT_EQ(256, tokenizer.nextUnusedToken.token) << "Expected 256 tokens. XY are not new";
    auto p = tokenizer.tokenizedInputSequence.begin();
    EXPECT_EQ('X', (p++)->token) << "Expected X";
    EXPECT_EQ('Y', (p++)->token) << "Expected Y";
    EXPECT_EQ('a', (p++)->token) << "Expected a";
}

TEST_F(TokenizerTest, OptimizeTokenLengthMergesMostFrequentPair)
{
    Tokenizer tokenizer;
    tokenizer.FeedInput("xababx");
    tokenizer.UpdateTokenPairCountAndPositions(tokenizer.tokenizedInputSequence.begin());

    tokenizer.OptimizeTokenLength(257);

    ASSERT_EQ(4, std::distance(tokenizer.tokenizedInputSequence.begin(), tokenizer.tokenizedInputSequence.end()));
    auto position = tokenizer.tokenizedInputSequence.begin();
    EXPECT_EQ('x' | Token::separated, (position++)->token);
    const auto replacement = (position++)->token;
    EXPECT_EQ(256, replacement);
    EXPECT_EQ(replacement, (position++)->token);
    EXPECT_EQ('x', position->token);
    EXPECT_EQ(1, tokenizer.tokenPairReplacements.size());
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
