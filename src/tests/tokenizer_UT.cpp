#include <gtest/gtest.h>
#include <atomic>
#include <deque>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <unistd.h>
using namespace testing;

#define private public

#include "tokenizer.h"
#include "SQLiteDB.h"

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
        static std::atomic<unsigned> sequence{0};
        for (unsigned attempt = 0; attempt < 100000; ++attempt)
        {
            std::ostringstream filename;
            filename << "/tmp/" << UnitTest::GetInstance()->current_test_info()->test_suite_name()
                     << "_" << UnitTest::GetInstance()->current_test_info()->test_case_name()
                     << '.' << std::setw(5) << std::setfill('0') << (sequence++ % 100000)
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

TEST_F(TokenizerTest, CreateTokenPairsForSequence)
{
    Tokenizer t;
    std::vector<Token::Data> sequence;
    TokenPairSet res = t.CreateTokenPairsForSequence(sequence);
    EXPECT_TRUE(res.empty());
    sequence.push_back(0);
    res = t.CreateTokenPairsForSequence(sequence);
    EXPECT_TRUE(res.empty());
    sequence.push_back(0);
    res = t.CreateTokenPairsForSequence(sequence);
    EXPECT_EQ(1, res.size());
    EXPECT_EQ(1, res.begin()->count);
    sequence.push_back(1);
    res = t.CreateTokenPairsForSequence(sequence);
    EXPECT_EQ(2, res.size());
    ASSERT_EQ(1, res.count(TokenPair(0,0)));
    EXPECT_EQ(1, res.find(TokenPair(0,0))->count);
    ASSERT_EQ(1, res.count(TokenPair(0,1)));
    EXPECT_EQ(1, res.find(TokenPair(0,1))->count);
    sequence.insert(sequence.begin(),0);
    res = t.CreateTokenPairsForSequence(sequence);
    EXPECT_EQ(2, res.size());
    ASSERT_EQ(1, res.count(TokenPair(0,0)));
    EXPECT_EQ(2, res.find(TokenPair(0,0))->count);
    ASSERT_EQ(1, res.count(TokenPair(0,1)));
    EXPECT_EQ(1, res.find(TokenPair(0,1))->count);
}

TEST_F(TokenizerTest, CreateTokenPairsForSequences)
{
    Tokenizer t;
    std::vector<std::vector<Token::Data>> sequences;
    auto res = t.CreateTokenPairsForSequences(sequences);
    EXPECT_EQ(sequences.size(), res.size());
    sequences.push_back({0,0,0,1});
    sequences.push_back({0,1});
    res = t.CreateTokenPairsForSequences(sequences);
    EXPECT_EQ(sequences.size(), res.size());
}

TEST_F(TokenizerTest, GetTopTokenPairs)
{
    Tokenizer t;
    TokenPairSet tokenPairs;
    auto res = t.GetTopTokenPairs(2, tokenPairs);
    EXPECT_TRUE(res.empty());

    tokenPairs.insert(TokenPair(0,0,2));
    tokenPairs.insert(TokenPair(0,1,1));
    tokenPairs.insert(TokenPair(0,2,4));
    res = t.GetTopTokenPairs(2, tokenPairs);
    ASSERT_EQ(2, res.size());
    EXPECT_EQ(TokenPair(0,2,4), res[0]);
    EXPECT_EQ(TokenPair(0,0,2), res[1]);

    tokenPairs.insert(TokenPair(0,3,4));
    tokenPairs.insert(TokenPair(1,0,3));
    res = t.GetTopTokenPairs(3, tokenPairs);
    ASSERT_EQ(3, res.size());
    EXPECT_EQ(TokenPair(0,3,4), res[0]);
    EXPECT_EQ(TokenPair(0,2,4), res[1]);
    EXPECT_EQ(TokenPair(1,0,3), res[2]);
    EXPECT_EQ(tokenPairs.size(), t.GetTopTokenPairs(tokenPairs.size() + 1, tokenPairs).size());
}

TEST_F(TokenizerTest, ReplaceTokenPairWithToken)
{
    Tokenizer t;
    t.tokens = 
    {
        {0, Token(0)},
        {1, Token(1)},
        {2, Token(2)},
    };
    t.tokens[0].count = 10;
    t.tokens[1].count = 20;
    t.tokens[2].count = 30;
    std::vector<Token::Data> sequence =
    {
        0, 0, 1, 1, 0
    };
    TokenPair tokenPair(2,3);
    Token::Data newToken(2);
    t.ReplaceTokenPairWithToken(sequence,tokenPair,newToken);
    EXPECT_EQ(5, sequence.size());
    EXPECT_EQ(10, t.tokens[0].count);
    EXPECT_EQ(20, t.tokens[1].count);
    EXPECT_EQ(30, t.tokens[2].count);
    tokenPair = TokenPair(0,1);
    t.ReplaceTokenPairWithToken(sequence,tokenPair,newToken);
    EXPECT_EQ(4, sequence.size());
    EXPECT_EQ(9, t.tokens[0].count);
    EXPECT_EQ(19, t.tokens[1].count);
    EXPECT_EQ(31, t.tokens[2].count);
}

TEST_F(TokenizerTest, FeedInput)
{
    Tokenizer t;
    t.FeedInput("this is a test");
    EXPECT_EQ(7, t.tokens.size());
    EXPECT_EQ(7, t.sequences.size());
    t.FeedInput("this is also a test");
    EXPECT_EQ(9, t.tokens.size());
    EXPECT_EQ(16, t.sequences.size());
}

TEST_F(TokenizerTest, OptimizeTokenLength)
{
    Tokenizer t;
    t.FeedInput("this is a test");
    EXPECT_EQ(7, t.tokens.size()); // 7 uniq characters used
    EXPECT_EQ(7, t.sequences.size());
    t.OptimizeTokenLength(100,1); 
    EXPECT_EQ(5, t.GetUsedTokensCount());
}

TEST_F(TokenizerTest, CreateMappingForTokensByCount)
{
    Tokenizer t;
    for (Token::Data id = 0; id < 12; ++id)
    {
        t.tokens.emplace(id, Token(id));
        t.tokens[id].count = (id * 7 + 3) % 13 + 1;
    }
    // Empty tokens are not included in the mapping.
    t.tokens.emplace(12, Token(12));
    t.tokens[12].type = Token::Type::Empty;
    t.tokens[12].count = 100;

    const TokensMappedByCount mapping = t.CreateMappingForTokensByCount();
    ASSERT_EQ(12, mapping.size());
    for (Token::Data id = 0; id < 12; ++id)
    {
        Token::Data expectedRank = 0;
        for (Token::Data other = 0; other < 12; ++other)
        {
            if (t.tokens[other].count > t.tokens[id].count)
                ++expectedRank;
        }
        EXPECT_EQ(expectedRank, mapping.at(id)) << "token " << id;
    }
}

TEST_F(TokenizerTest, GetTokenTextSequence)
{
    Tokenizer t;
    t.tokens.emplace('a', Token('a'));
    t.tokens.emplace('b', Token('b'));
    t.tokens.emplace('c', Token('c'));
    t.tokens.emplace(256, Token(256, 'a', 'b'));
    t.tokens.emplace(257, Token(257, 256, 'c'));

    EXPECT_EQ("a", t.GetTokenTextSequence('a'));
    EXPECT_EQ("ab", t.GetTokenTextSequence(256));
    EXPECT_EQ("abc", t.GetTokenTextSequence(257));
}

TEST_F(TokenizerTest, FullFunctionalityTest)
{
    std::string randomFilename = GetRandomFilename("sq3");
    Tokenizer t;
    t.FeedInput("This is a test");
    ASSERT_EQ(7, t.sequences.size());
    ASSERT_EQ(8, t.tokens.size());

    t.OptimizeTokenLength(1024,1);
    ASSERT_EQ(5, t.GetUsedTokensCount());
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
    std::deque<Token::Data> pastTokens;
    for (const auto& sequence : t.sequences)
    {
        for (const auto token : sequence)
        {
            auto& relations = expectedTokenRelations[token];
            if (relations.empty())
            {
                relations.resize(5);
            }
            pastTokens.emplace_front(token);
            if (pastTokens.size() > 6)
            {
                pastTokens.pop_back();
            }
            for (size_t depth = 1; depth < pastTokens.size(); ++depth)
            {
                ++relations[depth - 1][pastTokens[depth]];
            }
        }
    }
    EXPECT_EQ(expectedTokenRelations, t.tokenRelations);

    t.StoreCalculatedDataToDB(randomFilename);

    SQLite::DB db;
    db.Open(randomFilename, true);
    EXPECT_EQ(5, db.ExecSingleInt64("SELECT COUNT(*) FROM Tokens;"));
    EXPECT_EQ(4, db.ExecSingleInt64("SELECT COUNT(*) FROM (SELECT Token FROM TokenRelations WHERE Depth = 0 GROUP BY Token);"));
}
