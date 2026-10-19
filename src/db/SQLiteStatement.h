#pragma once

#include <memory>
#include <string>

namespace SQLite
{
    class Statement
    {
    public:
        class State;
    public:
        Statement();
        Statement(std::shared_ptr<State>&& state);
        Statement(Statement&& statement);
        Statement(const Statement& statement);
        ~Statement();

        Statement& operator = (const Statement& statement);
        Statement& operator = (Statement&& statement);

        int ExecDML();
        Query ExecQuery();
        void Bind(int param, const std::string& value);
        void Bind(int param, const char* value, const int length = -1);
        void Bind(int param, const int value);
        void Bind(int param, const long long value);
        void Bind(int param, const double value);
        void Bind(int param, const unsigned char* value, int length);
        void BindNull(int param);
        void Reset();
        void Finalize();
    private:
        std::shared_ptr<State> m_state;
    };
} // namespace SQLite

