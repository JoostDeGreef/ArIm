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
    , unusedTokensCount(256)
{
    // insert the 256 ascii chars, and indicate which characters are special
    std::array<bool, 256> separators= {};
    separators[(Token::Data)' '] = true;
    separators[(Token::Data)'.'] = true;
    separators[(Token::Data)';'] = true;
    separators[(Token::Data)':'] = true;
    separators[(Token::Data)'\''] = true;
    separators[(Token::Data)'\"'] = true;
    separators[(Token::Data)'\n'] = true;
    separators[(Token::Data)'\t'] = true;
    for(Token::Data t = 0; t<256; ++t)
    {
        tokens.emplace(t,t).first->second.separator = separators[t];
    }
}

void Tokenizer::FeedInput(const std::string& input)
{
    std::vector<Token::Data> running;
    for(uint8_t t: input)
    {
        auto & token = tokens[t];
        ++token.count;
        if(token.separator)
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
    // reset the empty tokens counter
    unusedTokensCount = 0;
    for(const auto & token: tokens)
    {
        if(token.second.count == 0)
        {
            ++unusedTokensCount;
        }
    }
}

size_t Tokenizer::GetUsedTokensCount() const
{
    return tokens.size() - unusedTokensCount;
}

void Tokenizer::CheckUsedTokensCount() const
{
    size_t count = 0;
    for(const auto & t : tokens)
    {
        if(t.second.count)
        {
            ++count;
        }
    }
    assert(count == GetUsedTokensCount());
}

TokenPairs Tokenizer::CreateTokenPairsForSequences()
{
    TokenPairs tokenPairs;
    tokenPairs.reserve(tokens.size()*tokens.size());
    size_t t = tokens.size();
    for(size_t s = 0;s < sequences.size(); ++s)
    {
        const auto & sequence = sequences[s];
        for(size_t i=0,j=1;j<sequence.size();++i,++j)
        {
            TokenPair key(t,sequence[i],sequence[j]);
            auto res = tokenPairs.try_emplace(key);
            if(res.second)
            {
                tokens.emplace(t,t).first->second.count = 0;
                ++t;
                ++unusedTokensCount;
            }
            res.first->second.emplace(s);
        }
    }
    return tokenPairs;
}

TokenPairSequence Tokenizer::GetTopTokenPairs(size_t n, const TokenPairs & tokenPairs) const
{
    constexpr size_t chunkSize = 100;
    struct TokenPairCount
    {
        TokenPair tokenpair;
        size_t count;
    };
    std::vector<TokenPairCount> tokenPairCounts;
    tokenPairCounts.reserve(chunkSize);
    auto SortTokenPairCounts = [&tokenPairCounts,&n]()
    {
        size_t count = min(n,tokenPairCounts.size());
        std::partial_sort(tokenPairCounts.begin(),tokenPairCounts.begin() + count,tokenPairCounts.end(),[](const TokenPairCount & a, const TokenPairCount & b)
        {
            return a.count > b.count;
        });
        tokenPairCounts.resize(count);
    };
    for(const auto & tokenPairSequences : tokenPairs)
    {
        tokenPairCounts.emplace_back(tokenPairSequences.first,tokenPairSequences.second.size());
        if(tokenPairCounts.size()>=chunkSize)
        {
            SortTokenPairCounts();
        }
    }
    SortTokenPairCounts();
    TokenPairSequence res;
    res.reserve(n);
    std::transform(tokenPairCounts.begin(), tokenPairCounts.end(), std::back_inserter(res), [](const TokenPairCount & a){ return a.tokenpair; });
    return res;
}

void Tokenizer::MergeTopTokenPairs(const TokenPairSequence & pairsForMerging, TokenPairs & tokenPairs)
{
    auto DecreaseTokenCount = [&](Token & t)
    {
        if(--t.count == 0)
        {
            ++unusedTokensCount;
        }
    };
    auto IncreaseTokenCount = [&](Token & t)
    {
        if(t.count++ == 0)
        {
            --unusedTokensCount;
        }
    };
    auto RemoveOnePair = [&](Token::Data left, Token::Data right, const size_t sequenceIndex)
    {
        auto tokenPairSequenceIter = tokenPairs.find(TokenPair(-1,left,right));
        if(tokenPairSequenceIter != tokenPairs.end())
        {
            auto & sequences = tokenPairSequenceIter->second;
            // only remove one, so use the iterator version
            auto iter = sequences.find(sequenceIndex);
            if(iter != sequences.end())
            {
                sequences.erase(iter);
                if(sequences.empty())
                {
                    tokenPairs.erase(tokenPairSequenceIter);
                }
            }
        }
    };
    auto AddOnePair = [&](Token::Data left, Token::Data right, const size_t sequenceIndex)
    {
        auto iter = tokenPairs.try_emplace(TokenPair(-1,left,right));
        iter.first->second.emplace(sequenceIndex);
        if(iter.second)
        {
            Token::Data t = tokens.size();
            iter.first->first.token = t;
            ++tokens.emplace(t,t).first->second.count;
        }
    };
    // merge the pair from pairsForMerging when found in sequence/sequenceTokenPairs
    auto ReplaceOnePair = [&](const TokenPair & tokenPair, const size_t sequenceIndex)
    {
        TokenSequence & sequence = sequences[sequenceIndex];
        for(size_t i = 1;i<sequence.size();++i)
        {
            if(sequence[i-1] == tokenPair.left && sequence[i-0] == tokenPair.right)
            {
                // update token counts
                CheckUsedTokensCount();
                DecreaseTokenCount(tokens.at(sequence[i-1]));
                DecreaseTokenCount(tokens.at(sequence[i-0]));
                IncreaseTokenCount(tokens.at(tokenPair.token));
                CheckUsedTokensCount();
                // remove pairs
                if(i>1) RemoveOnePair(sequence[i-2],sequence[i-1],sequenceIndex);
                RemoveOnePair(sequence[i-1], sequence[i-0],sequenceIndex);
                if(i+1<sequence.size()) RemoveOnePair(sequence[i-0],sequence[i+1],sequenceIndex);
                // update sequence
                sequence[i-1] = tokenPair.token;
                sequence.erase(sequence.begin()+i);
                // add pairs
                if(i>1) AddOnePair(sequence[i-2],sequence[i-1],sequenceIndex);
                if(i<sequence.size()) AddOnePair(sequence[i-1],sequence[i-0],sequenceIndex);
                // remove only one pair, so return here
                CheckUsedTokensCount();
                return;
            }
        }
        assert(false); // should not be reached. sequenceTokenPairs promised the token pair exists!
    };
    // todo:
    // run this multiple times until nothing changes. 
    // the newly added tokenpairs could be the ones we are replacing
    for(const auto & pairForMerging : pairsForMerging)
    {
        auto iter = tokenPairs.find(pairForMerging);
        if(iter == tokenPairs.end())
        {
            continue;
        }
        // the indices have to be copied since tokenPairs will be adjusted on the fly
        std::vector<size_t> indices(iter->second.begin(),iter->second.end());
        for(const size_t sequenceIndex: indices)
        {
            ReplaceOnePair(pairForMerging,sequenceIndex);
        }
    }
}


void Tokenizer::OptimizeTokenLength(size_t n, size_t step)
{
    CheckUsedTokensCount();
    auto tokenPairs = CreateTokenPairsForSequences();
    CheckUsedTokensCount();
    while(GetUsedTokensCount()<n)
    {
        CheckUsedTokensCount();
        auto topTokenPairs = GetTopTokenPairs(step, tokenPairs);
        if(topTokenPairs.empty())
        {
            break;
        }
        MergeTopTokenPairs(topTokenPairs, tokenPairs);        
    }
    CheckUsedTokensCount();
}

// void Tokenizer::CalculateTokenRelationships(size_t depth)
// {
//     std::deque<Token::Data> past;
//     // create the data structure for every non-empty token
//     decltype(tokenRelations.begin()->second) mapping;
//     mapping.resize(depth);
//     for(auto & kv : tokens)
//     {
//         auto & token = kv.second;
//         if(token.count)
//         {
//             tokenRelations.emplace(kv.first,mapping);
//         }
//     }
//     // process all the tokens/token sequences
//     auto ProcessPastTokens = [this](const std::deque<Token::Data> & past)
//     {
//         auto & tokenPast = tokenRelations[past.front()];
//         for(size_t i=1;i<past.size();++i)
//         {
//             tokenPast[i-1].emplace(past[i],0).first->second++;
//         }
//     };
//     for(const auto & sequence : sequences)
//     {
//         for(const auto & token : sequence)
//         {
//             past.emplace_front(token);
//             if(past.size()>depth+1)
//             {
//                 past.pop_back();
//             }
//             ProcessPastTokens(past);
//         }
//     }
// }

// TokensMappedByCount Tokenizer::CreateMappingForTokensByCount() const
// {
//     struct TokenDataCount
//     {
//         Token::Data data;
//         size_t count;
//     };
//     std::vector<TokenDataCount> sortedTokens;
//     sortedTokens.reserve(GetUsedTokensCount());
//     for(const auto & token: tokens)
//     {
//         if(token.second.type != Token::Type::Empty)
//         {
//             sortedTokens.emplace_back(token.first, token.second.count);
//         }
//     }
//     std::sort(sortedTokens.begin(),sortedTokens.end(),[&](const TokenDataCount & a, const TokenDataCount & b)
//     {
//         return a.count > b.count;
//     });
//     std::unordered_map<Token::Data,Token::Data,Token::HashFunction> mappedTokens;
//     for(Token::Data i=0;i<sortedTokens.size();++i)
//     {
//         mappedTokens.emplace(sortedTokens[i].data,i);
//     }
//     return mappedTokens;
// }

// std::string Tokenizer::GetTokenTextSequence(const Token::Data & token) const
// {
//     std::string text;
//     std::vector<Token::Data> pending{token};
//     while(!pending.empty())
//     {
//         const Token & current = tokens.at(pending.back());
//         pending.pop_back();
//         if(current.type == Token::Type::Root)
//         {
//             text.push_back(static_cast<char>(current.token));
//         }
//         else if(current.type == Token::Type::Node)
//         {
//             pending.push_back(current.right);
//             pending.push_back(current.left);
//         }
//     }
//     return text;
// }

// void Tokenizer::StoreCalculatedDataToDB(const std::string & filename) const
// {
//     // map the tokens in order, highest count first, for more compact storage in sqlite3
//     SQLite::DB db;
//     db.Open(filename, false);

//     db.ExecDML("PRAGMA journal_mode = WAL;");
//     db.ExecDML("PRAGMA synchronous = NORMAL;");
//     db.ExecDML("PRAGMA temp_store = MEMORY;");

//     db.ExecDML("DROP TABLE IF EXISTS Tokens");
//     db.ExecDML("DROP TABLE IF EXISTS TokenRelations");
    
//     db.ExecDML("CREATE TABLE Tokens(Token INTEGER PRIMARY KEY, Count INTEGER, Sequence TEXT) WITHOUT ROWID;");
//     db.ExecDML("CREATE TABLE TokenRelations(RelationId INTEGER PRIMARY KEY, Token INTEGER, Depth INTEGER, ConnectedToken INTEGER, Count INTEGER);");

//     TokensMappedByCount mappedTokens = CreateMappingForTokensByCount();
//     db.BeginTransaction();
//     auto tokensStatement = db.CompileStatement("INSERT INTO Tokens(Token, Count, Sequence) VALUES(?, ?, ?);");
//     for(const auto & kv : mappedTokens)
//     {
//         auto & token = tokens.at(kv.first);
//         long long int count = token.count;
//         if(count)
//         {
//             std::string sequence = GetTokenTextSequence(kv.first);
//             tokensStatement.Bind(1,kv.second);
//             tokensStatement.Bind(2,count);
//             tokensStatement.Bind(3,sequence);
//             tokensStatement.ExecDML();
//         }
//     }
//     db.CommitTransaction();

//     db.BeginTransaction();
//     auto tokenRelationsStatement = db.CompileStatement("INSERT INTO TokenRelations(RelationId, Token, Depth, ConnectedToken, Count) VALUES(NULL, ?, ?, ?, ?);");
//     for(const auto & tokenRelation : tokenRelations)
//     {
//         auto mappedTokenData = mappedTokens.at(tokenRelation.first);
//         tokenRelationsStatement.Bind(1,mappedTokenData);
//         for(size_t depth = 0; depth < tokenRelation.second.size(); ++depth)
//         {
//             tokenRelationsStatement.Bind(2,(long long int)depth);
//             auto & depthRelations = tokenRelation.second[depth];
//             for(const auto & depthRelation : depthRelations)
//             {
//                 auto mappedConnectedTokenData = mappedTokens.at(depthRelation.first);
//                 long long int count = depthRelation.second;
//                 tokenRelationsStatement.Bind(3,mappedConnectedTokenData);
//                 tokenRelationsStatement.Bind(4,count);
//                 tokenRelationsStatement.ExecDML();
//             }
//         }
//     }
//     db.CommitTransaction();

//     db.ExecDML("CREATE INDEX idx_TokenRelations_TokenDepth ON TokenRelations(Token,Depth);");
// }

