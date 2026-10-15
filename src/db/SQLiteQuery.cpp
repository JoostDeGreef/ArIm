#include <memory>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

#include "sqlite3.h"
#include "SQLiteDB.h"

using namespace SQLite;

#include "SQLiteSupport.h"
#include "SQLiteQueryState.h"

Query::Query()
    : m_state(make_shared<State>(nullptr,nullptr,true))
{}

Query::Query(std::shared_ptr<State>&& state)
    : m_state(state)
{}

Query::Query(Query&& query)
    : m_state(query.m_state)
{}

Query::Query(const Query& query)
    : m_state(query.m_state)
{}

Query::~Query()
{}

Query& Query::operator = (const Query& query)
{
    m_state = query.m_state;
    return *this;
}

bool Query::IsEOF() const
{
    return m_state->m_eof;
}

int Query::GetFieldCount() const
{
    return m_state->m_columnCount;
}

const std::vector<std::string>& Query::GetFieldNames() const
{
    if (m_state->m_columnCount != static_cast<int>(m_state->m_fieldNames.size()))
    {
        m_state->Check();
        m_state->m_fieldNames.resize(m_state->m_columnCount);
        for (int field = 0; field < m_state->m_columnCount; field++)
        {
            m_state->m_fieldNames[field] = sqlite3_column_name(m_state->m_statement, field);
            std::transform(m_state->m_fieldNames[field].begin(), m_state->m_fieldNames[field].end(), m_state->m_fieldNames[field].begin(), ::tolower);
        }
    }
    return m_state->m_fieldNames;
}

void Query::NextRow()
{
    m_state->Check();

    int ret = sqlite3_step(m_state->m_statement);

    if (ret == SQLITE_DONE)
    {
        // no rows
        m_state->m_eof = true;
    }
    else if (ret == SQLITE_ROW)
    {
        // more rows, nothing to do
    }
    else
    {
        Finalize();
        ThrowError(m_state->m_db, ret);
    }
}

void Query::Finalize()
{
    m_state->Finalize();
}

int Query::FieldIndex(const std::string& fieldName) const
{
    m_state->Check();
    string key(fieldName);
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    const std::vector<std::string>& fieldNames = GetFieldNames();
    auto index = std::find(fieldNames.begin(), fieldNames.end(), key);
    if (index == fieldNames.end())
    {
        ThrowError("Invalid field name requested");
    }
    return static_cast<int>(distance(fieldNames.begin(), index));
}

const std::string& Query::FieldName(int field) const
{
    m_state->CheckField(field);
    const std::vector<std::string>& fieldNames = GetFieldNames();
    return fieldNames[field];
}

std::string Query::FieldDeclType(int field) const
{
    m_state->Check();
    m_state->CheckField(field);

    const char* res = sqlite3_column_decltype(m_state->m_statement, field);
    return nullptr == res ? "NULL" : res;
}

DataType Query::FieldDataType(int field) const
{
    m_state->Check();
    m_state->CheckField(field);

    switch (sqlite3_column_type(m_state->m_statement, field))
    {
    case SQLITE_INTEGER: return DataType::Integer;
    case SQLITE_FLOAT: return DataType::Float;
    case SQLITE_BLOB: return DataType::Blob;
    case SQLITE_NULL: return DataType::Null;
    case SQLITE_TEXT: return DataType::Text;
    }

    ThrowError("Unknown DataType!?");
    return DataType::Null;
}

std::string Query::FieldValue(int field) const
{
    m_state->Check();
    m_state->CheckField(field);
    return (const char*)sqlite3_column_text(m_state->m_statement, field);
}

std::string Query::FieldValue(const std::string& field) const
{
    int index = FieldIndex(field);
    return FieldValue(index);
}

int Query::GetIntField(int field, int nullValue) const
{
    return (FieldDataType(field) == DataType::Null) ? nullValue : sqlite3_column_int(m_state->m_statement, field);    
}

int Query::GetIntField(const std::string& field, int nullValue) const
{
    int index = FieldIndex(field);
    return GetIntField(index, nullValue);
}

int64_t Query::GetInt64Field(int field, long long nullValue) const
{
    return (FieldDataType(field) == DataType::Null) ? nullValue : sqlite3_column_int64(m_state->m_statement, field);
}

int64_t Query::GetInt64Field(const std::string& field, long long nullValue) const
{
    int index = FieldIndex(field);
    return GetInt64Field(index, nullValue);
}

double Query::GetFloatField(int field, double nullValue) const
{
    return (FieldDataType(field) == DataType::Null) ? nullValue : sqlite3_column_double(m_state->m_statement, field);
}

double Query::GetFloatField(const std::string& field, double nullValue) const
{
    int index = FieldIndex(field);
    return GetFloatField(index, nullValue);
}

std::string Query::GetStringField(int field, const std::string& nullValue) const
{
    return (FieldDataType(field) == DataType::Null) ? nullValue : (const char *)(sqlite3_column_text(m_state->m_statement, field));
}

std::string Query::GetStringField(const std::string& field, const std::string& nullValue) const
{
    int index = FieldIndex(field);
    return GetStringField(index, nullValue);
}

std::vector<unsigned char> Query::GetBlobField(int field) const
{
    std::vector<unsigned char> res;
    if (FieldDataType(field) != DataType::Null)
    {
        int length = sqlite3_column_bytes(m_state->m_statement, field);
        auto blob = (const unsigned char*)sqlite3_column_blob(m_state->m_statement, field);
        res.reserve(length);
        res.insert(res.end(), &blob[0], &blob[length]);
    }
    return res;
}

std::vector<unsigned char> Query::GetBlobField(const std::string& field) const
{
    int index = FieldIndex(field);
    return GetBlobField(index);
}

bool Query::FieldIsNull(int field) const
{
    return (FieldDataType(field) != DataType::Null);
}

bool Query::FieldIsNull(const std::string& field) const
{
    int index = FieldIndex(field);
    return FieldIsNull(index);
}


