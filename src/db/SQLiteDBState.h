#pragma once

class DB::State
{
public:
    State()
        : m_db(nullptr)
        , m_busyTimeoutMs(6000)
    {}

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
            sqlite3_close(m_db);
            m_db = nullptr;
        }
    }

    sqlite3* m_db;
    int m_busyTimeoutMs;
};

