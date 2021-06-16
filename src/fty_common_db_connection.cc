#include "fty_common_db_connection.h"
#include "fty_common_db_dbpath.h"
#include <tntdb.h>

// =====================================================================================================================

void fty::db::shutdown()
{
    tntdb::dropCached();
}

// =====================================================================================================================

struct fty::db::Row::Impl
{
    explicit Impl(const tntdb::Row& row)
        : m_row(row)
    {
    }

    tntdb::Row m_row;
};

// =====================================================================================================================

struct fty::db::Rows::Impl
{
    explicit Impl(const tntdb::Result& rows)
        : m_rows(rows)
    {
    }

    tntdb::Result m_rows;
};

// =====================================================================================================================

struct fty::db::Statement::Impl
{
    explicit Impl(const tntdb::Statement& st)
        : m_st(st)
    {
    }

    mutable tntdb::Statement m_st;
};

// =====================================================================================================================

struct fty::db::Connection::Impl
{
    Impl()
        : m_connection(tntdb::connectCached(getenv("DBURL") ? getenv("DBURL") : DBConn::url))
    {
    }

    tntdb::Connection m_connection;
};

// =====================================================================================================================

struct fty::db::Transaction::Impl
{
    explicit Impl(tntdb::Connection& tr)
        : m_trans(tr)
    {
    }

    tntdb::Transaction m_trans;
};

// =====================================================================================================================

fty::db::Connection::Connection()
    : m_impl(new Impl)
{
}

fty::db::Statement fty::db::Connection::prepare(const std::string& sql)
{
    return fty::db::Statement(std::make_unique<Statement::Impl>(m_impl->m_connection.prepareCached(sql)));
}

int64_t fty::db::Connection::lastInsertId()
{
    return m_impl->m_connection.lastInsertId();
}

// =====================================================================================================================

fty::db::Statement::Statement(std::unique_ptr<Statement::Impl> impl)
    : m_impl(std::move(impl))
{
}

fty::db::Row fty::db::Statement::selectRow() const
{
    return fty::db::Row(std::make_shared<fty::db::Row::Impl>(m_impl->m_st.selectRow()));
}

fty::db::Rows fty::db::Statement::select() const
{
    return fty::db::Rows(std::make_shared<fty::db::Rows::Impl>(m_impl->m_st.select()));
}

uint fty::db::Statement::execute() const
{
    return m_impl->m_st.execute();
}

fty::db::Statement& fty::db::Statement::bind()
{
    return *this;
}

void fty::db::Statement::set(const std::string& name, const std::string& val)
{
    m_impl->m_st.set(name, val);
}

void fty::db::Statement::set(const std::string& name, bool val)
{
    m_impl->m_st.set(name, val);
}

void fty::db::Statement::set(const std::string& name, int8_t val)
{
    m_impl->m_st.set(name, val);
}

void fty::db::Statement::set(const std::string& name, uint8_t val)
{
    m_impl->m_st.set(name, val);
}


void fty::db::Statement::set(const std::string& name, int16_t val)
{
    m_impl->m_st.set(name, val);
}

void fty::db::Statement::set(const std::string& name, uint16_t val)
{
    m_impl->m_st.set(name, val);
}

void fty::db::Statement::set(const std::string& name, int32_t val)
{
    m_impl->m_st.set(name, val);
}

void fty::db::Statement::set(const std::string& name, uint32_t val)
{
    m_impl->m_st.set(name, val);
}

void fty::db::Statement::set(const std::string& name, int64_t val)
{
    m_impl->m_st.set(name, val);
}

void fty::db::Statement::set(const std::string& name, uint64_t val)
{
    m_impl->m_st.set(name, val);
}

void fty::db::Statement::setNull(const std::string& name)
{
    m_impl->m_st.setNull(name);
}

void fty::db::Statement::set(const std::string& name, float val)
{
    m_impl->m_st.set(name, val);
}

void fty::db::Statement::set(const std::string& name, double val)
{
    m_impl->m_st.set(name, val);
}

// =====================================================================================================================

fty::db::Row::Row(std::shared_ptr<Impl> impl)
    : m_impl(impl)
{
}

std::string fty::db::Row::getString(const std::string& name) const
{
    try {
        return m_impl->m_row.getString(name);
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

bool fty::db::Row::getBool(const std::string& name) const
{
    try {
        return m_impl->m_row.getBool(name);
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

int8_t fty::db::Row::getInt8(const std::string& name) const
{
    try {
        return int8_t(m_impl->m_row.getInt(name));
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

uint8_t fty::db::Row::getUint8(const std::string& name) const
{
    try {
        return uint8_t(m_impl->m_row.getUnsigned(name));
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

int16_t fty::db::Row::getInt16(const std::string& name) const
{
    try {
        return int16_t(m_impl->m_row.getUnsigned(name));
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

uint16_t fty::db::Row::getUint16(const std::string& name) const
{
    try {
        return uint16_t(m_impl->m_row.getUnsigned(name));
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

int32_t fty::db::Row::getInt32(const std::string& name) const
{
    try {
        return m_impl->m_row.getInt(name);
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

uint32_t fty::db::Row::getUint32(const std::string& name) const
{
    try {
        return m_impl->m_row.getUnsigned(name);
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

int64_t fty::db::Row::getInt64(const std::string& name) const
{
    try {
        return m_impl->m_row.getInt64(name);
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

uint64_t fty::db::Row::getUint64(const std::string& name) const
{
    try {
        return m_impl->m_row.getUnsigned64(name);
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

float fty::db::Row::getFloat(const std::string& name) const
{
    try {
        return m_impl->m_row.getFloat(name);
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

double fty::db::Row::getDouble(const std::string& name) const
{
    try {
        return m_impl->m_row.getDouble(name);
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

bool fty::db::Row::isNull(const std::string& name) const
{
    try {
        return m_impl->m_row.isNull(name);
    } catch (const tntdb::NullValue& /* e*/) {
        return {};
    }
}

std::string fty::db::Row::get(const std::string& col) const
{
    if (isNull(col)) {
        return {};
    } else {
        return getString(col);
    }
}

// =====================================================================================================================
// Rows iterator impl
// =====================================================================================================================

void fty::db::ConstIterator::setOffset(size_t off)
{
    if (off != m_offset) {
        m_offset = off;
        if (m_offset < m_rows.m_impl->m_rows.size()) {
            m_current = std::make_shared<fty::db::Row::Impl>(m_rows.m_impl->m_rows.getRow(unsigned(m_offset)));
        }
    }
}

fty::db::ConstIterator::ConstIterator(const Rows& r, size_t off)
    : m_rows(r)
    , m_offset(off)
{
    if (m_offset < r.m_impl->m_rows.size()) {
        m_current = std::make_shared<fty::db::Row::Impl>(r.m_impl->m_rows.getRow(unsigned(m_offset)));
    }
}

bool fty::db::ConstIterator::operator==(const ConstIterator& it) const
{
    return m_offset == it.m_offset;
}

bool fty::db::ConstIterator::operator!=(const ConstIterator& it) const
{
    return !operator==(it);
}

fty::db::ConstIterator& fty::db::ConstIterator::operator++()
{
    setOffset(m_offset + 1);
    return *this;
}

fty::db::ConstIterator fty::db::ConstIterator::operator++(int)
{
    ConstIterator ret = *this;
    setOffset(m_offset + 1);
    return ret;
}

fty::db::ConstIterator fty::db::ConstIterator::operator--()
{
    setOffset(m_offset - 1);
    return *this;
}

fty::db::ConstIterator fty::db::ConstIterator::operator--(int)
{
    ConstIterator ret = *this;
    setOffset(m_offset - 1);
    return ret;
}

fty::db::ConstIterator::ConstReference fty::db::ConstIterator::operator*() const
{
    return m_current;
}

fty::db::ConstIterator::ConstPointer fty::db::ConstIterator::operator->() const
{
    return &m_current;
}

fty::db::ConstIterator& fty::db::ConstIterator::operator+=(difference_type n)
{
    setOffset(m_offset + size_t(n));
    return *this;
}

fty::db::ConstIterator fty::db::ConstIterator::operator+(difference_type n) const
{
    ConstIterator it(*this);
    it += n;
    return it;
}

fty::db::ConstIterator& fty::db::ConstIterator::operator-=(difference_type n)
{
    setOffset(m_offset - size_t(n));
    return *this;
}

fty::db::ConstIterator fty::db::ConstIterator::operator-(difference_type n) const
{
    ConstIterator it(*this);
    it -= n;
    return it;
}

fty::db::ConstIterator::difference_type fty::db::ConstIterator::operator-(const ConstIterator& it) const
{
    return fty::db::ConstIterator::difference_type(m_offset - it.m_offset);
}

// =====================================================================================================================
// Rows impl
// =====================================================================================================================

inline fty::db::ConstIterator fty::db::Rows::begin() const
{
    return fty::db::ConstIterator(*this, 0);
}

inline fty::db::ConstIterator fty::db::Rows::end() const
{
    return fty::db::ConstIterator(*this, size());
}

inline size_t fty::db::Rows::size() const
{
    return m_impl->m_rows.size();
}

inline bool fty::db::Rows::empty() const
{
    return m_impl->m_rows.empty();
}

inline fty::db::Row fty::db::Rows::operator[](size_t off) const
{
    return fty::db::Row(std::make_unique<fty::db::Row::Impl>(m_impl->m_rows.getRow(unsigned(off))));
}

inline fty::db::Rows::Rows(std::shared_ptr<Impl> impl)
    : m_impl(impl)
{
}

inline fty::db::Rows::Rows()
{
}

// =====================================================================================================================
// Transaction impl
// =====================================================================================================================

inline fty::db::Transaction::Transaction(Connection& con)
    : m_impl(std::make_unique<Impl>(con.m_impl->m_connection))
{
}

inline void fty::db::Transaction::commit()
{
    m_impl->m_trans.commit();
}

inline void fty::db::Transaction::rollback()
{
    m_impl->m_trans.rollback();
}

// =====================================================================================================================
