#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace SQLite
{
    class Exception : public std::runtime_error
    {
    public:
        Exception(int code, const std::string& message)
            : std::runtime_error(message)
            , m_code(code)
        {}

        int Code() const noexcept
        {
            return m_code;
        }

    private:
        int m_code;
    };
} // namespace SQLite
