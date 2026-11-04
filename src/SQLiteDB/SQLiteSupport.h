#pragma once

#include <memory>

#include "sqlite3.h"
#include "SQLiteException.h"

namespace sqlite
{
    using DatabaseHandle = std::shared_ptr<sqlite3>;
    using StatementHandle = std::shared_ptr<sqlite3_stmt>;

    inline DatabaseHandle OwnDatabase(sqlite3* db)
    {
        return DatabaseHandle(db, [](sqlite3* value)
        {
            if (value)
            {
                sqlite3_close_v2(value);
            }
        });
    }

    inline StatementHandle OwnStatement(sqlite3_stmt* statement)
    {
        return StatementHandle(statement, [](sqlite3_stmt* value)
        {
            if (value)
            {
                sqlite3_finalize(value);
            }
        });
    }

    inline void ThrowError(const char* szError, int code = 42)
    {
        throw Exception(code, szError ? szError : "Unknown SQLite error");
    }

    // sqlite3_exec() allocates this error message; take ownership and free it.
    inline void ThrowError(char* szError, int code = 42)
    {
        const std::string message = szError ? szError : "Unknown SQLite error";
        sqlite3_free(szError);
        throw Exception(code, message);
    }

    inline void ThrowError(sqlite3* db, int code = 42)
    {
        const char* message = db ? sqlite3_errmsg(db) : sqlite3_errstr(code);
        throw Exception(code, message ? message : "Unknown SQLite error");
    }

    inline void ThrowErrorIfNotOK(const char* szError, int code)
    {
        if (SQLITE_OK != code)
        {
            ThrowError(szError, code);
        }
    }

    inline void ThrowErrorIfNotOK(char* szError, int code)
    {
        if (SQLITE_OK != code)
        {
            ThrowError(szError, code);
        }
        else
        {
            sqlite3_free(szError);
        }
    }

    inline void ThrowErrorIfNotOK(sqlite3* db, int code)
    {
        if (SQLITE_OK != code)
        {
            ThrowError(db, code);
        }
    }
} // namespace SQLite
