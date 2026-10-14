#include <sqlite3.h>

#include <gtest/gtest.h>
using namespace testing;

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
    std::vector<TokenPairSet> sequencesTokenPairs;
    auto res = t.GetTopTokenPairs(2, sequencesTokenPairs);
    EXPECT_EQ(0, res.size());
    {
        TokenPairSet tps{TokenPair(0,0,2),TokenPair(0,1,1),TokenPair(0,2,4)};
        sequencesTokenPairs.emplace_back(tps);
    }
    res = t.GetTopTokenPairs(2, sequencesTokenPairs);
    EXPECT_EQ(2, res.size());
    EXPECT_EQ(4, res[0].count);
    EXPECT_EQ(2, res[1].count);
    {
        TokenPairSet tps{TokenPair(0,0,2),TokenPair(0,1,1),TokenPair(0,3,3)};
        sequencesTokenPairs.emplace_back(tps);
    }
    res = t.GetTopTokenPairs(3, sequencesTokenPairs);
    EXPECT_EQ(3, res.size());
    EXPECT_EQ(4, res[0].count);
    EXPECT_EQ(4, res[1].count);
    EXPECT_EQ(3, res[2].count);
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
