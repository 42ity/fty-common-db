#pragma once

#include <fmt/format.h>
#include <fty/string-utils.h>
#include <fty/traits.h>
#include <memory>
#include <optional>

namespace fty::db {

// =====================================================================================================================

template <typename T>
struct Arg
{
    std::string_view name;
    T                value;
    bool             isNull = false;
};

template <class, template <class> class>
struct is_instance : public std::false_type
{
};

template <class T, template <class> class U>
struct is_instance<U<T>, U> : public std::true_type
{
};

void shutdown();

// =====================================================================================================================

class Statement;
class ConstIterator;
class Rows;
class Row;

class Connection
{
public:
    Connection();
    ~Connection();
    Statement prepare(const std::string& sql);

public:
    template <typename... Args>
    Row selectRow(const std::string& queryStr, Args&&... args);

    template <typename... Args>
    Rows select(const std::string& queryStr, Args&&... args);

    template <typename... Args>
    uint execute(const std::string& queryStr, Args&&... args);

    int64_t lastInsertId();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    friend class Transaction;
};

// =====================================================================================================================

class Row
{
public:
    Row() = default;
    Row(const Row& other) = default;
    ~Row();

    template <typename T>
    T get(const std::string& col) const;

    std::string get(const std::string& col) const;

    template <typename T>
    void get(const std::string& name, T& val) const;

    Row& operator=(const Row& other) = default;

protected:
    std::string getString(const std::string& name) const;
    bool        getBool(const std::string& name) const;
    int8_t      getInt8(const std::string& name) const;
    uint8_t     getUint8(const std::string& name) const;
    int16_t     getInt16(const std::string& name) const;
    uint16_t    getUint16(const std::string& name) const;
    int32_t     getInt32(const std::string& name) const;
    uint32_t    getUint32(const std::string& name) const;
    int64_t     getInt64(const std::string& name) const;
    uint64_t    getUint64(const std::string& name) const;
    float       getFloat(const std::string& name) const;
    double      getDouble(const std::string& name) const;
    bool        isNull(const std::string& name) const;

private:
    struct Impl;
    std::shared_ptr<Impl> m_impl;
    Row(std::shared_ptr<Impl> impl);

    friend class Statement;
    friend class ConstIterator;
    friend class Rows;
};

// =====================================================================================================================

class Rows
{
public:
    Rows();
    Rows(const Rows& other) = default;
    ~Rows();

    ConstIterator begin() const;
    ConstIterator end() const;
    size_t        size() const;
    bool          empty() const;
    Row           operator[](size_t off) const;

private:
    struct Impl;
    std::shared_ptr<Impl> m_impl;
    Rows(std::shared_ptr<Impl> impl);
    friend class ConstIterator;
    friend class Statement;
};

// =====================================================================================================================

class ConstIterator : public std::iterator<std::random_access_iterator_tag, Row>
{
public:
    using ConstReference = const value_type&;
    using ConstPointer   = const value_type*;

private:
    Rows   m_rows;
    Row    m_current;
    size_t m_offset;

    void setOffset(size_t off);

public:
    ConstIterator(const Rows& r, size_t off);
    bool            operator==(const ConstIterator& it) const;
    bool            operator!=(const ConstIterator& it) const;
    ConstIterator&  operator++();
    ConstIterator   operator++(int);
    ConstIterator   operator--();
    ConstIterator   operator--(int);
    ConstReference  operator*() const;
    ConstPointer    operator->() const;
    ConstIterator&  operator+=(difference_type n);
    ConstIterator   operator+(difference_type n) const;
    ConstIterator&  operator-=(difference_type n);
    ConstIterator   operator-(difference_type n) const;
    difference_type operator-(const ConstIterator& it) const;
};

// =====================================================================================================================

class Statement
{
public:
    ~Statement();

    template <typename T>
    Statement& bind(const std::string& name, const T& value);

    template <typename TArg, typename... TArgs>
    Statement& bind(TArg&& val, TArgs&&... args);

    template <typename TArg>
    Statement& bind(Arg<TArg>&& arg);

    template <typename TArg, typename... TArgs>
    Statement& bindMulti(size_t count, TArg&& val, TArgs&&... args);

    template <typename TArg>
    Statement& bindMulti(size_t count, Arg<TArg>&& arg);

    Statement& bind();

protected:
    void set(const std::string& name, const std::string& val);
    void set(const std::string& name, bool val);
    void set(const std::string& name, int8_t val);
    void set(const std::string& name, uint8_t val);
    void set(const std::string& name, int16_t val);
    void set(const std::string& name, uint16_t val);
    void set(const std::string& name, int32_t val);
    void set(const std::string& name, uint32_t val);
    void set(const std::string& name, int64_t val);
    void set(const std::string& name, uint64_t val);
    void set(const std::string& name, float val);
    void set(const std::string& name, double val);
    void setNull(const std::string& name);

public:
    Row  selectRow() const;
    Rows select() const;
    uint execute() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    Statement(std::unique_ptr<Impl> impl);
    friend class Connection;
};

// =====================================================================================================================

class Transaction
{
public:
    Transaction(Connection& con);
    ~Transaction();

    void commit();
    void rollback();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// =====================================================================================================================

class NotFound: public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

// =====================================================================================================================

inline std::string multiInsert(std::initializer_list<std::string> cols, size_t count)
{
    std::vector<std::string> colsStr;
    for (const auto& col : cols) {
        colsStr.push_back(":" + col + "_{0}");
    }

    std::string out;
    for (size_t i = 0; i < count; ++i) {
        out += (i > 0 ? ", " : "") + fmt::format("(" + fty::implode(colsStr, ", ") + ")", i);
    }
    return out;
}

} // namespace tnt

// =====================================================================================================================
// Arguments
// =====================================================================================================================

namespace fty::db::internal {

struct Arg
{
    std::string_view str;

    template <typename T>
    auto operator=(T&& value) const
    {
        if constexpr (is_instance<T, std::optional>::value) {
            if (value) {
                return fty::db::Arg<typename T::value_type>{str, std::forward<typename T::value_type>(*value)};
            } else {
                return fty::db::Arg<typename T::value_type>{str, typename T::value_type{}, true};
            }
        } else {
            return fty::db::Arg<T>{str, std::forward<T>(value)};
        }
    }
};

} // namespace fty::db::internal

constexpr fty::db::internal::Arg operator"" _p(const char* s, size_t n)
{
    return {{s, n}};
}

template <typename T>
std::optional<std::decay_t<T>> nullable(bool cond, T&& value)
{
    if (cond) {
        return std::optional<std::decay_t<T>>(value);
    }
    return std::nullopt;
}

// =====================================================================================================================
// Connection impl
// =====================================================================================================================

template <typename... Args>
inline fty::db::Row fty::db::Connection::selectRow(const std::string& queryStr, Args&&... args)
{
    return prepare(queryStr).bind(std::forward<Args>(args)...).selectRow();
}

template <typename... Args>
inline fty::db::Rows fty::db::Connection::select(const std::string& queryStr, Args&&... args)
{
    return prepare(queryStr).bind(std::forward<Args>(args)...).select();
}

template <typename... Args>
inline uint fty::db::Connection::execute(const std::string& queryStr, Args&&... args)
{
    return prepare(queryStr).bind(std::forward<Args>(args)...).execute();
}

// =====================================================================================================================
// Statement impl
// =====================================================================================================================

template <typename T>
inline fty::db::Statement& fty::db::Statement::bind(const std::string& name, const T& value)
{
    set(name, value);
    return *this;
}

template <typename TArg, typename... TArgs>
inline fty::db::Statement& fty::db::Statement::bind(TArg&& val, TArgs&&... args)
{
    bind(std::forward<TArg>(val));
    bind(std::forward<TArgs>(args)...);
    return *this;
}

template <typename TArg>
inline fty::db::Statement& fty::db::Statement::bind(Arg<TArg>&& arg)
{
    if (arg.isNull) {
        setNull(arg.name.data());
    } else {
        set(arg.name.data(), arg.value);
    }
    return *this;
}

template <typename TArg, typename... TArgs>
inline fty::db::Statement& fty::db::Statement::bindMulti(size_t count, TArg&& val, TArgs&&... args)
{
    bindMulti(count, std::forward<TArg>(val));
    bindMulti(count, std::forward<TArgs>(args)...);
    return *this;
}

template <typename TArg>
inline fty::db::Statement& fty::db::Statement::bindMulti(size_t count, Arg<TArg>&& arg)
{
    if (arg.isNull) {
        setNull(fmt::format("{}_{}", arg.name, count));
    } else {
        set(fmt::format("{}_{}", arg.name, count), arg.value);
    }
    return *this;
}

// =====================================================================================================================
// Row impl
// =====================================================================================================================

template <typename T>
inline T fty::db::Row::get(const std::string& col) const
{
    if (isNull(col)) {
        return {};
    }

    if constexpr (std::is_same_v<T, std::string>) {
        return getString(col);
    } else if constexpr (std::is_same_v<T, bool>) {
        return getBool(col);
    } else if constexpr (std::is_same_v<T, int64_t>) {
        return getInt64(col);
    } else if constexpr (std::is_same_v<T, int32_t>) {
        return getInt32(col);
    } else if constexpr (std::is_same_v<T, int16_t>) {
        return getInt16(col);
    } else if constexpr (std::is_same_v<T, int8_t>) {
        return getInt8(col);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        return getUint64(col);
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        return getUint32(col);
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        return getUint16(col);
    } else if constexpr (std::is_same_v<T, uint8_t>) {
        return getUint8(col);
    } else if constexpr (std::is_same_v<T, float>) {
        return getFloat(col);
    } else if constexpr (std::is_same_v<T, double>) {
        return getDouble(col);
    } else {
        static_assert(fty::always_false<T>, "Unsupported type");
    }
}

template <typename T>
inline void fty::db::Row::get(const std::string& name, T& val) const
{
    val = get<std::decay_t<T>>(name);
}
