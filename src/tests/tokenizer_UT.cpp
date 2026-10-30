#include <algorithm>
#include <deque>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <unistd.h>

#include "googletest_wrapper.h"

#include "SQLiteDB.h"

#define private public
#include "tokenizer.h"

class TokenizerTest : public Test 
{
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown() 
    {
    }

    std::string GetRandomFilename(std::string extension)
    {
        const auto *testInfo = UnitTest::GetInstance()->current_test_info();
        const std::string testName = std::string(testInfo->test_suite_name()) + "_" + testInfo->name();
        for (unsigned attempt = 0; attempt < 100000; ++attempt)
        {
            const unsigned sequence = attempt;
            std::ostringstream filename;
            filename << "temp/" << testName
                     << '.' << std::setw(5) << std::setfill('0') << sequence
                     << "." << extension;
            const std::string path = filename.str();
            const int fd = open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
            if (fd >= 0)
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
    Tokenizer t;
    ASSERT_EQ(256,t.tokens.size());
    ASSERT_EQ(256,t.unusedTokensCount);
}

TEST_F(TokenizerTest, GetUsedTokensCount)
{
    Tokenizer t;
    ASSERT_EQ(0,t.GetUsedTokensCount());
}

TEST_F(TokenizerTest, FeedInput)
{
    Tokenizer t;
    t.FeedInput("this is a test");
    EXPECT_EQ(7, t.GetUsedTokensCount());
    EXPECT_EQ(7, t.sequences.size());
    t.FeedInput("this is also a test");
    EXPECT_EQ(9, t.GetUsedTokensCount());
    EXPECT_EQ(16, t.sequences.size());
}

TEST_F(TokenizerTest, CreateTokenPairsForSequences)
{
    Tokenizer t;
    auto res = t.CreateTokenPairsForSequences();
    EXPECT_EQ(0, res.size());
    t.sequences.push_back({0,0,0,0,1});
    t.sequences.push_back({0,1}); 
    res = t.CreateTokenPairsForSequences();
    EXPECT_EQ(2, res.size());
    EXPECT_EQ(3, res[TokenPair(-1,0,0)].size());
    EXPECT_EQ(2, res[TokenPair(-1,0,1)].size());
    Tokenizer t2;
    t2.FeedInput("this is a test");
    EXPECT_EQ(7, t2.GetUsedTokensCount()); // 7 uniq characters used
    EXPECT_EQ(7, t2.sequences.size());     // 7 sequences inserted
    res = t2.CreateTokenPairsForSequences();
    EXPECT_EQ(6, res.size());              // 6 pairs provided as input
}

TEST_F(TokenizerTest, MergeTopTokenPairsUpdatesCountsAndNeighborPairs)
{
    Tokenizer t;
    t.sequences = {{'x', 'a', 'b', 'x', 'a', 'b', 'x'}};
    for (const auto token : t.sequences.front())
    {
        ++t.tokens.find(token)->count;
    }

    TokenPairs tokenPairs = t.CreateTokenPairsForSequences();
    const auto pairToMergeIter = tokenPairs.find(TokenPair(-1, 'a', 'b'));
    ASSERT_NE(tokenPairs.end(), pairToMergeIter);
    const TokenPair pairToMerge = pairToMergeIter->first;
    const auto mergedToken = pairToMerge.token;

    t.MergeTopTokenPairs({pairToMerge}, tokenPairs);

    EXPECT_EQ((TokenSequence{'x', mergedToken, 'x', mergedToken, 'x'}), t.sequences[0]);
    EXPECT_EQ(3, t.tokens.find('x')->count);
    EXPECT_EQ(0, t.tokens.find('a')->count);
    EXPECT_EQ(0, t.tokens.find('b')->count);
    EXPECT_EQ(2, t.tokens.find(mergedToken)->count);

    EXPECT_EQ(2, tokenPairs.at(TokenPair(-1, 'x', mergedToken)).size());
    EXPECT_EQ(2, tokenPairs.at(TokenPair(-1, mergedToken, 'x')).size());
    EXPECT_EQ(tokenPairs.end(), tokenPairs.find(TokenPair(-1, 'x', 'a')));
    EXPECT_EQ(tokenPairs.end(), tokenPairs.find(TokenPair(-1, 'a', 'b')));
    EXPECT_EQ(tokenPairs.end(), tokenPairs.find(TokenPair(-1, 'b', 'x')));
}

TEST_F(TokenizerTest, GetTopTokenPairs)
{
    Tokenizer t;
    TokenPairs tokenPairs;
    auto res = t.GetTopTokenPairs(2, tokenPairs);
    EXPECT_TRUE(res.empty());

    tokenPairs[TokenPair(-1,0,0)] = {0,0};
    tokenPairs[TokenPair(-1,0,1)] = {0};
    tokenPairs[TokenPair(-1,0,2)] = {0,1,2,3};
    res = t.GetTopTokenPairs(2, tokenPairs);
    ASSERT_EQ(2, res.size());
    EXPECT_EQ(TokenPair(-1,0,2), res[0]);
    EXPECT_EQ(TokenPair(-1,0,0), res[1]);

    tokenPairs[TokenPair(-1,0,3)] = {0,1,2,3};
    tokenPairs[TokenPair(-1,1,0)] = {0,1,2};
    res = t.GetTopTokenPairs(3, tokenPairs);
    ASSERT_EQ(3, res.size());
    EXPECT_EQ(TokenPair(-1,0,3), res[0]);
    EXPECT_EQ(TokenPair(-1,0,2), res[1]);
    EXPECT_EQ(TokenPair(-1,1,0), res[2]);
    EXPECT_EQ(tokenPairs.size(), t.GetTopTokenPairs(tokenPairs.size() + 1, tokenPairs).size());
}

TEST_F(TokenizerTest, CalculateTokenRelationships)
{
    Tokenizer t;
    t.sequences = {{'a', 'b', 'a', 'c', 'a'}};
    for (const auto token : t.sequences.front())
    {
        ++t.tokens.find(token)->count;
    }

    t.CalculateTokenRelationships(2);

    ASSERT_EQ(3, t.tokenRelations.size());
    for (const auto token : {'a', 'b', 'c'})
    {
        ASSERT_TRUE(t.tokenRelations.count(token));
        ASSERT_EQ(2, t.tokenRelations.at(token).size());
    }

    const auto relationCount = [&t](char current, size_t depth, char previous)
    {
        const auto & relations = t.tokenRelations.at(current)[depth];
        const auto relation = std::find_if(relations.begin(), relations.end(), [previous](const TokenCount & count)
        {
            return count.token == previous;
        });
        return relation == relations.end() ? 0 : relation->count;
    };
    EXPECT_EQ(1, relationCount('a', 0, 'b'));
    EXPECT_EQ(1, relationCount('a', 0, 'c'));
    EXPECT_EQ(2, relationCount('a', 1, 'a'));

    EXPECT_EQ(1, relationCount('b', 0, 'a'));
    EXPECT_EQ(1, relationCount('c', 0, 'a'));
    EXPECT_EQ(1, relationCount('c', 1, 'b'));
}

TEST_F(TokenizerTest, OptimizeTokenLength)
{
    Tokenizer t;
    t.FeedInput("this is a test");
    EXPECT_EQ(7, t.GetUsedTokensCount()); // 7 uniq characters used
    EXPECT_EQ(7, t.sequences.size());
    t.OptimizeTokenLength(100,1); 
    EXPECT_EQ(5, t.GetUsedTokensCount());
}

TEST_F(TokenizerTest, GetTokenTextSequence)
{
    Tokenizer t;
    t.tokens.emplace(Token('a'));
    t.tokens.emplace(Token('b'));
    t.tokens.emplace(Token('c'));
    t.tokens.emplace(Token(256, 'a', 'b'));
    t.tokens.emplace(Token(257, 256, 'c'));

    EXPECT_EQ("a", t.GetTokenTextSequence('a'));
    EXPECT_EQ("ab", t.GetTokenTextSequence(256));
    EXPECT_EQ("abc", t.GetTokenTextSequence(257));
}

TEST_F(TokenizerTest, OptimizeTokenLengthMergesRepeatedPair)
{
    Tokenizer t;
    t.FeedInput("abab");

    EXPECT_EQ(2, t.GetUsedTokensCount());
    EXPECT_EQ(1, t.sequences.size());
    EXPECT_EQ(4, t.sequences.front().size());

    t.OptimizeTokenLength(3, 1);

    // "ab" occurs twice, so it should be merged into one token.
    EXPECT_EQ(1, t.GetUsedTokensCount());
    EXPECT_EQ(1, t.sequences.size());
    EXPECT_EQ(1, t.sequences.front().size());

    const auto& sequence = t.sequences.front();

    // The merged token must expand back to the original text.
    EXPECT_EQ("abab", t.GetTokenTextSequence(sequence[0]));
}

TEST_F(TokenizerTest, FullFunctionalityTest)
{
    std::string randomFilename = GetRandomFilename("sq3");
    Tokenizer t;
    t.FeedInput("This is a test");
    EXPECT_EQ(7, t.sequences.size());
    EXPECT_EQ(8, t.GetUsedTokensCount());

    t.OptimizeTokenLength(1024,1);
    EXPECT_EQ(5, t.GetUsedTokensCount());
    for (const auto& sequence : t.sequences)
    {
        ASSERT_FALSE(sequence.empty());
        for (const auto token : sequence)
        {
            ASSERT_TRUE(t.tokens.count(token));
        }
    }

    t.CalculateTokenRelationships();
    ASSERT_FALSE(t.tokenRelations.empty());
    EXPECT_EQ(5, t.tokenRelations.size());
    decltype(t.tokenRelations) expectedTokenRelations;
    for (const auto& token : t.tokens)
    {
        if (token.count)
        {
            expectedTokenRelations.emplace(token.token, std::vector<TokenCounts>(5));
        }
    }
    std::deque<Token::Data> pastTokens;
    for (const auto& sequence : t.sequences)
    {
        for (const auto token : sequence)
        {
            auto& relations = expectedTokenRelations[token];
            pastTokens.emplace_front(token);
            if (pastTokens.size() > 6)
            {
                pastTokens.pop_back();
            }
            for (size_t depth = 1; depth < pastTokens.size(); ++depth)
            {
                auto & counts = relations[depth - 1];
                auto relation = std::find_if(counts.begin(), counts.end(), [token = pastTokens[depth]](const TokenCount & count)
                {
                    return count.token == token;
                });
                if (relation == counts.end())
                {
                    counts.push_back(TokenCount{pastTokens[depth], 1});
                }
                else
                {
                    ++relation->count;
                }
            }
        }
    }
    EXPECT_EQ(expectedTokenRelations, t.tokenRelations);

    SQLite::DB db;
    db.Open(randomFilename, false);

    db.ExecDML("PRAGMA journal_mode = OFF;");
    db.ExecDML("PRAGMA synchronous = OFF;");
    db.ExecDML("PRAGMA temp_store = MEMORY;");
    t.StoreCalculatedDataToDB(db);

    EXPECT_EQ(t.tokens.size(), db.ExecSingleInt64("SELECT COUNT(*) FROM Tokens;"));
    EXPECT_EQ(5, db.ExecSingleInt64("SELECT COUNT(*) FROM (SELECT Token FROM TokenRelations WHERE Depth = 0 GROUP BY Token);"));
}

TEST_F(TokenizerTest, DBStoreLoad)
{
    std::string randomFilename = GetRandomFilename("sq3");
    MESSAGE() << "Test DB: " << randomFilename;
    Tokenizer t0;
    t0.FeedInput("This is a test and more data, even more data. much more data");
    t0.OptimizeTokenLength(1024,1);
    t0.CalculateTokenRelationships();
    EXPECT_EQ(12, t0.tokenRelations.size());

    SQLite::DB db;
    db.Open(randomFilename, false);

    db.ExecDML("PRAGMA journal_mode = OFF;");
    db.ExecDML("PRAGMA synchronous = OFF;");
    db.ExecDML("PRAGMA temp_store = MEMORY;");
    t0.StoreCalculatedDataToDB(db);

    Tokenizer t1(db);    
    db.Close();
    EXPECT_EQ(t0.tokens.size(), t1.tokens.size());
    EXPECT_EQ(t0.tokenRelations.size(), t1.tokenRelations.size());
    EXPECT_EQ(t0.unusedTokensCount, t1.unusedTokensCount);
    EXPECT_EQ(t0.sequences.size(), t1.sequences.size());
    EXPECT_TRUE(t0.tokens==t1.tokens);
    EXPECT_TRUE(t0.tokenRelations==t1.tokenRelations);
    EXPECT_TRUE(t0.sequences==t1.sequences);
}
 
