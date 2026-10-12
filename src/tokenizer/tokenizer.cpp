#include <algorithm>
#include <cassert>
#include <map>
#include <unordered_set>
#include <set>

#include "tokenizer.h"

using namespace std;

Tokenizer::Tokenizer()
    : tokens()
{
}

void Tokenizer::FeedInput(const std::string& input)
{
    // sanity check to see if functions are called in the right order
    assert(nextToken<=(1<<8));
    // original input tokens
    std::array<Token,1<<8> ascii;
    for(uint16_t t = 0; t<256; ++t)
    {
        ascii[t] = Token(t);
    }
    std::array<bool, 256> separators= {};
    separators[' '] = true;
    separators['.'] = true;
    separators[';'] = true;
    separators[':'] = true;
    separators['\''] = true;
    separators['\"'] = true;
    separators['\n'] = true;
    separators['\t'] = true;
    std::vector<Token::data_type> running;
    for(uint16_t t: input)
    {
        ++ascii[t].count;
        if(separators[t])
        {
            if(!running.empty())
            {
                sequences.emplace_back(running);
            }
            sequences.emplace_back(std::vector<Token::data_type>{t});
            running.clear();
        }
    }
    if(!running.empty())
    {
        sequences.emplace_back(running);
    }
    for(auto & t: ascii)
    {
        if(t.count)
        {
            tokens.emplace(t.token, t);
        }
    }
    nextToken = 1<<8;
}

TokenPairSet Tokenizer::CreateTokenPairsForSequence(const std::vector<Token::data_type> & sequence) const
{
    TokenPairSet tokenPairs;
    for(size_t i=0,j=1;j<sequence.size();++i,++j)
    {
        auto res = tokenPairs.emplace(TokenPair(sequence[i],sequence[j]));
        res.first->count++;
    }
    return tokenPairs;
}

std::vector<TokenPairSet> Tokenizer::CreateTokenPairsForSequences(const std::vector<std::vector<Token::data_type>> & sequences) const
{
    std::vector<TokenPairSet> res;
    res.reserve(sequences.size());
    for(const auto & sequence: sequences)
    {
        auto pairs = CreateTokenPairsForSequence(sequence);
        res.emplace_back(pairs);
    }
    return res;
}

TokenPairVector Tokenizer::GetTopTokenPairs(size_t n, const std::vector<TokenPairSet> & sequencesTokenPairs) const
{
    // summarize tokenPairs for all sequences
    std::unordered_set<TokenPair,TokenPair::HashFunction> tokenPairs;
    TokenPair tp;
    std::vector<const TokenPair*> topN(n,&tp); // list of high counts, lowest at back()
    auto CollectPairs = [&tokenPairs,&topN,&tp](const TokenPairSet & sequenceTokenPairs)
    {
        for(const auto & tokenPair : sequenceTokenPairs)
        {
            auto res = tokenPairs.emplace(tokenPair);
            if(!res.second)
            {
                res.first->count += tokenPair.count;
            }
            if(topN.back()->count < res.first->count || topN.back() == &(*res.first))
            {
                // the tokenpair might already be in the top 5, remove it first
                for(auto iter=topN.begin();iter<topN.end();++iter)
                {
                    if(*iter == &(*res.first))
                    {
                        topN.erase(iter);
                        topN.push_back(&tp);
                        break;
                    }
                }
                for(auto iter=topN.begin();iter<topN.end();++iter)
                {
                    if(res.first->count > (*iter)->count)
                    {
                        topN.insert(iter, &(*res.first));
                        topN.pop_back();
                        break;
                    }
                }
            }
        }
    };
    for(const auto & sequenceTokenPairs: sequencesTokenPairs)
    {
        CollectPairs(sequenceTokenPairs);
    }
    TokenPairVector res;
    for(const auto p: topN)
    {
        if(p->count)
        {
            res.emplace_back(*p);
        }
    }
    return res;
}

void Tokenizer::MergeSpecificPairs(const TokenPairVector & pairsForMerging, std::vector<TokenPairSet> & sequencesTokenPairs)
{
    std::vector<Token::data_type> newTokens;
    tokens.reserve(tokens.size()+pairsForMerging.size());
    for(const auto & pair: pairsForMerging)
    {
        Token t{nextToken++,pair.left,pair.right};
        tokens.emplace(t.token,t);
        newTokens.emplace_back(t.token);
    }
    auto replaceTokenPairWithToken = [this]
        (std::vector<Token::data_type> & sequence, const TokenPair & tokenPair,const Token::data_type newToken)
    {
        size_t write = 0;
        for(size_t read = 0; read < sequence.size();)
        {
            if(read + 1 < sequence.size() && sequence[read] == tokenPair.left && sequence[read + 1] == tokenPair.right)
            {
                --tokens.at(tokenPair.left).count;
                --tokens.at(tokenPair.right).count;
                ++tokens.at(newToken).count;
                sequence[write++] = newToken;
                read += 2;
            }
            else
            {
                sequence[write++] = sequence[read++];
            }
        }
        sequence.resize(write);
    };
    auto MergePairs = [&replaceTokenPairWithToken, this, &pairsForMerging, &newTokens]
        (std::vector<Token::data_type> & sequence,TokenPairSet & sequenceTokenPairs)
    {
        auto newToken = newTokens.begin();
        for(const auto & tokenPair : pairsForMerging)
        {
            auto sequenceTokenPairIter = sequenceTokenPairs.find(tokenPair);
            if( sequenceTokenPairIter != sequenceTokenPairs.end() )
            {
                replaceTokenPairWithToken(sequence,tokenPair,*newToken);
                sequenceTokenPairs = CreateTokenPairsForSequence(sequence);
            }
            ++newToken;
        }
    };
    auto sequencesIter = sequences.begin();
    auto sequencesTokenPairsIter = sequencesTokenPairs.begin();
    for(;
        sequencesIter != sequences.end() && sequencesTokenPairsIter != sequencesTokenPairs.end();
        ++sequencesIter, ++sequencesTokenPairsIter)
    {
        auto & sequence = *sequencesIter;
        auto & sequenceTokenPairs = *sequencesTokenPairsIter;
        MergePairs(sequence, sequenceTokenPairs);
    }
}

void Tokenizer::OptimizeTokenLength(size_t n)
{
    // sanity check
    assert(nextToken>=(1<<8));
    auto sequencesTokenPairs = CreateTokenPairsForSequences(sequences);
    while(tokens.size()<n)
    {
        // todo: have MergeSpecifPairs update topTokenPairs?
        auto topTokenPairs = GetTopTokenPairs(5, sequencesTokenPairs);
        if(topTokenPairs.empty())
        {
            break;
        }
        MergeSpecificPairs(topTokenPairs, sequencesTokenPairs);        
    }
}

void Tokenizer::CalculateTokenRelationships(size_t depth)
{}


