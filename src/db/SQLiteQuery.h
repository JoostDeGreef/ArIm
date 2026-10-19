#pragma once

#include <cinttypes>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace SQLite
{
    class Rows;

    class Query
    {
    public:
        class State;
    public:
        Query();
        Query(std::shared_ptr<State>&& state);
        Query(Query&& query);
        Query(const Query& query);
        ~Query();

        Query& operator = (const Query& query);
        Query& operator = (Query&& query);

        int GetFieldCount() const;
        const std::vector<std::string>& GetFieldNames() const;
        int FieldIndex(const std::string& fieldName) const;
        const std::string& FieldName(int field) const;
        std::string FieldDeclType(int field) const;
        DataType FieldDataType(int field) const;
        std::string FieldValue(int field) const;
        std::string FieldValue(const std::string& field) const;
        int GetIntField(int field, int nullValue = 0) const;
        int GetIntField(const std::string& field, int nullValue = 0) const;
        int64_t GetInt64Field(int field, long long nullValue = 0) const;
        int64_t GetInt64Field(const std::string& field, long long nullValue = 0) const;
        double GetFloatField(int field, double nullValue = 0.0) const;
        double GetFloatField(const std::string& field, double nullValue = 0.0) const;
        std::string GetStringField(int field, const std::string& nullValue = "") const;
        std::string GetStringField(const std::string& field, const std::string& nullValue = "") const;
        std::vector<unsigned char> GetBlobField(int field) const;
        std::vector<unsigned char> GetBlobField(const std::string& field) const;
        bool FieldIsNull(int field) const;
        bool FieldIsNull(const std::string& field) const;
        bool IsEOF() const;
        void NextRow();
        void Finalize();

        Rows GetRows();
    private:
        std::shared_ptr<State> m_state;
    };

    class Rows
    {
    public:
        explicit Rows(Query&& query) : m_query(std::move(query)) {}
        Rows(const Rows&) = delete;
        Rows& operator=(const Rows&) = delete;
        Rows(Rows&&) = default;
        Rows& operator=(Rows&&) = default;
        ~Rows() { m_query.Finalize(); }

        class Iterator
        {
        public:
            explicit Iterator(Query* query) : m_query(query) {}
            Query& operator*() const { return *m_query; }
            Query* operator->() const { return m_query; }
            Iterator& operator++() { m_query->NextRow(); return *this; }
            bool operator!=(const Iterator& other) const
            { return m_query != other.m_query && (m_query == nullptr || !m_query->IsEOF()); }
        private:
            Query* m_query;
        };

        Iterator begin() { return m_query.IsEOF() ? Iterator(nullptr) : Iterator(&m_query); }
        Iterator end() { return Iterator(nullptr); }
    private:
        Query m_query;
    };

} // namespace SQLite

