#pragma once

namespace SQLite
{
    inline void ThrowError(const char* szError,int code = 42)
    {
// sqlite3_free !
//        throw CppSQLite3Exception(code, (char*)szError, DONT_DELETE_MSG);
        throw exception();
    }
    inline void ThrowError(sqlite3* db, int code = 42)
    {
        const char* szError = sqlite3_errmsg(db);
//        throw CppSQLite3Exception(code, (char*)szError, DONT_DELETE_MSG);
        throw exception();
    }
    inline void ThrowErrorIfNotOK(const char* szError, int code)
    {
        if (SQLITE_OK != code)
        {
            ThrowError(szError, code);
        }
    }
    inline void ThrowErrorIfNotOK(sqlite3* db, int code)
    {
        if (SQLITE_OK != code)
        {
            ThrowError(db, code);
        }
    }   
};


