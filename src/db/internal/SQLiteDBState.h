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
        : m_db(nullptr)
        , m_busyTimeoutMs(6000)
    {}

    ~State()
    {
        if (m_db)
        {
            sqlite3_close_v2(m_db);
            m_db = nullptr;
        }
    }

    void Check()
    {
        if (nullptr == m_db)
        {
            ThrowError("DB not opened");
        }
    }

    sqlite3_stmt* Compile(const std::string& sql)
    {
        Check();

        const char* szTail = 0;
        sqlite3_stmt* statement;

        int ret = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &statement, &szTail);

        ThrowErrorIfNotOK(m_db, ret);

        return statement;
    }

    void Close()
    {
        if (m_db)
        {
            int ret = sqlite3_close_v2(m_db);
            ThrowErrorIfNotOK(m_db, ret);
            m_db = nullptr;
        }
    }

    sqlite3* m_db;
    int m_busyTimeoutMs;
};

