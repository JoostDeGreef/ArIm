#pragma once

#include <string>
#include <vector>

#include "sqlite3.h"

#include "../SQLiteDB.h"
#include "../SQLiteSupport.h"

using namespace SQLite;

class DB::State
{
public:
    State()
        : m_db()
        , m_busyTimeoutMs(6000)
    {}

    ~State() = default;

    void Check()
    {
        if (!m_db)
        {
            ThrowError("DB not opened");
        }
    }

    StatementHandle Compile(const std::string& sql)
    {
        Check();

        const char* szTail = 0;
        sqlite3_stmt* rawStatement = nullptr;

        int ret = sqlite3_prepare_v2(m_db.get(), sql.c_str(), -1, &rawStatement, &szTail);
        StatementHandle statement = OwnStatement(rawStatement);

        ThrowErrorIfNotOK(m_db.get(), ret);

        return statement;
    }

    void Close()
    {
        m_db.reset();
    }

    DatabaseHandle m_db;
    int m_busyTimeoutMs;
};

