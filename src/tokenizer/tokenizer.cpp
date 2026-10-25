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
            auto iter = tokens.emplace(t.token, t);
            if( !iter.second )
            {
                iter.first->second.count += t.count;
            }
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

TokenPairVector Tokenizer::GetTopTokenPairs(size_t n, const TokenPairSet & tokenPairSet) const
{
    TokenPairVector res(tokenPairSet.begin(),tokenPairSet.end());
    std::sort(res.begin(),res.end(),[&](const TokenPair & a, const TokenPair & b)
    {
        if( a.count != b.count )
        {
            return a.count > b.count;
        }
        if( a.left != b.left )
        {
            return a.left > b.left;
        }
        return a.right > b.right;
    });
    res.resize(std::min(n, res.size()));
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

void Tokenizer::MergeSpecificPairs(
       const TokenPairVector & pairsForMerging, 
       TokenPairSets & sequencesTokenPairs,
       TokenPairSet & flattenedTokenPairs)
{
    std::vector<Token::Data> newTokens;
    tokens.reserve(tokens.size()+pairsForMerging.size());
    for(const auto & pair: pairsForMerging)
    {
        Token t{nextToken++,pair.left,pair.right};
        tokens.emplace(t.token,t);
        newTokens.emplace_back(t.token);
    }
    auto MergePairs = [this, &pairsForMerging, &newTokens, &flattenedTokenPairs]
        (std::vector<Token::Data> & sequence,TokenPairSet & sequenceTokenPairs)
    {
        auto AdjustFlattenedCount = [&flattenedTokenPairs](const TokenPair & pair, bool add)
        {
            auto iter = flattenedTokenPairs.find(pair);
            if(add)
            {
                if(iter == flattenedTokenPairs.end())
                {
                    flattenedTokenPairs.emplace(pair);
                }
                else
                {
                    iter->count += pair.count;
                }
            }
            else if(iter != flattenedTokenPairs.end())
            {
                if(iter->count <= pair.count)
                {
                    flattenedTokenPairs.erase(iter);
                }
                else
                {
                    iter->count -= pair.count;
                }
            }
        };
        auto newToken = newTokens.begin();
        for(const auto & tokenPair : pairsForMerging)
        {
            auto sequenceTokenPairIter = sequenceTokenPairs.find(tokenPair);
            if( sequenceTokenPairIter != sequenceTokenPairs.end() )
            {
                for(const auto & pair : sequenceTokenPairs)
                {
                    AdjustFlattenedCount(pair, false);
                }
                ReplaceTokenPairWithToken(sequence,tokenPair,*newToken);
                sequenceTokenPairs = CreateTokenPairsForSequence(sequence);
                for(const auto & pair : sequenceTokenPairs)
                {
                    AdjustFlattenedCount(pair, true);
                }
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

TokenPairSet Tokenizer::FlattenSequenceTokenPairs(const TokenPairSets & tokenPairSets)
{
    TokenPairSet res;
    for(const auto & tokenPairSet : tokenPairSets)
    {
        for(const auto & tokenPair : tokenPairSet)
        {
            auto iter = res.emplace(tokenPair);
            if( !iter.second )
            {
                iter.first->count += tokenPair.count;
            }
        }
    }
    return res;
}

void Tokenizer::OptimizeTokenLength(size_t n, size_t step)
{
    // sanity check
    assert(nextToken>=(1<<8));
    auto sequencesTokenPairs = CreateTokenPairsForSequences(sequences);
    auto flattenedTokenPairs = FlattenSequenceTokenPairs(sequencesTokenPairs);
    while(GetUsedTokensCount()<n)
    {
        auto topTokenPairs = GetTopTokenPairs(step, flattenedTokenPairs);
        if(topTokenPairs.empty())
        {
            break;
        }
        MergeSpecificPairs(topTokenPairs, sequencesTokenPairs, flattenedTokenPairs);        
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
        if(token.count)
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

std::string Tokenizer::GetTokenTextSequence(const Token::Data & token) const
{
    std::string text;
    std::vector<Token::Data> pending{token};
    while(!pending.empty())
    {
        const Token & current = tokens.at(pending.back());
        pending.pop_back();
        if(current.type == Token::Type::Root)
        {
            text.push_back(static_cast<char>(current.token));
        }
        else if(current.type == Token::Type::Node)
        {
            pending.push_back(current.right);
            pending.push_back(current.left);
        }
    }
    return text;
}

void Tokenizer::StoreCalculatedDataToDB(const std::string & filename) const
{
    // map the tokens in order, highest count first, for more compact storage in sqlite3
    SQLite::DB db;
    db.Open(filename, false);

    db.ExecDML("DROP TABLE IF EXISTS Tokens");
    db.ExecDML("DROP TABLE IF EXISTS TokenRelations");
    
    db.ExecDML("CREATE TABLE Tokens(Token INTEGER PRIMARY KEY, Count INTEGER, Sequence TEXT);");
    db.ExecDML("CREATE TABLE TokenRelations(RelationId INTEGER PRIMARY KEY ASC, Token INTEGER, Depth INTEGER, ConnectedToken INTEGER, Count INTEGER);");

    TokensMappedByCount mappedTokens = CreateMappingForTokensByCount();
    db.BeginTransaction();
    auto tokensStatement = db.CompileStatement("INSERT INTO Tokens(Token, Count, Sequence) VALUES(?, ?, ?);");
    for(const auto & kv : mappedTokens)
    {
        auto & token = tokens.at(kv.first);
        long long int count = token.count;
        if(count)
        {
            std::string sequence = GetTokenTextSequence(kv.first);
            tokensStatement.Bind(1,kv.second);
            tokensStatement.Bind(2,count);
            tokensStatement.Bind(3,sequence);
            tokensStatement.ExecDML();
        }
    }
    db.CommitTransaction();

    db.BeginTransaction();
    auto tokenRelationsStatement = db.CompileStatement("INSERT INTO TokenRelations(RelationId, Token, Depth, ConnectedToken, Count) VALUES(NULL, ?, ?, ?, ?);");
    for(const auto & tokenRelation : tokenRelations)
    {
        auto mappedTokenData = mappedTokens.at(tokenRelation.first);
        tokenRelationsStatement.Bind(1,mappedTokenData);
        for(size_t depth = 0; depth < tokenRelation.second.size(); ++depth)
        {
            tokenRelationsStatement.Bind(2,(long long int)depth);
            auto & depthRelations = tokenRelation.second[depth];
            for(const auto & depthRelation : depthRelations)
            {
                auto mappedConnectedTokenData = mappedTokens.at(depthRelation.first);
                long long int count = depthRelation.second;
                tokenRelationsStatement.Bind(3,mappedConnectedTokenData);
                tokenRelationsStatement.Bind(4,count);
                tokenRelationsStatement.ExecDML();
            }
        }
    }
    db.CommitTransaction();
    db.ExecDML("CREATE INDEX idx_TokenRelations_TokenDepth ON TokenRelations(Token,Depth);");
}

