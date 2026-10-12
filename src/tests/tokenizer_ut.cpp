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
    std::vector<Token::data_type> sequence;
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
    std::vector<std::vector<Token::data_type>> sequences;
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
    TokenPairSet tps{TokenPair(0,0,2),TokenPair(0,1,1),TokenPair(0,2,3)};
    sequencesTokenPairs.emplace_back(tps);
    res = t.GetTopTokenPairs(2, sequencesTokenPairs);
    EXPECT_EQ(2, res.size());
    EXPECT_EQ(3, res[0].count);
    EXPECT_EQ(2, res[1].count);
}

