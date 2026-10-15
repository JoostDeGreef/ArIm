#pragma once

class Statement::State
{
public:
    State(sqlite3* db, sqlite3_stmt* statement,bool eof,int columnCount = -1)
        : m_db(db)
        , m_statement(statement)
        , m_eof(eof)
        , m_columnCount(columnCount)
    {
        if (-1 == m_columnCount && nullptr != m_statement)
        {
            m_columnCount = sqlite3_column_count(m_statement);
        }
    }
    ~State()
    {
        Finalize();
    }

    void Check()
    {
        if (nullptr == m_db)
        {
            ThrowError("Database not open");
        }
        if (nullptr == m_statement)
        {
            ThrowError("Statement not active");
        }
    }

    void Finalize()
    {
        if (m_statement)
        {
            int ret = sqlite3_finalize(m_statement);
            m_statement = nullptr;
            ThrowErrorIfNotOK(m_db, ret);
        }
    }

    void Reset()
    {
        if (m_statement)
        {
            int ret = sqlite3_reset(m_statement);
            ThrowErrorIfNotOK(m_db, ret);
        }
    }


    sqlite3_stmt* m_statement;
    sqlite3* m_db;
    bool m_eof;
    int m_columnCount;
};


