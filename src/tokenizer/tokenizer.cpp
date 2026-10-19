#include <algorithm>
#include <cassert>
#include <deque>
#include <map>
#include <unordered_set>
#include <set>

#include "tokenizer.h"
#include "db.h"

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
    // cast to uint8_t, just in case the system uses signed char. 
    separators[(uint8_t)' '] = true;
    separators[(uint8_t)'.'] = true;
    separators[(uint8_t)';'] = true;
    separators[(uint8_t)':'] = true;
    separators[(uint8_t)'\''] = true;
    separators[(uint8_t)'\"'] = true;
    separators[(uint8_t)'\n'] = true;
    separators[(uint8_t)'\t'] = true;
    std::vector<Token::Data> running;
    for(uint8_t t: input)
    {
        ++ascii[t].count;
        if(separators[t])
        {
            if(!running.empty())
            {
                sequences.emplace_back(running);
            }
            sequences.emplace_back(std::vector<Token::Data>{t});
            running.clear();
        }
        else
        {
            running.emplace_back(t);
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
    empty_tokens_count = 0;
}

size_t Tokenizer::GetUsedTokensCount() const
{
    return tokens.size() - empty_tokens_count;
}

TokenPairSet Tokenizer::CreateTokenPairsForSequence(const TokenSequence & sequence) const
{
    TokenPairSet tokenPairs;
    for(size_t i=0,j=1;j<sequence.size();++i,++j)
    {
        auto res = tokenPairs.emplace(TokenPair(sequence[i],sequence[j]));
        res.first->count++;
    }
    return tokenPairs;
}

std::vector<TokenPairSet> Tokenizer::CreateTokenPairsForSequences(const TokenSequences & sequences) const
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
    TokenPairSet tokenPairs;
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

void Tokenizer::ReplaceTokenPairWithToken(TokenSequence & sequence, const TokenPair & tokenPair,const Token::Data newToken)
{
    size_t write = 0;
    for(size_t read = 0; read < sequence.size();)
    {
        if(read + 1 < sequence.size() && sequence[read] == tokenPair.left && sequence[read + 1] == tokenPair.right)
        {
            auto & left = tokens.at(tokenPair.left);
            auto & right = tokens.at(tokenPair.right);
            auto DecreaseTokenCount = [this](Token & t)
            {
                if(--t.count == 0)
                {
                    ++empty_tokens_count;
                    t.type = t.type == Token::Type::Root ? Token::Type::Root : Token::Type::Empty;
                }
            };
            DecreaseTokenCount(left);
            DecreaseTokenCount(right);
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

void Tokenizer::MergeSpecificPairs(const TokenPairVector & pairsForMerging, std::vector<TokenPairSet> & sequencesTokenPairs)
{
    std::vector<Token::Data> newTokens;
    tokens.reserve(tokens.size()+pairsForMerging.size());
    for(const auto & pair: pairsForMerging)
    {
        Token t{nextToken++,pair.left,pair.right};
        tokens.emplace(t.token,t);
        newTokens.emplace_back(t.token);
    }
    auto MergePairs = [this, &pairsForMerging, &newTokens]
        (std::vector<Token::Data> & sequence,TokenPairSet & sequenceTokenPairs)
    {
        auto newToken = newTokens.begin();
        for(const auto & tokenPair : pairsForMerging)
        {
            auto sequenceTokenPairIter = sequenceTokenPairs.find(tokenPair);
            if( sequenceTokenPairIter != sequenceTokenPairs.end() )
            {
                ReplaceTokenPairWithToken(sequence,tokenPair,*newToken);
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

void Tokenizer::OptimizeTokenLength(size_t n, size_t step)
{
    // sanity check
    assert(nextToken>=(1<<8));
    auto sequencesTokenPairs = CreateTokenPairsForSequences(sequences);
    while(GetUsedTokensCount()<n)
    {
        // todo: have MergeSpecifPairs update topTokenPairs?
        auto topTokenPairs = GetTopTokenPairs(step, sequencesTokenPairs);
        if(topTokenPairs.empty())
        {
            break;
        }
        MergeSpecificPairs(topTokenPairs, sequencesTokenPairs);        
    }
}

void Tokenizer::CalculateTokenRelationships(size_t depth)
{
    std::deque<Token::Data> past;
    // create the data structure for every non-empty token
    decltype(tokenRelations.begin()->second) mapping;
    mapping.resize(depth);
    for(auto & kv : tokens)
    {
        auto & token = kv.second;
        if(token.type != Token::Type::Empty)
        {
            tokenRelations.emplace(kv.first,mapping);
        }
    }
    // process all the tokens/token sequences
    auto ProcessPastTokens = [this](const std::deque<Token::Data> & past)
    {
        auto & tokenPast = tokenRelations[past.front()];
        for(size_t i=1;i<past.size();++i)
        {
            tokenPast[i-1].emplace(past[i],0).first->second++;
        }
    };
    for(const auto & sequence : sequences)
    {
        for(const auto & token : sequence)
        {
            past.emplace_front(token);
            if(past.size()>depth+1)
            {
                past.pop_back();
            }
            ProcessPastTokens(past);
        }
    }
}

TokensMappedByCount Tokenizer::CreateMappingForTokensByCount() const
{
    std::vector<Token::Data> sortedTokens;
    for(const auto & token: tokens)
    {
        if(token.second.type != Token::Type::Empty)
        {
            sortedTokens.emplace_back(token.first);
        }
    }
    std::sort(sortedTokens.begin(),sortedTokens.end(),[&](const Token::Data & a, const Token::Data & b)
    {
        auto count_a = tokens.find(a)->second.count;
        auto count_b = tokens.find(b)->second.count;
        return count_a > count_b;
    });
    std::unordered_map<Token::Data,Token::Data,Token::HashFunction> mappedTokens;
    for(Token::Data i=0;i<sortedTokens.size();++i)
    {
        mappedTokens.emplace(sortedTokens[i],i);
    }
    return mappedTokens;
}

void Tokenizer::StoreCalculatedDataToDB(const std::string & filename) const
{
    // map the tokens in order, highest count first, for more compact storage in sqlite3
    SQLite::DB db;
    db.Open(filename, false);

    TokensMappedByCount mappedTokens = CreateMappingForTokensByCount();
}

