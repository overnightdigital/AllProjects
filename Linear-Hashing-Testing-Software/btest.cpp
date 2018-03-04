#include <algorithm>
#include <future>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <thread>
#include <vector>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "ADS_set.h"

//#define PH2

bool const ISTTY = isatty(fileno(stderr)) && isatty(fileno(stdout));

#define RED(x)    (ISTTY ? "\033[1;31m" : "") << x << (ISTTY ? "\033[0m" : "")
#define GREEN(x)  (ISTTY ? "\033[1;32m" : "") << x << (ISTTY ? "\033[0m" : "")
#define YELLOW(x) (ISTTY ? "\033[1;33m" : "") << x << (ISTTY ? "\033[0m" : "")
#define BLUE(x)   (ISTTY ? "\033[1;34m" : "") << x << (ISTTY ? "\033[0m" : "")

#define DEFINED 0x55

struct val_t {
    size_t i;
    char defined = ~DEFINED;

    val_t(size_t i): i{ i }, defined{ DEFINED } {}

    val_t()                        = default;
    val_t(val_t const&)            = default;
    val_t& operator=(val_t const&) = default;
};

std::ostream& operator<<(std::ostream& o, val_t const& v) {
    if(v.defined == DEFINED) { return o << v.i; }
    return o << "undefined";
}

std::istream& operator>>(std::istream& i, val_t& v) {
    v.defined = DEFINED;
    return i >> v.i;
}

namespace std {
    template <>
    struct hash<val_t> {
        size_t operator()(val_t const& v) const {
            if(v.defined == DEFINED) { return std::hash<size_t>{}(v.i); }
            throw std::invalid_argument("hash: undefined value");
        }
    };

    template <>
    struct equal_to<val_t> {
        bool operator()(val_t const& lhs, val_t const& rhs) const {
            if(lhs.defined != DEFINED && rhs.defined != DEFINED) {
                throw std::invalid_argument("equal_to: both lhs and rhs undefined");
            } else if(lhs.defined != DEFINED) {
                throw std::invalid_argument("equal_to: lhs undefined");
            } else if(rhs.defined != DEFINED) {
                throw std::invalid_argument("equal_to: rhs undefined");
            }

            return lhs.i == rhs.i;
        }
    };

    template <>
    struct less<val_t> {
        bool operator()(val_t const& lhs, val_t const& rhs) const {
            if(lhs.defined != DEFINED && rhs.defined != DEFINED) {
                throw std::invalid_argument("less: both lhs and rhs undefined");
            } else if(lhs.defined != DEFINED) {
                throw std::invalid_argument("less: lhs undefined");
            } else if(rhs.defined != DEFINED) {
                throw std::invalid_argument("less: rhs undefined");
            }

            return lhs.i < rhs.i;
        }
    };
}

namespace ads {
    template <class T>
    using set =
#ifdef SIZE
        ADS_set<T, SIZE>;
#else
        ADS_set<T>;
#endif
}

// gestohlen aus simpletest
template <typename C, typename It>
std::string it2str(const C &c, const It &it) {
    if(it == c.end()) return std::string{"end()"};
    std::stringstream buf;
    buf << '&' << *it;
    return buf.str();
}

// gestohlen aus simpletest
template <typename C1, typename It1, typename C2, typename It2>
bool it_equal(const C1 &c1, const It1 &it1, const C2 &c2, const It2 &it2) {
    return (it1 == c1.end() && it2 == c2.end()) ||
           (it1 != c1.end() && it2 != c2.end() && std::equal_to<typename C1::value_type>{}(*it1, *it2));
}

bool operator==(ads::set<val_t> const& lhs, std::set<val_t> const& rhs) {
    if(lhs.size() != rhs.size()) { return false; }
    for(auto const& v: rhs) { if(!lhs.count(v)) { return false; } }
    return true;
}

bool operator!=(ads::set<val_t> const& lhs, std::set<val_t> const& rhs) {
    return !(lhs == rhs);
}

void dump_compare(ads::set<val_t> const& a, std::set<val_t> const& r) {
    std::cerr << "values that were expected:\n\n";

    for(auto const& v: r) { std::cerr << v << ' '; }
    std::cerr << "\n\n";

    std::cerr << "dump of student container:\n\n";
    a.dump();
    std::cerr << "size = " << a.size() << " (should be " << r.size() << ")\n";
}

void sanity_check(std::string const& where, ads::set<val_t> const& a, std::set<val_t> const& r) {
    if(a != r) {
        std::cerr << RED("[" << where << "] err: sanity check failed --- equality does not hold.\n");
        dump_compare(a, r);

        std::abort();
    }
}

#ifdef PH2
template <class RNG>
void test_insert(ads::set<val_t>& a, std::set<val_t>& r, size_t n, size_t max_value, RNG&& gen) {
    std::cerr << "\n=== test_insert ===\n";
    std::uniform_int_distribution<size_t> dist{ 0, max_value };
    for(size_t i = 0; i < n; ++i) {
        val_t v{ dist(gen) };

        std::cerr << "in " << v << '\n';
        auto it_r = r.insert(v);
        auto it_a = a.insert(v);

        if(it_a.second != it_r.second) {
            std::cerr << RED("[insert] err: mismatch of insertion status\n"
                      << "value was " << (it_a.second ? "" : "not") << " inserted, but "
                      << (it_r.second ? "should've" : "shouldn't have") << " been\n");

            std::abort();
        }

        if(!it_equal(a, it_a.first, r, it_r.first)) {
            std::cerr << "[insert] err: returned iterator does not match expected value.\n"
                      << "inserted value " << v << " but iterator points to " << it2str(a, it_a.first) << '\n';
            std::abort();
        }
    }
}
#endif

template <class RNG>
void test_insert_it(ads::set<val_t>& a, std::set<val_t>& r, size_t n, size_t max_value, RNG&& gen) {
    std::cerr << "\n=== test_insert_it ===\n";
    std::uniform_int_distribution<size_t> dist{ 0, max_value };
    std::vector<val_t> values;

    std::cerr << "ii ";
    for(size_t i = 0; i < n; ++i) {
        val_t v{ dist(gen) };

        std::cerr << v << ' ';
        values.push_back(v);
    }
    std::cerr << '\n';

    a.insert(values.begin(), values.end());
    r.insert(values.begin(), values.end());

    sanity_check("insert_it", a, r);
}

void test_insert_ilist(ads::set<val_t>& a, std::set<val_t>& r) {
    std::cerr << "\n=== test_insert_ilist ===\n";
    std::initializer_list<val_t> il = {1, 2, 777, 666, 1337, 891, 278, 1, 2, 4, 8, 16, 32, 64, 128, 256};

    std::cerr << "in { ";
    for(auto const& v: il) { std::cerr << v << ' '; }
    std::cerr << "}\n";
    a.insert(il);
    r.insert(il);

    sanity_check("insert_ilist", a, r);
}

#ifdef PH2
template <class RNG>
void test_insert_erase(ads::set<val_t>& a, std::set<val_t>& r, size_t n, size_t max_value, RNG&& gen) {
    std::cerr << "\n=== test_insert_erase ===\n";
    std::uniform_int_distribution<size_t> dist_i{ 0, max_value };
    std::uniform_real_distribution<double> dist_f{ 0, 1 };

    for(size_t i = 0; i < n; ++i) {
        val_t v{ dist_i(gen) };

        if(dist_f(gen) < 0.5) {
            std::cerr << "in " << v << '\n';
            auto it_r = r.insert(v);
            auto it_a = a.insert(v);

            if(it_a.second != it_r.second) {
                std::cerr << RED("[insert_erase] err: mismatch of insertion status\n"
                          << "value was " << (it_a.second ? "" : "not") << " inserted, but "
                          <<  (it_r.second ? "should've" : "shouldn't have") << " been\n");

                std::abort();
            }

            if(!it_equal(a, it_a.first, r, it_r.first)) {
                std::cerr << RED("[insert_erase] err: returned iterators do not match expected value.\n"
                          << "inserted value " << v << " but iterator points to " << it2str(a, it_a.first) << '\n');
                std::abort();
            }
        } else {
            std::cerr << "er " << v << '\n';
            size_t c_r = r.erase(v);
            size_t c_a = a.erase(v);

            if(c_r != c_a) {
                std::cerr << RED("[insert_erase] err: returned count for erase does not match expected value.\n"
                          << "erasing value " << v << " returned " << c_a << ", but expected " << c_r << '\n');
                std::abort();
            }
        }
    }

    sanity_check("insert_erase", a, r);
}

template <class RNG>
void test_insert_it_erase(ads::set<val_t>& a, std::set<val_t>& r, size_t n, size_t max_value, RNG&& gen) {
    std::cerr << "\n=== test_insert_it_erase ===\n";
    std::uniform_int_distribution<size_t> dist_i(0, max_value);
    std::uniform_real_distribution<double> dist_f(0, 1);

    std::vector<val_t> values_to_insert;
    std::vector<val_t> values_to_erase;

    std::cerr << "ii ";
    for(size_t i = 0; i < n; ++i) {
        val_t v{ dist_i(gen) };

        if(dist_f(gen) < 0.5) {
            std::cerr << v << ' ';
            values_to_insert.push_back(v);
        } else {
            values_to_erase.push_back(v);
        }
    }
    std::cerr << '\n';

    a.insert(values_to_insert.begin(), values_to_insert.end());
    r.insert(values_to_insert.begin(), values_to_insert.end());

    for(auto const& v: values_to_erase) {
        std::cerr << "er " << v << '\n';
        a.erase(v);
        r.erase(v);
    }

    sanity_check("insert_it_erase", a, r);
}
#endif

void test_count(ads::set<val_t> const& a, std::set<val_t> const& r, size_t max_value) {
    std::cerr << "\n=== test_count ===\n";
    for(size_t i = 0; i < max_value; ++i) {
        std::cerr << "co " << i << '\n';
        size_t c_a = a.count(i);
        size_t c_r = r.count(i);

        if(c_a != c_r) {
            std::cerr << RED("[count] err: returned count does not match for value " << i << '\n'
                      << "expected: " << c_r << " but got " << c_a << '\n');

            dump_compare(a, r);
            std::abort();
        }
    }
}

#ifdef PH2
void test_find(ads::set<val_t> const& a, std::set<val_t> const& r, size_t max_value) {
    std::cerr << "\n=== test_find ===\n";
    for(size_t i = 0; i < max_value; ++i) {
        std::cerr << "fi " << i << '\n';
        auto it_r = r.find(i);
        auto it_a = a.find(i);

        if(!it_equal(a, it_a, r, it_r)) {
            std::cerr << RED("[find] err: returned iterators do not match expected value.\n"
                      << "value to find was " << i << " but iterator points to " << it2str(a, it_a) << '\n');

            dump_compare(a, r);
            std::abort();
        }
    }
}
#endif

void test_initlist_constructor1() {
    std::cerr << "\n=== test_initlist_constructor1 ===\n";

    std::cerr << "constructing w/ { }\n";
    ads::set<val_t> a{ std::initializer_list<val_t>{} };

    if(a.size() != 0) {
        std::cerr << RED("[initlist_constructor1] err: constructed w/ empty initlist, but size is not 0!\n");
        std::abort();
    }

    if(!a.empty()) {
        std::cerr << RED("[initlist_constructor1] err: constructed w/ empty initlist, but is not empty!\n");
        std::abort();
    }

    for(size_t i = 0; i < 10; ++i) {
        std::cerr << "co " << i << '\n';
        if(a.count(i)) {
            std::cerr << RED("[initlist_constructor1] err: count(" << i << ") returns 1, but expected 0\n");
            std::abort();
        }
    }
}

void test_initlist_constructor2() {
    std::cerr << "\n=== test_initlist_constructor2 ===\n";

    std::initializer_list<val_t> il = { 1, 2, 3 };

    std::cerr << "constructing w/ { ";
    for(auto const& v: il) { std::cerr << v << ' '; }
    std::cerr << "}\n";

    ads::set<val_t> a{ il };
    std::set<val_t> r{ il };

    sanity_check("initlist_constructor2", a, r);
}

template <class RNG>
void test_initlist_constructor3(size_t n, size_t max_value, RNG&& gen) {
    std::cerr << "\n=== test_initlist_constructor3 ===\n";
    std::initializer_list<val_t> il = { 1, 2, 3 };

    std::cerr << "constructing w/ { ";
    for(auto const& v: il) { std::cerr << v << ' '; }
    std::cerr << "}\n";

    ads::set<val_t> a{ il };
    std::set<val_t> r{ il };

    std::uniform_int_distribution<size_t> dist{ 0, max_value };
    std::vector<val_t> vs;

    std::cerr << "ii ";
    for(size_t i = 0; i < n; ++i) {
        val_t v{ dist(gen) };
        std::cerr << v << ' ';
        vs.push_back(v);
    }
    std::cerr << '\n';

    a.insert(vs.begin(), vs.end());
    r.insert(vs.begin(), vs.end());

    sanity_check("initlist_constructor3", a, r);
}

void test_range_constructor1() {
    std::cerr << "\n=== test_range_constructor1 ===\n";
    std::vector<val_t> const vs{ };

    std::cerr << "constructing w/ [ ]\n";
    ads::set<val_t> a(vs.begin(), vs.end());

    if(a.size() != 0) {
        std::cerr << RED("[range_constructor1] err: constructed w/ empty range, but size is not 0!\n");
        std::abort();
    }

    if(!a.empty()) {
        std::cerr << RED("[range_constructor1] err: constructed w/ empty range, but is not empty!\n");
        std::abort();
    }

    for(size_t i = 0; i < 10; ++i) {
        std::cerr << "co " << i << '\n';
        if(a.count(i)) {
            std::cerr << RED("[range_constructor1] err: count(" << i << ") returns 1, but expected 0\n");
            std::abort();
        }
    }
}

void test_range_constructor2() {
    std::cerr << "\n=== test_range_constructor2 ===\n";
    std::vector<val_t> const vs{ 1, 2, 3 };

    std::cerr << "constructing w/ [ ";
    for(auto const& v: vs) { std::cerr << v << ' '; }
    std::cerr << "]\n";

    ads::set<val_t> a(vs.begin(), vs.end());
    std::set<val_t> r(vs.begin(), vs.end());

    sanity_check("range_constructor2", a, r);
}

template <class RNG>
void test_range_constructor3(size_t n, size_t max_value, RNG&& gen) {
    std::cerr << "\n=== test_range_constructor3 ===\n";
    std::vector<val_t> vs{ 1, 2, 3 };

    std::cerr << "constructing w/ [ ";
    for(auto const& v: vs) { std::cerr << v << ' '; }
    std::cerr << "]\n";

    ads::set<val_t> a(vs.begin(), vs.end());
    std::set<val_t> r(vs.begin(), vs.end());

    std::uniform_int_distribution<size_t> dist{ 0, max_value };
    std::cerr << "ii ";
    for(size_t i = 0; i < n; ++i) {
        val_t v{ dist(gen) };
        std::cerr << v << ' ';
        vs.push_back(v);
    }
    std::cerr << '\n';

    a.insert(vs.begin(), vs.end());
    r.insert(vs.begin(), vs.end());

    sanity_check("range_constructor3", a, r);
}

void test_empty(ads::set<val_t> const& a, std::set<val_t> const& r) {
    std::cerr << "\n=== test_empty ===\n";
    if(a.empty() != r.empty()) {
        std::cerr << RED("[empty] err: container is " << (a.empty() ? "" : "not") << " empty, "
                  << "but shouldn't be\n");
        dump_compare(a, r);

        std::abort();
    }
}

void test_size(ads::set<val_t> const& a, std::set<val_t> const& r) {
    std::cerr << "\n=== test_size ===\n";
    if(a.size() != r.size()) {
        std::cerr << RED("[empty] err: container size is " << a.size() << ", but should be " << r.size() << '\n');
        dump_compare(a, r);
        std::abort();
    }
}

#ifdef PH2
void test_clear(ads::set<val_t>& a, std::set<val_t>& r) {
    std::cerr << "\n=== test_clear ===\n";
    a.clear();
    r.clear();

    if(a.size() != 0) {
        std::cerr << RED("[clear] err: size after clear is not 0!\n");
        dump_compare(a, r);
        std::abort();
    }

    if(!a.empty()) {
        std::cerr << RED("[clear] err: container is not empty after clear!\n");
        dump_compare(a, r);
        std::abort();
    }

    if(a.begin() != a.end()) {
        std::cerr << RED("[clear] err: iterator range is not empty after clear!\n");
        dump_compare(a, r);
        std::abort();
    }
}

void test_copy(ads::set<val_t> const& a, std::set<val_t> const& r) {
    std::cerr << "\n=== test_copy ===\n";
    ads::set<val_t> copy_a{ a };

    if(a.size() != copy_a.size()) {
        std::cerr << RED("[copy] err: size after copying does not match.\n"
                  << "size should be: " << a.size() << ", but is " << copy_a.size() << '\n');

        dump_compare(a, r);
        std::abort();
    }

    for(auto const& v: a) {
        if(!copy_a.count(v)) {
            std::cerr << RED("[copy] err: containers not equal after copy. lost value " << v << '\n');
            dump_compare(copy_a, r);
            std::abort();
        }
    }
}

void test_assign(ads::set<val_t> const& a, std::set<val_t> const& r) {
    std::cerr << "\n=== test_assign ===\n";

    ads::set<val_t> b;
    b = a;

    if(a.size() != b.size()) {
        std::cerr << RED("[copy] err: size after assigning does not match.\n"
                  << "size should be: " << a.size() << ", but is " << b.size() << '\n');

        dump_compare(a, r);
        std::abort();
    }

    for(auto const& v: a) {
        if(!b.count(v)) {
            std::cerr << RED("[copy] err: containers not equal after assign. lost value " << v << '\n');
            dump_compare(b, r);
            std::abort();
        }
    }
}

void test_assign_initlist(ads::set<val_t>& a, std::set<val_t>& r) {
    std::cerr << "\n=== test_assign_initlist ===\n";

    a = {1, 2, 3};
    r = {1, 2, 3};

    if(a.size() != r.size()) {
        std::cerr << RED("[assign_initlist] err: size after assigning does not match.\n"
                  << "size should be: " << r.size() << ", but is " << a.size() << '\n');

        dump_compare(a, r);
        std::abort();
    }

    sanity_check("assign_initlist", a, r);
}

void test_iter(ads::set<val_t> const& a, std::set<val_t> const& r) {
    std::cerr << "\n=== test_iter ===\n";

    size_t dist = std::distance(a.begin(), a.end());
    if(dist != r.size()) {
        std::cerr << RED("[iter] err: range size (distance between begin and end) is wrong.\n"
                  << "expected: " << r.size() << ", but got: " << dist << '\n');
        dump_compare(a, r);
        std::abort();
    }

    std::vector<val_t> dump; dump.reserve(a.size());
    for(auto const& v: a) {
        if(!r.count(v)) {
            std::cerr << RED("[iter] err: encountered unexpected value while iterating: " << v << " should not be part of container!\n");
            dump_compare(a, r);
            std::abort();
        }
        dump.push_back(v);
    }

    std::vector<val_t> dump_copy = dump;

    std::sort(dump.begin(), dump.end(), std::less<val_t>{});
    auto last = std::unique(dump.begin(), dump.end(), std::equal_to<val_t>{});
    dump.erase(last, dump.end());

    if(dump.size() != r.size()) {
        std::cerr << RED("[iter] err: had duplicate encounters while iterating. found following elements (in this order):\n\n");

        for(auto const& v: dump_copy) { std::cerr << v << ' '; }
        std::cerr << '\n';

        dump_compare(a, r);
        std::abort();
    }
}
#endif

#ifndef PH2
template <class RNG>
void test_all_ph1(size_t n, size_t max_value, RNG&& gen) {
    test_initlist_constructor3(n, max_value, gen);
    test_range_constructor3(n, max_value, gen);

    {
        ads::set<val_t> a;
        std::set<val_t> r;

        test_count(a, r, max_value);
        test_empty(a, r);
        test_size(a, r);

        test_insert_it(a, r, n, max_value, gen);

        test_size(a, r);
        test_empty(a, r);
    }

    {
        ads::set<val_t> a;
        std::set<val_t> r;

        test_insert_ilist(a, r);
        test_size(a, r);
    }

    {
        ads::set<val_t> a;
        std::set<val_t> r;

        test_insert_it(a, r, n, max_value, gen);
        test_size(a, r);
        test_count(a, r, max_value);
    }
}
#else
template <class RNG>
void test_all_ph2(size_t n, size_t max_value, RNG&& gen) {
    test_initlist_constructor3(n, max_value, gen);
    test_range_constructor3(n, max_value, gen);

    {
        ads::set<val_t> a;
        std::set<val_t> r;

        test_iter(a, r);

        test_copy(a, r);
        test_assign(a, r);
        test_assign_initlist(a, r);

        test_insert(a, r, n, max_value, gen);
        test_iter(a, r);

        test_clear(a, r);

        test_size(a, r);
        test_empty(a, r);
    }

    {
        ads::set<val_t> a;
        std::set<val_t> r;

        test_insert_it(a, r, n, max_value, gen);
        test_count(a, r, max_value);
        test_find(a, r, max_value);

        test_size(a, r);
        test_clear(a, r);

        test_copy(a, r);
        test_assign(a, r);
    }

    {
        ads::set<val_t> a;
        std::set<val_t> r;

        test_insert_erase(a, r, n, max_value, gen);
        test_count(a, r, max_value);
        test_find(a, r, max_value);
        test_iter(a, r);

        test_size(a, r);
        test_empty(a, r);
    }

    {
        ads::set<val_t> a;
        std::set<val_t> r;
        test_insert_it_erase(a, r, n, max_value, gen);
        test_count(a, r, max_value);
        test_find(a, r, max_value);

        test_size(a, r);
        test_empty(a, r);
    }

    {
        ads::set<val_t> a;
        std::set<val_t> r;

        test_insert_ilist(a, r);
        test_count(a, r, max_value);
        test_find(a, r, max_value);
    }
}
#endif

void do_stresstest1() {
    std::cerr << "\n=== stresstest1 ===\n";

    size_t const n = 1000000;
    std::vector<val_t> vs(n);
    std::iota(vs.begin(), vs.end(), 0);

    ads::set<val_t> a;

    double elapsed_insert;
    {
        auto start = std::chrono::high_resolution_clock::now();
        a.insert(vs.begin(), vs.end());
        auto end = std::chrono::high_resolution_clock::now();

        elapsed_insert = std::chrono::duration<double, std::milli>(end - start).count();
    }

    double elapsed_count;
    {
        auto start = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < n; ++i) {
            if(!a.count(i)) {
                std::cerr << RED("[stresstest1] err: missing value " << i << '\n');
                std::abort();
            }
        }
        auto end = std::chrono::high_resolution_clock::now();

        elapsed_count = std::chrono::duration<double, std::milli>(end - start).count();
    }

    if(a.size() != n) {
        std::cerr << RED("[stresstest1] err: wrong size, expected " << n << " but is " << a.size()) << '\n';
        std::abort();
    }

    std::cerr << "elapsed_insert = " << elapsed_insert << " ms\n"
              << "elapsed_count  = " << elapsed_count  << " ms\n";
}

#ifdef PH2
void do_stresstest2() {
    std::cerr << "\n=== stresstest2 ===\n";

    size_t const n = 1000000;
    ads::set<val_t> a;

    double elapsed_insert;
    {
        auto start = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < n; ++i) {
            if(!a.insert(i).second) {
                std::cerr << RED("[stresstest2] err: returned wrong insertion status (false) for value " << i << '\n');
                std::abort();
            }
        }
        auto end = std::chrono::high_resolution_clock::now();

        elapsed_insert = std::chrono::duration<double, std::milli>(end - start).count();
    }

    if(a.size() != n) {
        std::cerr << RED("[stresstest2] err: wrong size, expected " << n << " but is " << a.size()) << '\n';
        std::abort();
    }

    double elapsed_count;
    {
        auto start = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < n; ++i) {
            if(!a.count(i)) {
                std::cerr << RED("[stresstest2] err: missing value " << i << '\n');
                std::abort();
            }
        }
        auto end = std::chrono::high_resolution_clock::now();

        elapsed_count = std::chrono::duration<double, std::milli>(end - start).count();
    }

    double elapsed_erase;
    {
        auto start = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < n; ++i) {
            if(!a.erase(i)) {
                std::cerr << RED("[stresstest2] err: couldn't erase element " << i << '\n');
                std::abort();
            }
        }
        auto end = std::chrono::high_resolution_clock::now();

        elapsed_erase = std::chrono::duration<double, std::milli>(end - start).count();
    }

    if(!a.empty() || a.size()) {
        std::cerr << RED("[stresstest2] err: not empty after erasing everything\n");
        std::abort();
    }

    std::cerr << "elapsed_insert = " << elapsed_insert << " ms\n"
              << "elapsed_count  = " << elapsed_count  << " ms\n"
              << "elapsed_erase  = " << elapsed_erase  << " ms\n";
}
#endif

/* zeit möglicherweise zu knapp bemessen für container mit pervers
 * kleinem default N. (-D SIZE) */
void stresstest() {
    auto f = std::async(std::launch::async, do_stresstest1);

    if(f.wait_for(std::chrono::seconds(1)) == std::future_status::timeout) {
        std::cerr << YELLOW("[stresstest1] timeout: exceeded 1s timeframe.\n");
        std::abort();
    }

#ifdef PH2
    auto g = std::async(std::launch::async, do_stresstest2);

    if(g.wait_for(std::chrono::seconds(2)) == std::future_status::timeout) {
        std::cerr << YELLOW("[stresstest2] timeout: exceeded 2s timeframe.\n");
        std::abort();
    }
#endif
}

#ifndef N
#define N 10
#endif

#ifndef M
#define M 10
#endif

#ifndef O
#define O 100
#endif

#ifndef V
#define V 10
#endif

#ifndef W
#define W 10
#endif

#ifndef X
#define X 100
#endif

#ifndef S
#define S 666
#endif

#ifndef T
#define T 1
#endif

int main(int argc, char** argv) {
    size_t n = N;
    size_t m = M;
    size_t o = O;

    size_t v = V;
    size_t w = W;
    size_t x = X;

    size_t s = S;
    size_t t = T;

    char c;
    while((c = getopt(argc, argv, "n:m:o:v:w:x:s:t:h")) != -1) {
        switch(c) {
            case 'n':
                n = std::atoll(optarg);
                std::cout << "n = " << n << '\n';
                break;
            case 'm':
                m = std::atoll(optarg);
                std::cout << "m = " << m << '\n';
                break;
            case 'o':
                o = std::atoll(optarg);
                std::cout << "o = " << o << '\n';
                break;
            case 'v':
                v = std::atoll(optarg);
                std::cout << "v = " << v << '\n';
                break;
            case 'w':
                w = std::atoll(optarg);
                std::cout << "w = " << w << '\n';
                break;
            case 'x':
                x = std::atoll(optarg);
                std::cout << "x = " << x << '\n';
                break;
            case 's':
                s = std::atoll(optarg);
                std::cout << "s = " << s << '\n';
                break;
            case 't':
                t = std::atoll(optarg);
                std::cout << "t = " << t << '\n';
                break;
            case 'h':
            default:
                std::cout << "usage: " << argv[0] << " opts\n"
                          << "where opt in opts is one of the following:\n\n"

                          << "  -n $value ... number of values for first test, default: " << N << '\n'
                          << "  -m $value ... stepsize for n, default: " << M << '\n'
                          << "  -o $value ... maximum value for n, default: " << O << '\n'
                          << "  -v $value ... maximum value for first test, default: " << V << '\n'
                          << "  -w $value ... stepsize for v, default: " << W << '\n'
                          << "  -x $value ... maxmimum value for v, default: " << X << '\n'
                          << "  -s $value ... first seed, default: " << S << '\n'
                          << "  -t $value ... number of seeds (will be drawn from rng w/ previous seed), default: " << T << '\n'
                          << "                will do the full test suite t times!\n"
                          << "  -h        ... this message\n\n"

                          << "btest is a program that will try to find a simple way to mess an ADS_set up. it should theoretically\n"
                          << "touch upon the full functionality, but is of course not capable of finding all possible error cases.\n";
                std::exit(-1);
        }
    }

    test_initlist_constructor1();
    test_range_constructor1();

    test_initlist_constructor2();
    test_range_constructor2();

    std::mt19937 gen;
    for(size_t i = 0; i < t; ++i) {
        gen.seed(s);
        for(size_t n_ = n; n_ <= o; n_ += m) {
            for(size_t v_ = v; v_ <= x; v_ += w) {
                std::cerr << "seed = " << s << "\nn = " << n_ << "\nv = " << v_ << '\n';
#ifdef PH2
                test_all_ph2(n_, v_, gen);
#else
                test_all_ph1(n_, v_, gen);
#endif
                std::cerr << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
            }
        }
        s = gen();
    }

    try { stresstest(); }
    catch(std::system_error const&) {
        std::cout << BLUE("stresstest disabled because no multithreading available. compile with -pthread to enable.");
    }

    std::cout << GREEN("\nOK\n");
}
