//
// Copyright(c) 2019 Eran Gilad, https://github.com/erangi/kdmt
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Keydomet.h"

#include "catch.hpp"

#include <set>
#include <unordered_set>
#include <vector>
#include <type_traits>
#include <algorithm>
#include <random>
#include <sstream>

#if (__cplusplus < 201703L) && !(defined(__clang__) && __clang_major__ > 7)
    #include <experimental/string_view>
    using std::experimental::string_view;
#else
    #include <string_view>
#endif

using namespace kdmt;
using namespace std;

using prefix2B = prefix_storage<prefix_size::SIZE_16BIT>;
using prefix4B = prefix_storage<prefix_size::SIZE_32BIT>;
using prefix8B = prefix_storage<prefix_size::SIZE_64BIT>;
using prefix16B = prefix_storage<prefix_size::SIZE_128BIT>;

TEST_CASE("Verify sizes", "[keydomet]")
{
    REQUIRE(sizeof(prefix2B::type) == 2);
    REQUIRE(sizeof(prefix4B::type) == 4);
    REQUIRE(sizeof(prefix8B::type) == 8);
    REQUIRE(sizeof(prefix16B::type) == 16);
}

TEST_CASE("str_to_prefix, 2B, const char*", "[str_to_prefix]")
{
    const char* str = "01";
    auto prefix = str_to_prefix<prefix2B::type>(str);
    REQUIRE((char)(prefix & 0xFF) == '1');
    REQUIRE((char)(prefix >> (8 * 1)) == '0');
}

TEST_CASE("str_to_prefix, 4B, const char*", "[str_to_prefix]")
{
    const char* str = "0001";
    auto prefix = str_to_prefix<prefix4B::type>(str);
    REQUIRE((char)(prefix >> (8 * 3)) == '0');
    REQUIRE((char)(prefix & 0xFF) == '1');
}

TEST_CASE("str_to_prefix, 8B, const char*", "[str_to_prefix]")
{
    const char* str = "00000001";
    auto prefix = str_to_prefix<prefix8B::type>(str);
    REQUIRE((char)(prefix & 0xFF) == '1');
    REQUIRE((char)(prefix >> (8 * 7)) == '0');
}

TEST_CASE("str_to_prefix, 16B, const char*", "[str_to_prefix]")
{
    const char* str = "0000000000000001";
    auto prefix = str_to_prefix<prefix16B::type>(str);
    REQUIRE((char)(prefix.lsbs & 0xFF) == '1');
    REQUIRE((char)(prefix.msbs >> (8 * 7)) == '0');
}

TEST_CASE("str_to_prefix, 2B, std::string", "[str_to_prefix]")
{
    string str = "01";
    auto prefix = str_to_prefix<prefix2B::type>(str);
    REQUIRE((char)(prefix & 0xFF) == '1');
    REQUIRE((char)(prefix >> (8 * 1)) == '0');
}

TEST_CASE("str_to_prefix, 4B, std::string", "[str_to_prefix]")
{
    string str = "0001";
    auto prefix = str_to_prefix<prefix4B::type>(str);
    REQUIRE((char)(prefix & 0xFF) == '1');
    REQUIRE((char)(prefix >> (8 * 3)) == '0');
}

TEST_CASE("str_to_prefix, 8B, std::string", "[str_to_prefix]")
{
    string str = "00000001";
    auto prefix = str_to_prefix<prefix8B::type>(str);
    REQUIRE((char)(prefix & 0xFF) == '1');
    REQUIRE((char)(prefix >> (8 * 7)) == '0');
}

TEST_CASE("str_to_prefix, 16B, std::string", "[str_to_prefix]")
{
    string str = "0000000000000001";
    auto prefix = str_to_prefix<prefix16B::type>(str);
    REQUIRE((char)(prefix.lsbs & 0xFF) == '1');
    REQUIRE((char)(prefix.msbs >> (8 * 7)) == '0');
}

TEST_CASE("flip_bytes, 2B", "[flip_bytes]")
{
    prefix2B::type prefix = 0x0011;
    flip_bytes(prefix);
    REQUIRE(prefix == 0x1100);
}

TEST_CASE("flip_bytes, 4B", "[flip_bytes]")
{
    prefix4B::type prefix = 0x00112233;
    flip_bytes(prefix);
    REQUIRE(prefix == 0x33221100);
}

TEST_CASE("flip_bytes, 8B", "[flip_bytes]")
{
    prefix8B::type prefix = 0x0011223344556677;
    flip_bytes(prefix);
    REQUIRE(prefix == 0x7766554433221100);
}

TEST_CASE("flip_bytes, 16B", "[flip_bytes]")
{
    prefix16B::type prefix;
    prefix.msbs = 0x0011223344556677;
    prefix.lsbs = 0x8899AABBCCDDEEFF;
    flip_bytes(prefix);
    REQUIRE(prefix.msbs == 0x7766554433221100);
    REQUIRE(prefix.lsbs == 0xFFEEDDCCBBAA9988);
}

TEST_CASE("prefix_rep, 4B, operator lt, long vs. long different keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaa"}, kdb{"bbbbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("prefix_rep, 4B, operator lt, long vs. long related keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaa"}, kdb{"aaaab"};
    REQUIRE(!(kda < kdb));
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("prefix_rep, 4B, operator lt, long vs. short keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaa"}, kdb{"bbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("prefix_rep, 4B, operator lt, short vs. short keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaa"}, kdb{"bbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("prefix_rep, 16B, operator lt, long vs. long different keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbbbbbbbbbbbbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("prefix_rep, 16B, operator lt, long vs. long related keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"aaaaaaaaaaaaaaaab"};
    REQUIRE(!(kda < kdb));
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("prefix_rep, 16B, operator lt, long vs. short keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("prefix_rep, 16B, operator lt, short vs. short keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaa"}, kdb{"bbbbbbbbbbbbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("prefix_rep, 16B, operator eq, long vs. long different keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbbbbbbbbbbbbb"};
    REQUIRE(!(kda == kdb));
    REQUIRE(kda == kda);
}

TEST_CASE("prefix_rep, 16B, operator eq, long vs. long related keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"aaaaaaaaaaaaaaaab"};
    REQUIRE(kda == kdb);
    REQUIRE(kda == kda);
}

TEST_CASE("prefix_rep, 16B, operator eq, long vs. short keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbb"};
    REQUIRE(!(kda == kdb));
    REQUIRE(kda == kda);
}

TEST_CASE("prefix_rep, 16B, operator eq, short vs. short keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaa"}, kdb{"bbbbbbbbbbbbb"};
    REQUIRE(!(kda == kdb));
    REQUIRE(kda == kda);
}

TEST_CASE("prefix_rep, 16B, operator ne, long vs. long different keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbbbbbbbbbbbbb"};
    REQUIRE(kda != kdb);
    REQUIRE(!(kda != kda));
}

TEST_CASE("prefix_rep, 16B, operator ne, long vs. long related keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"aaaaaaaaaaaaaaaab"};
    REQUIRE(!(kda != kdb));
    REQUIRE(!(kda != kda));
}

TEST_CASE("prefix_rep, 16B, operator ne, long vs. short keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbb"};
    REQUIRE(kda != kdb);
    REQUIRE(!(kda != kda));
}

TEST_CASE("prefix_rep, 16B, operator ne, short vs. short keys", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> kda{"aaa"}, kdb{"bbbbbbbbbbbbb"};
    REQUIRE(kda != kdb);
    REQUIRE(!(kda != kda));
}

TEST_CASE("string_shorter_than_prefix, 2B", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_16BIT> shorter{string(1, 's')}, longer{string(3, 'l')};
    CAPTURE((void*)(uintptr_t)shorter.get_val());
    REQUIRE(shorter.string_shorter_than_prefix());
    CAPTURE((void*)(uintptr_t)longer.get_val());
    REQUIRE(!longer.string_shorter_than_prefix());
}

TEST_CASE("string_shorter_than_prefix, 4B", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_32BIT> shorter{string(3, 's')}, longer{string(4, 'l')};
    REQUIRE(shorter.string_shorter_than_prefix());
    REQUIRE(!longer.string_shorter_than_prefix());
}

TEST_CASE("string_shorter_than_prefix, 8B", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_64BIT> shorter{string(7, 's')}, longer{string(8, 'l')};
    REQUIRE(shorter.string_shorter_than_prefix());
    REQUIRE(!longer.string_shorter_than_prefix());
}

TEST_CASE("string_shorter_than_prefix, 16B", "[prefix_rep]")
{
    prefix_rep<prefix_size::SIZE_128BIT> shorter{string(15, 's')}, longer{string(16, 'l')};
    REQUIRE(shorter.string_shorter_than_prefix());
    REQUIRE(!longer.string_shorter_than_prefix());
}

TEST_CASE("compare equal long keys, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kkkkkkkk"}, k2{"kkkkkkkk"};
    int res = k1.compare(k2);
    REQUIRE(res == 0);
}

TEST_CASE("compare equal short keys, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"k"}, k2{"k"};
    int res = k1.compare(k2);
    REQUIRE(res == 0);
}

TEST_CASE("compare k1 < k2, diff at prefix, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kkkkkkkk"}, k2{"llllllll"};
    int res = k1.compare(k2);
    REQUIRE(res < 0);
}

TEST_CASE("compare k1 > k2, diff at prefix, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kkkkkkkk"}, k2{"jjjjjjjj"};
    int res = k1.compare(k2);
    REQUIRE(res > 0);
}

TEST_CASE("compare k1 < k2, diff at suffix, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kkkkkkkk"}, k2{"kkkkllll"};
    int res = k1.compare(k2);
    REQUIRE(res < 0);
}

TEST_CASE("compare k1 > k2, diff at suffix, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kkkkkkkk"}, k2{"kkkkjjjj"};
    int res = k1.compare(k2);
    REQUIRE(res > 0);
}

TEST_CASE("compare k1 < k2, short keys, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kk"}, k2{"ll"};
    int res = k1.compare(k2);
    REQUIRE(res < 0);
}

TEST_CASE("compare k1 > k2, short keys, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kk"}, k2{"jj"};
    int res = k1.compare(k2);
    REQUIRE(res > 0);
}

TEST_CASE("operator< k1 < k2, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kkkkk"}, k2{"lllll"};
    bool res = k1 < k2;
    REQUIRE(res == true);
}

TEST_CASE("operator< k1 > k2, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kkkkk"}, k2{"jjjjj"};
    bool res = k1 < k2;
    REQUIRE(res == false);
}

TEST_CASE("operator< k1 == k2, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kkkkk"}, k2{"kkkkk"};
    bool res = k1 < k2;
    REQUIRE(res == false);
}

TEST_CASE("operator== k1 < k2, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kkkkk"}, k2{"lllll"};
    bool res = k1 == k2;
    REQUIRE(res == false);
}

TEST_CASE("operator== k1 > k2, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kkkkk"}, k2{"jjjjj"};
    bool res = k1 == k2;
    REQUIRE(res == false);
}

TEST_CASE("operator== k1 == k2, 4B", "[keydomet]")
{
    keydomet<const char*, prefix_size::SIZE_32BIT> k1{"kkkkk"}, k2{"kkkkk"};
    bool res = k1 == k2;
    REQUIRE(res == true);
}

TEST_CASE("associative containers take a const str&", "[make_find_key]")
{
    using kdmt_str = keydomet<string, prefix_size::SIZE_32BIT>;
    set<kdmt_str, less<>> s;
    string lookupStr{"dummy"};
    using find_key_type = decltype(make_key_view(s, lookupStr))::str_imp;
    constexpr bool is_const_ref = is_same<find_key_type, const string&>::value;
    CAPTURE(typeid(find_key_type).name());
    REQUIRE(is_const_ref == true);
}

TEST_CASE("associative container key requires no allocation", "[make_find_key]")
{
    using kdmt_str = keydomet<string, prefix_size::SIZE_32BIT>;
    set<kdmt_str, less<>> s;
    string org{"dummy"};
    auto fk = make_key_view(s, org);
    const string& ref = fk.get_str();
    REQUIRE(&ref == &org);
}

TEST_CASE("sorting multiple keys", "[containers]")
{
    vector<string> org_vals(10 + 100 + 1000);
    // generate single character strings (shorter than prefix_size): "0" - "9"
    generate_n(org_vals.begin(),       10,  [n = 0] () mutable { return to_string(n++); });
    // generate two character strings (exactly prefix_size): "00" - "99"
    generate_n(org_vals.begin() + 10,  10,  [n = 0] () mutable { return string{"0"} + to_string(n++); });
    generate_n(org_vals.begin() + 20,  90,  [n = 10] () mutable { return to_string(n++); });
    // generate three character strings (longer than prefix_size): "000" - "999"
    generate_n(org_vals.begin() + 110, 10,  [n = 0] () mutable { return string{"00"} + to_string(n++); });
    generate_n(org_vals.begin() + 120, 90,  [n = 0] () mutable { return string{"0"} + to_string(n++); });
    generate_n(org_vals.begin() + 210, 900, [n = 100] () mutable { return to_string(n++); });
    // this is the reference sort
    sort(org_vals.begin(), org_vals.end());

    using kdmt_view = keydomet<string_view, prefix_size::SIZE_16BIT>;
    vector<kdmt_view> kdm_vals;
    kdm_vals.reserve(org_vals.size());
    transform(org_vals.begin(), org_vals.end(), back_inserter(kdm_vals), [](const string& v) {
        return kdmt_view{v.c_str()};
    });

    std::mt19937 g(std::random_device{}());

    for (int i = 0; i < 100; ++i)
    {
        std::shuffle(kdm_vals.begin(), kdm_vals.end(), g);
        sort(kdm_vals.begin(), kdm_vals.end());
        bool eq = equal(org_vals.begin(), org_vals.end(), kdm_vals.begin(), kdm_vals.end(),
                [](const string& org, const kdmt_view& kdm) {
            return kdm.get_str() == org;
        });
        CAPTURE(org_vals);
        CAPTURE(kdm_vals);
        REQUIRE(eq);
    }
}

TEST_CASE("populate keydomet set", "[containers]")
{
    using kdmt_str = keydomet<std::string, prefix_size::SIZE_16BIT>;
    set<kdmt_str, less<>> kdmt_set;
    vector<string> input({"Ac", "Jg", "OE", "S_", "Uv", "ak", "bT", "in", "s^", "xy"});

    for (size_t i = 0; i < input.size(); ++i)
    {
        kdmt_set.insert(kdmt_str{input[i]});
        REQUIRE(kdmt_set.size() == i + 1);
    }
}

static string get_rand_key(size_t len)
{
    static mt19937 gen{random_device{}()};
    uniform_int_distribution<short> dis('A', 'z');
    string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i)
        s += (char)dis(gen);
    return s;
}

template<typename T>
static string dump_container(const T& container)
{
    stringstream ss;
    for (auto iter = container.begin(); iter != container.end(); ++iter)
    {
        ss << *iter << " ";
    }
    return ss.str();
}

TEST_CASE("searching sets with and without keydomet", "[containers]")
{
    using kdmt_str = keydomet<std::string, prefix_size::SIZE_16BIT>;
    set<kdmt_str, less<>> kdmt_set;
    set<string> str_set;

    constexpr size_t inputs_num = 10, lookups_num = 10, key_len = 2;
    for (size_t i = 0; i < inputs_num; ++i)
    {
        string key = get_rand_key(key_len);
        kdmt_set.insert(kdmt_str{key});
        str_set.insert(key);
    }
    CHECK(kdmt_set.size() == str_set.size());
    for (size_t i = 0; i < lookups_num; ++i)
    {
        string key = get_rand_key(key_len);
        bool found_kdmt = kdmt_set.find(kdmt_str{key}) == kdmt_set.end();
        bool found_str = str_set.find(key) == str_set.end();
        if (found_kdmt != found_str)
        {
            string kdmt_dump = dump_container(kdmt_set);
            string str_dump = dump_container(str_set);
            CAPTURE(kdmt_dump);
            CAPTURE(str_dump);
            FAIL();
        }
        REQUIRE(found_kdmt == found_str);
    }
}

class my_string
{
public:
    my_string(const string& s) : str(s) {}
    bool operator<(const my_string& other) const {
        return str < other.str;
    }
    bool operator==(const my_string& other) const {
        return str == other.str;
    }
    friend const char* get_raw_str(const my_string& s)
    {
        return s.str.c_str();
    }

private:
    string str;
};

ostream& operator<<(ostream& out, const my_string& str)
{
    out << get_raw_str(str);
    return out;
}

TEST_CASE("using keydomet with non-standard strings", "[custom strings]")
{
    using kdmt_my_str = keydomet<my_string, prefix_size::SIZE_16BIT>;
    set<kdmt_my_str, less<>> kdmt_set;
    my_string str1("str1"), str2("str2"), str3("str3");
    kdmt_set.insert(str3);
    kdmt_set.insert(str1);
    kdmt_set.insert(str2);
    auto iter = kdmt_set.begin();
    REQUIRE(iter->get_str() == str1);
    ++iter;
    REQUIRE(iter->get_str() == str2);
    ++iter;
    REQUIRE(iter->get_str() == str3);
}

TEST_CASE("store in str when sso not used", "[sso optimization]")
{
    using kdmt_str = keydomet<string, prefix_size::SIZE_32BIT>;
    set<kdmt_str, less<>> container;
    container.insert(string(65, '2'));
    container.insert(string(65, '1'));
    container.insert(string(65, '3'));
    string s1(65, '1'), s2(65, '2'), s3(65, '3');
    auto find_key1 = make_key_view(container, s1);
    auto iter = container.find(find_key1);
    REQUIRE(iter != container.end());
    auto find_key2 = make_key_view(container, s2);
    iter = container.find(find_key2);
    REQUIRE(iter != container.end());
    auto find_key3 = make_key_view(container, s3);
    iter = container.find(find_key3);
    REQUIRE(iter != container.end());
}

TEST_CASE("move embedded keydomet", "[sso optimization]")
{
    using kdmt_str = keydomet<string, prefix_size::SIZE_16BIT>;
    string suffix(65, '0');
    kdmt_str ks1("11" + suffix);
    kdmt_str ks2m("12" + suffix);
    kdmt_str ks2 = move(ks2m);
    kdmt_str ks3("13" + suffix);

    REQUIRE(ks1 < ks2);
    REQUIRE(ks2 < ks3);
}

TEST_CASE("copy - short string", "[sso optimization]")
{
    using kdmt_str = keydomet<std::string, prefix_size::SIZE_16BIT>;
    kdmt_str s1{"1"}, s2{"2"}, s3{"3"};
    kdmt_str c1{s1}, c2{s2}, c3{s3};

    CHECK(s1 < s2);
    CHECK(s2 < s3);

    REQUIRE(c1 < c2);
    REQUIRE(c2 < c3);
}

TEST_CASE("copy - long string", "[sso optimization]")
{
    using kdmt_str = keydomet<std::string, prefix_size::SIZE_16BIT>;
    kdmt_str s1{string{'1', 64}}, s2{string{'2', 64}}, s3{string{'3', 64}};
    kdmt_str c1{s1}, c2{s2}, c3{s3};

    CHECK(s1 < s2);
    CHECK(s2 < s3);

    REQUIRE(c1 < c2);
    REQUIRE(c2 < c3);
}

