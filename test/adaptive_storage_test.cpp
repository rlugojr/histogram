// Copyright 2015-2016 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <boost/histogram/storage/adaptive_storage.hpp>
#include <boost/histogram/storage/container_storage.hpp>
#include <boost/histogram/serialization.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>
#include <limits>

namespace boost {
namespace histogram {

template <typename T>
adaptive_storage<> prepare(unsigned n=1) {
    adaptive_storage<> s(n);
    s.increase(0);
    const auto tmax = std::numeric_limits<T>::max();
    while (s.value(0) < 0.1 * tmax)
        s += s;
    return s;
}

template <>
adaptive_storage<> prepare<void>(unsigned n) {
    adaptive_storage<> s(n);
    return s;
}

template <>
adaptive_storage<> prepare<detail::weight>(unsigned n) {
    adaptive_storage<> s(n);
    s.increase(0, 1.0);
    return s;
}

template <>
adaptive_storage<> prepare<detail::mp_int>(unsigned n) {
    adaptive_storage<> s(n);
    s.increase(0);
    const auto tmax = std::numeric_limits<uint64_t>::max();
    while (s.value(0) <= tmax)
        s += s;
    return s;
}

struct storage_access {
    template <typename T>
    static adaptive_storage<>
    set_value(unsigned n, T x) {
        adaptive_storage<> s = prepare<T>(n);
        static_cast<T*>(s.buffer_.ptr_)[0] = x;
        return s;
    }
};

template <typename T>
void copy_impl() {
    const auto b = prepare<T>(1);
    auto a(b);
    BOOST_TEST(a == b);
    a = b;
    BOOST_TEST(a == b);
    a.increase(0);
    BOOST_TEST(!(a == b));
    a = b;
    BOOST_TEST(a == b);
    a = prepare<T>(2);
    BOOST_TEST(!(a == b));
    a = b;
    BOOST_TEST(a == b);
}

template <typename T>
void serialization_impl()
{
    const auto a = storage_access::set_value(1, T(1));
    std::ostringstream os;
    std::string buf;
    {
        std::ostringstream os;
        boost::archive::text_oarchive oa(os);
        oa << a;
        buf = os.str();
    }
    adaptive_storage<> b;
    BOOST_TEST(!(a == b));
    {
        std::istringstream is(buf);
        boost::archive::text_iarchive ia(is);
        ia >> b;
    }
    BOOST_TEST(a == b);
}

template <>
void serialization_impl<void>()
{
    adaptive_storage<> a(1);
    std::ostringstream os;
    std::string buf;
    {
        std::ostringstream os;
        boost::archive::text_oarchive oa(os);
        oa << a;
        buf = os.str();
    }
    adaptive_storage<> b;
    BOOST_TEST(!(a == b));
    {
        std::istringstream is(buf);
        boost::archive::text_iarchive ia(is);
        ia >> b;
    }
    BOOST_TEST(a == b);
}

template <typename T>
void equal_impl() {
    adaptive_storage<> a(1);
    auto b = storage_access::set_value(1, T(0));
    BOOST_TEST_EQ(a.value(0), 0.0);
    BOOST_TEST_EQ(a.variance(0), 0.0);
    BOOST_TEST(a == b);
    b.increase(0);
    BOOST_TEST(!(a == b));

    container_storage<std::vector<unsigned>> c(1);
    auto d = storage_access::set_value(1, T(0));
    BOOST_TEST(c == d);
    c.increase(0);
    BOOST_TEST(!(c == d));
}

template <>
void equal_impl<void>() {
    adaptive_storage<> a(1);
    adaptive_storage<> b(1);
    BOOST_TEST_EQ(a.value(0), 0.0);
    BOOST_TEST_EQ(a.variance(0), 0.0);
    BOOST_TEST(a == b);
    b.increase(0);
    BOOST_TEST(!(a == b));
    adaptive_storage<> c = storage_access::set_value(1, unsigned(0));
    BOOST_TEST(c == a);
}

template <typename T>
void increase_and_grow_impl()
{
    auto tmax = std::numeric_limits<T>::max();
    adaptive_storage<> s = storage_access::set_value<T>(2, tmax - 1);
    auto n = s;
    auto n2 = s;

    n.increase(0);
    n.increase(0);

    adaptive_storage<> x(2);
    x.increase(0);
    n2 += x;
    n2 += x;

    double v = tmax;
    ++v;
    BOOST_TEST_EQ(n.value(0), v);
    BOOST_TEST_EQ(n2.value(0), v);
    BOOST_TEST_EQ(n.value(1), 0.0);
    BOOST_TEST_EQ(n2.value(1), 0.0);
}

template <>
void increase_and_grow_impl<void>()
{
    adaptive_storage<> s(2);
    s.increase(0);
    BOOST_TEST_EQ(s.value(0), 1.0);
    BOOST_TEST_EQ(s.value(1), 0.0);
}

template <typename T>
void convert_container_storage_impl() {
    const auto aref = storage_access::set_value(1, T(0));
    BOOST_TEST_EQ(aref.value(0), 0.0);
    container_storage<std::vector<uint8_t>> s(1);
    s.increase(0);

    auto a = aref;
    a = s;
    BOOST_TEST_EQ(a.value(0), 1.0);
    BOOST_TEST(a == s);
    a.increase(0);
    BOOST_TEST(!(a == s));

    adaptive_storage<> b(s);
    BOOST_TEST_EQ(b.value(0), 1.0);
    BOOST_TEST(b == s);
    b.increase(0);
    BOOST_TEST(!(b == s));

    auto c = aref;
    c += s;
    BOOST_TEST_EQ(c.value(0), 1.0);
    BOOST_TEST(c == s);
    BOOST_TEST(s == c);

    container_storage<std::vector<uint8_t>> t(2);
    t.increase(0);
    BOOST_TEST(!(c == t));
}

template <>
void convert_container_storage_impl<void>() {
    adaptive_storage<> aref(1);
    BOOST_TEST_EQ(aref.value(0), 0.0);
    container_storage<std::vector<unsigned>> s(1);
    s.increase(0);

    auto a = aref;
    a = s;
    BOOST_TEST_EQ(a.value(0), 1.0);
    BOOST_TEST(a == s);
    a.increase(0);
    BOOST_TEST(!(a == s));

    auto c = aref;
    c += s;
    BOOST_TEST_EQ(c.value(0), 1.0);
    BOOST_TEST(c == s);
}

} // NS histogram
} // NS boost

int main() {
    using namespace boost::histogram;

    // copy
    {
        copy_impl<detail::weight>();
        copy_impl<void>();
        copy_impl<uint8_t>();
        copy_impl<uint16_t>();
        copy_impl<uint32_t>();
        copy_impl<uint64_t>();
        copy_impl<detail::mp_int>();
    }

    // equal_operator
    {
        equal_impl<void>();
        equal_impl<uint8_t>();
        equal_impl<uint16_t>();
        equal_impl<uint32_t>();
        equal_impl<uint64_t>();
        equal_impl<detail::mp_int>();
        equal_impl<detail::weight>();

        // special case
        adaptive_storage<> a = storage_access::set_value(1, unsigned(0));
        adaptive_storage<> b(1);
    }

    // increase_and_grow
    {
        increase_and_grow_impl<void>();
        increase_and_grow_impl<uint8_t>();
        increase_and_grow_impl<uint16_t>();
        increase_and_grow_impl<uint32_t>();
        increase_and_grow_impl<uint64_t>();

        // only increase for mp_int
        auto a = storage_access::set_value(2, detail::mp_int(1));
        BOOST_TEST_EQ(a.value(0), 1.0);
        BOOST_TEST_EQ(a.value(1), 0.0);
        a.increase(0);
        BOOST_TEST_EQ(a.value(0), 2.0);
        BOOST_TEST_EQ(a.value(1), 0.0);
    }

    // add_and_grow
    {
        adaptive_storage<> a(1);
        a.increase(0);
        double x = 1.0;
        adaptive_storage<> y(1);
        BOOST_TEST_EQ(y.value(0), 0.0);
        a += y;
        BOOST_TEST_EQ(a.value(0), x);
        for (unsigned i = 0; i < 80; ++i) {
            a += a;
            x += x;
            adaptive_storage<> b(1);
            b += a;
            BOOST_TEST_EQ(a.value(0), x);
            BOOST_TEST_EQ(a.variance(0), x);
            BOOST_TEST_EQ(b.value(0), x);
            BOOST_TEST_EQ(b.variance(0), x);
            b.increase(0, 0.0);
            BOOST_TEST_EQ(b.value(0), x);
            BOOST_TEST_EQ(b.variance(0), x);
            adaptive_storage<> c(1);
            c.increase(0, 0.0);
            c += a;
            BOOST_TEST_EQ(c.value(0), x);
            BOOST_TEST_EQ(c.variance(0), x);
        }
    }

    // convert_container_storage
    {
        convert_container_storage_impl<detail::weight>();
        convert_container_storage_impl<void>();
        convert_container_storage_impl<uint8_t>();
        convert_container_storage_impl<uint16_t>();
        convert_container_storage_impl<uint32_t>();
        convert_container_storage_impl<uint64_t>();
        convert_container_storage_impl<detail::mp_int>();
    }

    // serialization_test
    {
        serialization_impl<void>();
        serialization_impl<uint8_t>();
        serialization_impl<uint16_t>();
        serialization_impl<uint32_t>();
        serialization_impl<uint64_t>();
        serialization_impl<detail::mp_int>();
        serialization_impl<detail::weight>();
    }

    return boost::report_errors();
}