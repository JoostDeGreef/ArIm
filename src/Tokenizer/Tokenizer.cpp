#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <deque>
#include <map>
#include <queue>
#include <unordered_set>
#include <set>
#include <stdexcept>
#include <iostream>

#include "Timer.h"
#include "Tokenizer.h"
#include "RingBuffer.h"

using namespace std;

#define TIMER Timer timer([n = __func__, &fr = m_functionRuntimes](std::int64_t duration){ auto & v = fr[n]; v.first++; v.second += duration; })

Tokenizer::Tokenizer()
    : m_tokenizedInputSequence()
    , m_nextUnusedToken(512)
    , m_tokenPairCountAndPositions()
    , m_tokenPairReplacements()
    , m_tokenReplacements()
    , m_tokenRelationships()
    , m_functionRuntimes()
{}

Tokenizer::Tokenizer(SQLite::DB & db)
    : Tokenizer()
{
    LoadDataFromDB(db);
}

void Tokenizer::Process(const std::string& input)
{
    auto firstTokenToUpdate = m_tokenizedInputSequence.Last();
    FeedInput(input);
    UpdateTokenPairCountAndPositions(firstTokenToUpdate);
    ApplyExistingTokenPairReplacements();
    OptimizeTokenLength();
    CalculateTokenRelationships();
}

void Tokenizer::FeedInput(const std::string& input)
{
    TIMER;
    bool separated = !m_tokenizedInputSequence.Empty();
    for(uint8_t c: input)
    {
        if(isspace(c))
        {
            separated = true;
        }
        else
        {
            m_tokenizedInputSequence.EmplaceBack(c + (separated?256:0));
            separated = false;
        }
    }
}

void Tokenizer::UpdateTokenPairCountAndPositions(LinkedList<Token>::Iterator position)
{
    TIMER;
    if(position == m_tokenizedInputSequence.end())
    {
        position = m_tokenizedInputSequence.begin();
    }
    if(position == m_tokenizedInputSequence.end())
    {
        return;
    }
    auto prevPosition = position;
    for(++position; position != m_tokenizedInputSequence.end(); ++position)
    {
        const Token & left = *prevPosition;
        const Token & right = *position;
        TokenPair tokenPair(left, right);
        auto & countAndPosition = m_tokenPairCountAndPositions[tokenPair];
        countAndPosition.count++;
        countAndPosition.positions.emplace(prevPosition);
        prevPosition = position;
    }
}

void Tokenizer::ApplyExistingTokenPairReplacements()
{
    TIMER;
    auto position = m_tokenizedInputSequence.begin();
    if(position == m_tokenizedInputSequence.end() || m_tokenPairReplacements.empty())
    {
        return;
    }
    auto prevPosition = position;
    for(++position; position != m_tokenizedInputSequence.end();)
    {
        const Token & left = *prevPosition;
        const Token & right = *position;
        TokenPair tokenPair(left, right);
        auto replacementIt = m_tokenPairReplacements.find(tokenPair);
        if(replacementIt != m_tokenPairReplacements.end())
        {
            const Token & replacement = replacementIt->second;
            position = prevPosition = m_tokenizedInputSequence.ReplaceTwo(prevPosition, replacement);
            auto countAndPositionIter = m_tokenPairCountAndPositions.find(tokenPair);
            if(countAndPositionIter != m_tokenPairCountAndPositions.end())
            {
                countAndPositionIter->second.count--;
                if(countAndPositionIter->second.count <= 0)
                {
                    m_tokenPairCountAndPositions.erase(countAndPositionIter);
                }
                else
                {
                    auto & positions = countAndPositionIter->second.positions;
                    auto posIt = std::find(positions.begin(), positions.end(), position);
                    if(posIt != positions.end())
                    {
                        positions.erase(posIt); 
                    }
                }
            }
        }
        else
        {
            prevPosition = position;
        }
        ++position;
    }
}

void Tokenizer::OptimizeTokenLength()
{
    TIMER;
    if(m_tokenPairCountAndPositions.empty())
    {
        return;
    }
    struct Candidate
    {
        size_t count;
        uint64_t version;
        TokenPair pair;
    };
    struct CandidateLess
    {
        bool operator()(const Candidate& a, const Candidate& b) const
        {
            return a.count < b.count;
        }
    };
    std::priority_queue<Candidate, std::vector<Candidate>, CandidateLess> candidates;
    std::unordered_map<TokenPair, uint64_t, TokenPair::HashFunction> versions;
    auto QueueCandidate = [&](const TokenPair& pair, const CountAndPosition& data)
    {
        const auto version = ++versions[pair];
        if(!data.positions.empty())
        {
            candidates.push({data.count, version, pair});
        }
    };
    for(const auto& [pair, data] : m_tokenPairCountAndPositions)
    {
        if(!data.positions.empty())
        {
            QueueCandidate(pair, data);
        }
    }
    auto RemoveTokenPairSequencePosition = [&](LinkedList<Token>::Iterator position)
    {
        if(position == m_tokenizedInputSequence.end())
        {
            return;
        }
        auto nextPosition = position;
        ++nextPosition;
        if(nextPosition == m_tokenizedInputSequence.end())
        {
            return;
        }
        auto & left = *position;
        auto & right = *nextPosition;
        const TokenPair pair(left, right);
        auto & countAndPosition = m_tokenPairCountAndPositions[pair];
        --countAndPosition.count;
        auto & positions = countAndPosition.positions;
        positions.erase(position);
        QueueCandidate(pair, countAndPosition);
    };
    auto AddTokenPairSequencePosition = [&](LinkedList<Token>::Iterator position)
    {
        if(position == m_tokenizedInputSequence.end())
        {
            return;
        }
        auto nextPosition = position;
        ++nextPosition;
        if(nextPosition == m_tokenizedInputSequence.end())
        {
            return;
        }
        auto & left = *position;
        auto & right = *nextPosition;
        const TokenPair pair(left, right);
        auto & countAndPosition = m_tokenPairCountAndPositions[pair];
        ++countAndPosition.count;
        auto & positions = countAndPosition.positions;
        positions.emplace(position);
        QueueCandidate(pair, countAndPosition);
    };
    // todo: think of what this should be
    while(m_tokenPairReplacements.size() + 512 < Configuration::TokenDictionary)
    {
        while(!candidates.empty())
        {
            const auto& candidate = candidates.top();
            const auto current = m_tokenPairCountAndPositions.find(candidate.pair);
            const auto version = versions.find(candidate.pair);
            if(current == m_tokenPairCountAndPositions.end() ||
               current->second.positions.empty() ||
               version == versions.end() || version->second != candidate.version)
            {
                candidates.pop();
                continue;
            }
            break;
        }
        if(candidates.empty())
        {
            break;
        }
        const auto tokenPair = candidates.top().pair;
        candidates.pop();
        const auto & left = tokenPair.left;
        const auto & right = tokenPair.right;
        auto replacementIt = m_tokenPairReplacements.find(tokenPair);
        if(replacementIt == m_tokenPairReplacements.end())
        {
            Token newToken = m_nextUnusedToken++;
            m_tokenPairReplacements[tokenPair] = newToken;
            m_tokenReplacements.emplace(newToken, tokenPair);
        }
        else
        {
            // an existing replacement for this token pair already exists, weird, but let's use it
        }
        const auto & replacement = m_tokenPairReplacements[tokenPair];
        while(true)
        {
            auto selectedPair = m_tokenPairCountAndPositions.find(tokenPair);
            if(selectedPair == m_tokenPairCountAndPositions.end() || selectedPair->second.positions.empty())
            {
                break;
            }
            auto position = *selectedPair->second.positions.begin();
            auto nextPosition = position;
            ++nextPosition;
            if(nextPosition != m_tokenizedInputSequence.end() && *position == left && *nextPosition == right)
            {
                auto prevPosition = position;
                --prevPosition;
                auto nextNextPosition = nextPosition;
                ++nextNextPosition;
                RemoveTokenPairSequencePosition(prevPosition);
                RemoveTokenPairSequencePosition(position);
                RemoveTokenPairSequencePosition(nextPosition);
                position = m_tokenizedInputSequence.ReplaceTwo(position, replacement);
                AddTokenPairSequencePosition(prevPosition);
                AddTokenPairSequencePosition(position);
            }
        }
    }
}

void Tokenizer::CalculateTokenRelationships()
{
    TIMER;
    // process all the tokens
    for(auto position = m_tokenizedInputSequence.begin();position != m_tokenizedInputSequence.end(); ++position)
    {
        const auto token = *position;
        auto & allRelations = m_tokenRelationships[token];
        auto depthPosition = position;
        for(size_t depth=0; depth<Configuration::RelationDepth; ++depth)
        {
            --depthPosition;
            if(depthPosition == m_tokenizedInputSequence.end())
            {
                break;
            }
            auto & relation = allRelations[depth];
            relation[*depthPosition]++;
        }
    }
}

std::string Tokenizer::GetTokenTextSequence(const Token & token) const
{
    TIMER;
    std::string text;
    std::vector<Token> pending{token};
    while(!pending.empty())
    {
        const Token current = pending.back();
        pending.pop_back();

        if(current.IsCharacter())
        {
            if(current.IsSeparator() && !text.empty())
            {
                text.push_back(' ');
            }
            text.push_back(current.GetCharacter());
            continue;
        }

        const auto expansion = m_tokenReplacements.find(current);
        if(expansion == m_tokenReplacements.end())
        {
            throw std::logic_error("Tokenizer cannot expand a token without a replacement pair");
        }
        pending.push_back(expansion->second.right);
        pending.push_back(expansion->second.left);
    }
    return text;
}

namespace
{
    template<size_t BLOB_SIZE = 1024>
    class SequenceStore
    {
    public:
        SequenceStore(SQLite::DB &db, const std::string & table)
            : m_statement(db.CompileStatement("INSERT INTO " + table + "(SequenceID, Data) VALUES(NULL, ?);"))
        {}

        // bool GetDataCallback(RingBuffer &)
        // should: emplace_back data and return true,
        //         or return false if no more data available;
        template <typename GetDataCallback>
        void Store(GetDataCallback&& GetData)
        {
            RingBuffer<unsigned char> data;
            data.Reserve(BLOB_SIZE*8);
            while((data.Size()>=BLOB_SIZE) || GetData(data))
            {
                if(data.Size()>=BLOB_SIZE)
                {
                    m_statement.Bind(1,data.GetData(BLOB_SIZE),BLOB_SIZE);
                    m_statement.ExecDML();
                    data.EraseFront(BLOB_SIZE);
                }
            }
            if(!data.Empty())
            {
                m_statement.Bind(1,data.GetData(data.Size()),data.Size());
                m_statement.ExecDML();
            }
        }
    private:
        SQLite::Statement m_statement;
    };

    class SequenceReader
    {
    public:
        SequenceReader(SQLite::DB& db, const std::string& table)
            : m_query(db.ExecQuery("SELECT Data FROM " + table + " ORDER BY SequenceID;"))
        {}

        template<typename T>
        bool Read(T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            auto* destination = reinterpret_cast<unsigned char*>(&value);
            size_t copied = 0;
            while(copied < sizeof(T))
            {
                if(m_position == m_data.size())
                {
                    if(m_query.IsEOF())
                    {
                        if(copied != 0)
                        {
                            throw std::runtime_error("Tokenizer database contains a truncated sequence");
                        }
                        return false;
                    }
                    m_data = m_query.GetBlobField(0);
                    m_position = 0;
                    m_query.NextRow();
                    continue;
                }

                const size_t chunkSize = std::min(sizeof(T) - copied, m_data.size() - m_position);
                std::memcpy(destination + copied, m_data.data() + m_position, chunkSize);
                m_position += chunkSize;
                copied += chunkSize;
            }
            return true;
        }

    private:
        SQLite::Query m_query;
        std::vector<unsigned char> m_data;
        size_t m_position = 0;
    };
}

void Tokenizer::StoreTokenizerToDB(SQLite::DB & db) const
{
    {
        TIMER;
        db.ExecDML("DROP TABLE IF EXISTS Settings;");
        db.ExecDML("DROP TABLE IF EXISTS TokenizedInputSequence;");
        db.ExecDML("DROP TABLE IF EXISTS TokenPairReplacements;");
        db.ExecDML("DROP TABLE IF EXISTS TokenRelations;");
        db.ExecDML("DROP TABLE IF EXISTS FunctionTimings;");

        db.ExecDML("CREATE TABLE Settings(Setting TEXT PRIMARY KEY, Value INTEGER NOT NULL);");
        db.ExecDML("CREATE TABLE TokenizedInputSequence(SequenceID INTEGER PRIMARY KEY, Data BLOB NOT NULL);");
        db.ExecDML("CREATE TABLE TokenPairReplacements(SequenceID INTEGER PRIMARY KEY, Data BLOB NOT NULL);");
        db.ExecDML("CREATE TABLE TokenRelations(SequenceID INTEGER PRIMARY KEY, Data BLOB NOT NULL);");
        db.ExecDML("CREATE TABLE FunctionTimings(Function TEXT PRIMARY KEY, Count INTEGER NOT NULL, Duration INTEGER NOT NULL);");

        db.BeginTransaction();

        // Store settings
        auto settingStatement = db.CompileStatement("INSERT INTO Settings(Setting, Value) VALUES(?, ?);");
        auto addSetting = [&]<typename T>(const std::string& setting, const T& value)
        {
            settingStatement.Bind(1, setting);
            settingStatement.Bind(2, value);
            settingStatement.ExecDML();
        };
        addSetting("Version", Configuration::Version);
        addSetting("RelationDepth", Configuration::RelationDepth);
        addSetting("TokenDictionary", Configuration::TokenDictionary);
        addSetting("NextToken", static_cast<unsigned long>(m_nextUnusedToken.GetToken())); 

        auto StorePOD = []<typename T>(RingBuffer<unsigned char> & data, const T & pod)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            size_t s = sizeof(T);
            const unsigned char *p = reinterpret_cast<const unsigned char *>(&pod);
            for(size_t i=0;i<s;++i)
            {
                data.PushBack(p[i]);
            }
        };

        // Store m_tokenizedInputSequence
        // LinkedList<Token> m_tokenizedInputSequence;
        SequenceStore(db, "TokenizedInputSequence").Store(
            [pos = m_tokenizedInputSequence.begin(), this, &StorePOD]
            (RingBuffer<unsigned char> & data) mutable -> bool
        {
            if(pos == m_tokenizedInputSequence.end())
            {
                return false;
            }
            StorePOD(data, *pos);
            ++pos;
            return true;
        });

        // Store m_tokenPairReplacements
        // std::unordered_map<TokenPair,Token,TokenPair::HashFunction> m_tokenPairReplacements;
        SequenceStore(db, "TokenPairReplacements").Store(
            [pos = m_tokenPairReplacements.begin(), this, &StorePOD]
            (RingBuffer<unsigned char> & data) mutable -> bool
        {
            if(pos == m_tokenPairReplacements.end())
            {
                return false;
            }
            StorePOD(data, pos->first.left);
            StorePOD(data, pos->first.right);
            StorePOD(data, pos->second);
            ++pos;
            return true;
        });

        // Store each relationship as Token, depth, connected token, and count.
        // std::unordered_map<Token,std::array<std::unordered_map<Token,size_t,Token::HashFunction>,Configuration::RelationDepth>,Token::HashFunction> m_tokenRelationships;
        if(!m_tokenRelationships.empty())
        {
            SequenceStore(db, "TokenRelations").Store(
                [ tokenIter = m_tokenRelationships.begin()
                , depthIter = m_tokenRelationships.begin()->second.begin()
                , this, &StorePOD]
                (RingBuffer<unsigned char> & data) mutable -> bool
            {
                if(tokenIter == m_tokenRelationships.end())
                {
                    return false;
                }
                if(depthIter == tokenIter->second.end())
                {
                    ++tokenIter;
                    if(tokenIter == m_tokenRelationships.end())
                    {
                        return false;
                    }
                    depthIter = tokenIter->second.begin();
                }
                if(depthIter == tokenIter->second.begin())
                {
                    StorePOD(data, tokenIter->first);
                }
                const auto &relations = *depthIter;
                StorePOD(data, relations.size());
                for(const auto & relation : relations)
                {
                    StorePOD(data, relation.first);
                    StorePOD(data, relation.second);
                }
                ++depthIter;
                return true;
            });
        }

        db.CommitTransaction();
    }

    // Store timings
    auto settingStatement = db.CompileStatement("INSERT INTO FunctionTimings(Function, Count, Duration) VALUES(?, ?, ?);");
    for(const auto & t : m_functionRuntimes)
    {
        settingStatement.Bind(1, t.first);
        settingStatement.Bind(2, t.second.first);
        settingStatement.Bind(3, t.second.second);
        settingStatement.ExecDML();
    }
}

void Tokenizer::LoadDataFromDB(SQLite::DB & db)
{
    TIMER;
    auto ReadSetting = [&](const std::string& setting)
    {
        return db.ExecSingleInt64("SELECT Value FROM Settings WHERE Setting='" + setting + "';");
    };
    auto RequireCompatibleSetting = [&](const std::string& setting, size_t expected)
    {
        if(ReadSetting(setting) != static_cast<long long>(expected))
        {
            throw std::runtime_error("Tokenizer database has incompatible setting: " + setting);
        }
    };

    RequireCompatibleSetting("Version", Configuration::Version);
    RequireCompatibleSetting("RelationDepth", Configuration::RelationDepth);
    RequireCompatibleSetting("TokenDictionary", Configuration::TokenDictionary);
    const auto nextToken = ReadSetting("NextToken");
    if(nextToken < 0 || nextToken > Token::Root)
    {
        throw std::runtime_error("Tokenizer database contains an invalid next token value");
    }

    m_tokenizedInputSequence.Clear();
    m_tokenPairCountAndPositions.clear();
    m_tokenPairReplacements.clear();
    m_tokenReplacements.clear();
    m_tokenRelationships.clear();
    m_nextUnusedToken.SetToken(static_cast<Token::Data>(nextToken));

    SequenceReader sequenceReader(db, "TokenizedInputSequence");
    Token::Data sequenceToken;
    while(sequenceReader.Read(sequenceToken))
    {
        m_tokenizedInputSequence.EmplaceBack(Token(sequenceToken));
    }

    SequenceReader replacementReader(db, "TokenPairReplacements");
    Token::Data left;
    Token::Data right;
    Token::Data replacement;
    while(replacementReader.Read(left))
    {
        if(!replacementReader.Read(right) || !replacementReader.Read(replacement))
        {
            throw std::runtime_error("Tokenizer database contains a truncated replacement sequence");
        }
        const TokenPair tokenPair{Token(left), Token(right)};
        const Token replacementToken(replacement);
        m_tokenPairReplacements.emplace(tokenPair, replacementToken);
        m_tokenReplacements.emplace(replacementToken, tokenPair);
    }

    SequenceReader relationReader(db, "TokenRelations");
    Token::Data relationToken;
    while(relationReader.Read(relationToken))
    {
        auto& relationsByDepth = m_tokenRelationships[Token(relationToken)];
        for(size_t depth = 0; depth < Configuration::RelationDepth; ++depth)
        {
            size_t relationCount;
            if(!relationReader.Read(relationCount))
            {
                throw std::runtime_error("Tokenizer database contains a truncated relationship sequence");
            }

            auto& relations = relationsByDepth[depth];
            for(size_t i = 0; i < relationCount; ++i)
            {
                Token::Data connectedToken;
                size_t count;
                if(!relationReader.Read(connectedToken) || !relationReader.Read(count))
                {
                    throw std::runtime_error("Tokenizer database contains a truncated relationship sequence");
                }
                relations[Token(connectedToken)] = count;
            }
        }
    }

    UpdateTokenPairCountAndPositions(m_tokenizedInputSequence.begin());
}
