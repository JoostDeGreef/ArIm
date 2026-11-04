#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "Tokenizer.h"

namespace
{
    template<size_t BLOB_SIZE = 1024>
    class SequenceStore
    {
    public:
        SequenceStore(sqlite::DB &db, const std::string & table)
            : m_statement(db.CompileStatement("INSERT INTO " + table + "(SequenceID, Data) VALUES(NULL, ?);"))
        {}

        // bool GetDataCallback(std::vector &)
        // should: emplace_back data and return true,
        //         or return false if no more data available;
        template <typename GetDataCallback>
        void Store(GetDataCallback&& GetData)
        {
            std::vector<unsigned char> data;
            data.reserve(BLOB_SIZE+256);
            while((data.size()>=BLOB_SIZE) || GetData(data))
            {
                if(data.size()>=BLOB_SIZE)
                {
                    m_statement.Bind(1,data.data(),BLOB_SIZE);
                    m_statement.ExecDML();
                    data.erase(data.begin()+0,data.begin()+BLOB_SIZE);
                }
            }
            if(!data.empty())
            {
                m_statement.Bind(1,data.data(),data.size());
                m_statement.ExecDML();
            }
        }
    private:
        sqlite::Statement m_statement;
    };

    class SequenceReader
    {
    public:
        SequenceReader(sqlite::DB& db, const std::string& table)
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
        sqlite::Query m_query;
        std::vector<unsigned char> m_data;
        size_t m_position = 0;
    };
}

void Tokenizer::StoreTokenizerToDB(sqlite::DB & db) const
{
    db.ExecDML("DROP TABLE IF EXISTS Settings;");
    db.ExecDML("DROP TABLE IF EXISTS TokenizedInputSequence;");
    db.ExecDML("DROP TABLE IF EXISTS TokenPairCountAndPositions;");
    db.ExecDML("DROP TABLE IF EXISTS TokenPairReplacements;");
    db.ExecDML("DROP TABLE IF EXISTS TokenRelations;");

    db.ExecDML("CREATE TABLE Settings(Setting TEXT PRIMARY KEY, Value INTEGER NOT NULL);");
    db.ExecDML("CREATE TABLE TokenizedInputSequence(SequenceID INTEGER PRIMARY KEY, Data BLOB NOT NULL);");
    db.ExecDML("CREATE TABLE TokenPairReplacements(SequenceID INTEGER PRIMARY KEY, Data BLOB NOT NULL);");
    db.ExecDML("CREATE TABLE TokenRelations(SequenceID INTEGER PRIMARY KEY, Data BLOB NOT NULL);");

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
    addSetting("NextToken", static_cast<unsigned long>(m_nextUnusedToken.GetData()));

    auto StorePOD = []<typename T>(std::vector<unsigned char> & data, const T & pod)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        size_t s = sizeof(T);
        const unsigned char *p = reinterpret_cast<const unsigned char *>(&pod);
        for(size_t i=0;i<s;++i)
        {
            data.emplace_back(p[i]);
        }
    };

    // Store m_tokenizedInputSequence
    // LinkedList<Token> m_tokenizedInputSequence;
    SequenceStore(db, "TokenizedInputSequence").Store(
        [pos = m_tokenizedInputSequence.begin(), this, &StorePOD]
        (std::vector<unsigned char> & data) mutable -> bool
    {
        if(pos == m_tokenizedInputSequence.end())
        {
            return false;
        }
        const Token::Data tokenData = pos->GetData();
        StorePOD(data, tokenData);
        ++pos;
        return true;
    });

    // Store m_tokenPairReplacements
    // std::unordered_map<TokenPair,Token,TokenPair::HashFunction> m_tokenPairReplacements;
    SequenceStore(db, "TokenPairReplacements").Store(
        [pos = m_tokenPairReplacements.begin(), this, &StorePOD]
        (std::vector<unsigned char> & data) mutable -> bool
    {
        if(pos == m_tokenPairReplacements.end())
        {
            return false;
        }
        const Token::Data left = pos->first.left.GetData();
        const Token::Data right = pos->first.right.GetData();
        const Token::Data replacement = pos->second.GetData();
        StorePOD(data, left);
        StorePOD(data, right);
        StorePOD(data, replacement);
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
            (std::vector<unsigned char> & data) mutable -> bool
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
                StorePOD(data, tokenIter->first.GetData());
            }
            const auto &relations = *depthIter;
            StorePOD(data, relations.size());
            for(const auto & relation : relations)
            {
                StorePOD(data, relation.first.GetData());
                StorePOD(data, relation.second);
            }
            ++depthIter;
            return true;
        });
    }

    db.CommitTransaction();

}

void Tokenizer::LoadDataFromDB(sqlite::DB & db)
{
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
    if(nextToken < 0 || nextToken > Token::NumberMask)
    {
        throw std::runtime_error("Tokenizer database contains an invalid next token value");
    }

    m_tokenizedInputSequence.Clear();
    m_tokenPairCountAndPositions.clear();
    m_tokenPairReplacements.clear();
    m_tokenReplacements.clear();
    m_tokenRelationships.clear();
    m_nextUnusedToken.SetData(static_cast<Token::Data>(nextToken));

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
