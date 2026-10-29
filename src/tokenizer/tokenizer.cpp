#include <algorithm>
#include <cassert>
#include <deque>
#include <map>
#include <unordered_set>
#include <set>

#include "tokenizer.h"

using namespace std;

Tokenizer::Tokenizer()
    : tokens()
    , unusedTokensCount(0)
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
        tokens.emplace(t).first->separator = separators[t];
    }
    unusedTokensCount = tokens.size();
}

Tokenizer::Tokenizer(SQLite::DB & db)
{
    LoadDataFromDB(db);
}

void Tokenizer::FeedInput(const std::string& input)
{
    std::vector<Token::Data> running;
    for(uint8_t t: input)
    {
        const auto & token = tokens.find(t);
        ++token->count;
        if(token->separator)
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
        if(token.count == 0)
        {
            ++unusedTokensCount;
        }
    }
}

size_t Tokenizer::GetUsedTokensCount() const
{
    return tokens.size() - unusedTokensCount;
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
                tokens.emplace(t,sequence[i],sequence[j]).first->count = 0;
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
    auto DecreaseTokenCount = [&](Token::Data & t)
    {
        if(--tokens.find(t)->count == 0)
        {
            ++unusedTokensCount;
        }
    };
    auto IncreaseTokenCount = [&](Token::Data & t)
    {
        if(tokens.find(t)->count++ == 0)
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
            //++tokens.emplace(t,left,right).first->count;
            tokens.emplace(t,left,right);
            ++unusedTokensCount;
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
                DecreaseTokenCount(sequence[i-1]);
                DecreaseTokenCount(sequence[i-0]);
                IncreaseTokenCount(tokenPair.token);
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
                return true;
            }
        }
        return false;
    };
    bool changed;
    do
    {
        changed = false;
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
                changed |= ReplaceOnePair(pairForMerging,sequenceIndex);
            }
        }
    } while(changed);
}


void Tokenizer::OptimizeTokenLength(size_t n, size_t step)
{
    auto tokenPairs = CreateTokenPairsForSequences();
    while(GetUsedTokensCount()<n)
    {
        auto topTokenPairs = GetTopTokenPairs(step, tokenPairs);
        if(topTokenPairs.empty())
        {
            break;
        }
        MergeTopTokenPairs(topTokenPairs, tokenPairs);        
    }
}

void Tokenizer::CalculateTokenRelationships(size_t depth)
{
    std::deque<Token::Data> past;
    // create the data structure for every non-empty token
    decltype(tokenRelations.begin()->second) mapping;
    mapping.resize(depth);
    for(auto & token : tokens)
    {
        if(token.count)
        {
            tokenRelations.emplace(token.token,mapping);
        }
    }
    // process all the tokens/token sequences
    auto ProcessPastTokens = [this](const std::deque<Token::Data> & past)
    {
        auto & tokenPast = tokenRelations[past.front()];
        for(size_t i=1;i<past.size();++i)
        {
            tokenPast[i-1].emplace(past[i]).first->count++;
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

std::string Tokenizer::GetTokenTextSequence(const Token & token) const
{
    std::string text;
    std::vector<Token::Data> pending{token.token};
    while(!pending.empty())
    {
        const Token & current = *tokens.find(pending.back());
        pending.pop_back();
        if(current.left == Token::root && current.right == Token::root)
        {
            text.push_back(current.token);
        }
        else
        {
            pending.push_back(current.right);
            pending.push_back(current.left);
        }
    }
    return text;
}

void Tokenizer::StoreCalculatedDataToDB(SQLite::DB & db) const
{
    // map the tokens in order, highest count first, for more compact storage in sqlite3
    db.ExecDML("DROP TABLE IF EXISTS Tokens");
    db.ExecDML("DROP TABLE IF EXISTS TokenRelations");
    db.ExecDML("DROP TABLE IF EXISTS Sequences");
    
    db.ExecDML("CREATE TABLE Tokens(Token INTEGER PRIMARY KEY, Left INTEGER, Right INTEGER, Count INTEGER) WITHOUT ROWID;");
    db.ExecDML("CREATE TABLE TokenRelations(RelationId INTEGER PRIMARY KEY, Token INTEGER, Depth INTEGER, ConnectedTokens BLOB);");
    db.ExecDML("CREATE TABLE Sequences(SequenceId INTEGER PRIMARY KEY, Data BLOB);");

    // write tokens
    db.BeginTransaction();
    {
        auto tokensStatement = db.CompileStatement("INSERT INTO Tokens(Token, Left, Right, Count) VALUES(?, ?, ?, ?);");
        for(const auto & token : tokens)
        {
            long long int count = token.count;
            std::string sequence = GetTokenTextSequence(token);
            tokensStatement.Bind(1,token.token);
            tokensStatement.Bind(2,token.left);
            tokensStatement.Bind(3,token.right);
            tokensStatement.Bind(4,count);
            tokensStatement.ExecDML();
        }
    }
    db.CommitTransaction();

    // write relations
    db.BeginTransaction();
    {
        TokenCounts counts;
        counts.reserve(1000);
        auto tokenRelationsStatement = db.CompileStatement("INSERT INTO TokenRelations(RelationId, Token, Depth, ConnectedTokens) VALUES(NULL, ?, ?, ?);");
        auto SaveCounts = [&]()
        {
            tokenRelationsStatement.Bind(3,reinterpret_cast<unsigned char *>(counts.data()),counts.size()*sizeof(TokenCount));
            tokenRelationsStatement.ExecDML();
            counts.clear();
        };
        for(const auto & tokenRelation : tokenRelations)
        {
            tokenRelationsStatement.Bind(1,tokenRelation.first);

            for(size_t depth = 0; depth < tokenRelation.second.size(); ++depth)
            {
                tokenRelationsStatement.Bind(2,(long long int)depth);

                const auto & depthRelations = tokenRelation.second[depth];
                for(const auto & depthRelation : depthRelations)
                {
                    counts.emplace_back(TokenCount{depthRelation.token,depthRelation.count});
                    if(counts.size()>=1000)
                    {
                        SaveCounts();
                    }
                }
                if(!counts.empty() || depthRelations.empty())
                {
                    SaveCounts();
                }
            }
        }
    }
    db.CommitTransaction();

    // write sequence
    db.BeginTransaction();
    {
        std::vector<unsigned char> flatSequence;
        flatSequence.reserve(4200);
        auto sequenceStatement = db.CompileStatement("INSERT INTO Sequences(SequenceId, Data) VALUES(NULL, ?);");
        auto SaveFlatSequence = [&]()
        {
            sequenceStatement.Bind(1,flatSequence.data(),flatSequence.size());
            sequenceStatement.ExecDML();
            flatSequence.clear();
        };
        auto AddSize = [&](size_t size)
        {
            flatSequence.push_back(size);
        };
        auto AddToken = [&](Token::Data token)
        {
            unsigned char * p = reinterpret_cast<unsigned char *>(&token);
            for(size_t i = 0, j=sizeof(token)-1;i<sizeof(token); ++i,--j)
            {
                flatSequence.push_back(p[j]);
            }
        };
        for(const auto & sequence : sequences)
        {
            AddSize(sequence.size());
            for(const auto & token : sequence)
            {
                AddToken(token);
            }
            if(flatSequence.size()>=4096)
            {
                SaveFlatSequence();
            }
        }
        if(!flatSequence.empty() || sequences.empty())
        {
            SaveFlatSequence();
        }
    }
    db.CommitTransaction();

    db.BeginTransaction();
    db.ExecDML("CREATE INDEX idx_TokenRelations_TokenDepth ON TokenRelations(Token,Depth);");
    db.CommitTransaction();
}

void Tokenizer::LoadDataFromDB(SQLite::DB & db)
{
    // read tokens
    db.BeginTransaction();
    {
        tokens.reserve(1000); // TODO: read nr of tokens from db
        auto query = db.ExecQuery("SELECT Token, Left, Right, Count FROM Tokens;");
        for(const auto &row : query.GetRows())
        {
            auto token = row.GetInt64Field(0);
            auto left = row.GetInt64Field(1);
            auto right = row.GetInt64Field(2);
            auto count = row.GetInt64Field(3);
            tokens.emplace(token,left,right).first->count = count;
            if(count == 0) ++unusedTokensCount;
        }
    }
    db.CommitTransaction();

    // read relations
    db.BeginTransaction();
    {
        tokenRelations.reserve(1000);
        auto query = db.ExecQuery("SELECT Token, Depth, ConnectedTokens FROM TokenRelations;");
        for(const auto &row : query.GetRows())
        {
            auto token = row.GetInt64Field(0);
            auto depth = row.GetInt64Field(1);
            auto connectedTokens = row.GetBlobField(2);
            auto iter = tokenRelations.try_emplace(token);
            if(depth>=(decltype(depth))(iter.first->second.size()))
            {
                iter.first->second.resize(depth+1);
            }
            for(size_t i=0;i<connectedTokens.size();i+=sizeof(TokenCount))
            {
                TokenCount * t = reinterpret_cast<TokenCount *>(&connectedTokens[i]);
                iter.first->second[depth].emplace(t->token).first->count = t->count;
            }
        }
    }
    db.CommitTransaction();

    // read sequences
    db.BeginTransaction();
    {
        sequences.reserve(1000);
        TokenSequence sequence;
        sequence.reserve(32);
        auto query = db.ExecQuery("SELECT Data FROM Sequences ORDER BY SequenceId;");
        for(const auto &row : query.GetRows())
        {
            auto data = row.GetBlobField(0);
            auto iter = data.begin();
            while(iter != data.end())
            {
                size_t size = *iter++;
                for(size_t i = 0;i<size; ++i)
                {
                    Token::Data token = *iter++;
                    for(size_t j = 1;j<sizeof(token);++j)
                    {
                        token <<= 8;
                        token |= *iter++;
                    }
                    sequence.emplace_back(token);
                }
                sequences.emplace_back(sequence);
                sequence.clear();
            }
        }
    }
    db.CommitTransaction();
}
