#pragma once

#include <string>
#include <vector>

#include "sqlite3.h"

#include "../SQLiteQuery.h"
#include "../SQLiteSupport.h"

using namespace sqlite;

class Query::State
{
public:
    State(DatabaseHandle db, StatementHandle statement, bool eof,int columnCount = -1)
        : m_fieldNames()
        , m_db(db)
        , m_statement(statement)
        , m_eof(eof)
        , m_columnCount(columnCount)
    {
        if (-1 == m_columnCount && m_statement)
        {
            m_columnCount = sqlite3_column_count(m_statement.get());
        }
        else if (-1 == m_columnCount)
        {
            m_columnCount = 0;
        }
    }
    ~State() = default;

    void Check()
    {
        if (!m_statement)
        {
            ThrowError("Statement not active");
        }
    }
    void CheckField(int field)
    {
        if (field < 0 || field > m_columnCount - 1)
        {
            ThrowError("Invalid field index requested");
        }
    }

    void Finalize()
    {
        if (m_statement)
        {
            m_statement.reset();
        }
        m_db.reset();
    }

    std::vector<std::string> m_fieldNames;
    DatabaseHandle m_db;
    StatementHandle m_statement;
    bool m_eof;
    int m_columnCount;
};


