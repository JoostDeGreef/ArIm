#pragma once

#include <string>
#include <vector>

#include "sqlite3.h"

#include "../SQLiteStatement.h"
#include "../SQLiteSupport.h"

using namespace SQLite;

class Statement::State
{
public:
    State(DatabaseHandle db, StatementHandle statement,bool eof,int columnCount = -1)
        : m_db(db)
        , m_statement(statement)
        , m_eof(eof)
        , m_columnCount(columnCount)
    {
        if (-1 == m_columnCount && m_statement)
        {
            m_columnCount = sqlite3_column_count(m_statement.get());
        }
    }
    ~State() = default;

    void Check()
    {
        if (!m_db)
        {
            ThrowError("Database not open");
        }
        if (!m_statement)
        {
            ThrowError("Statement not active");
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

    void Reset()
    {
        if (m_statement)
        {
            int ret = sqlite3_reset(m_statement.get());
            ThrowErrorIfNotOK(m_db.get(), ret);
        }
    }

    DatabaseHandle m_db;
    StatementHandle m_statement;
    bool m_eof;
    int m_columnCount;
};


