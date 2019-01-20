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
#include <experimental/string_view>

using namespace kdmt;
using namespace std;
using experimental::string_view;

using KDS2 = KeydometStorage<KeydometSize::SIZE_16BIT>;
using KDS4 = KeydometStorage<KeydometSize::SIZE_32BIT>;
using KDS8 = KeydometStorage<KeydometSize::SIZE_64BIT>;
using KDS16 = KeydometStorage<KeydometSize::SIZE_128BIT>;

TEST_CASE("Verify sizes", "[Keydomet]")
{
    REQUIRE(sizeof(KDS2::type) == 2);
    REQUIRE(sizeof(KDS4::type) == 4);
    REQUIRE(sizeof(KDS8::type) == 8);
    REQUIRE(sizeof(KDS16::type) == 16);
}

TEST_CASE("strToPrefix, 2B, const char*", "[strToPrefix]")
{
    const char* str = "01";
    auto prefix = strToPrefix<KDS2::type>(str);
    REQUIRE((char)(prefix & 0xFF) == '1');
    REQUIRE((char)(prefix >> (8 * 1)) == '0');
}

TEST_CASE("strToPrefix, 4B, const char*", "[strToPrefix]")
{
    const char* str = "0001";
    auto prefix = strToPrefix<KDS4::type>(str);
    REQUIRE((char)(prefix & 0xFF) == '1');
    REQUIRE((char)(prefix >> (8 * 3)) == '0');
}

TEST_CASE("strToPrefix, 8B, const char*", "[strToPrefix]")
{
    const char* str = "00000001";
    auto prefix = strToPrefix<KDS8::type>(str);
    REQUIRE((char)(prefix & 0xFF) == '1');
    REQUIRE((char)(prefix >> (8 * 7)) == '0');
}

TEST_CASE("strToPrefix, 16B, const char*", "[strToPrefix]")
{
    const char* str = "0000000000000001";
    auto prefix = strToPrefix<KDS16::type>(str);
    REQUIRE((char)(prefix.lsbs & 0xFF) == '1');
    REQUIRE((char)(prefix.msbs >> (8 * 7)) == '0');
}

TEST_CASE("strToPrefix, 2B, std::string", "[strToPrefix]")
{
    string str = "01";
    auto prefix = strToPrefix<KDS2::type>(str);
    REQUIRE((char)(prefix & 0xFF) == '1');
    REQUIRE((char)(prefix >> (8 * 1)) == '0');
}

TEST_CASE("strToPrefix, 4B, std::string", "[strToPrefix]")
{
    string str = "0001";
    auto prefix = strToPrefix<KDS4::type>(str);
    REQUIRE((char)(prefix & 0xFF) == '1');
    REQUIRE((char)(prefix >> (8 * 3)) == '0');
}

TEST_CASE("strToPrefix, 8B, std::string", "[strToPrefix]")
{
    string str = "00000001";
    auto prefix = strToPrefix<KDS8::type>(str);
    REQUIRE((char)(prefix & 0xFF) == '1');
    REQUIRE((char)(prefix >> (8 * 7)) == '0');
}

TEST_CASE("strToPrefix, 16B, std::string", "[strToPrefix]")
{
    string str = "0000000000000001";
    auto prefix = strToPrefix<KDS16::type>(str);
    REQUIRE((char)(prefix.lsbs & 0xFF) == '1');
    REQUIRE((char)(prefix.msbs >> (8 * 7)) == '0');
}

TEST_CASE("flipBytes, 2B", "[flipBytes]")
{
    KDS2::type prefix = 0x0011;
    flipBytes(prefix);
    REQUIRE(prefix == 0x1100);
}

TEST_CASE("flipBytes, 4B", "[flipBytes]")
{
    KDS4::type prefix = 0x00112233;
    flipBytes(prefix);
    REQUIRE(prefix == 0x33221100);
}

TEST_CASE("flipBytes, 8B", "[flipBytes]")
{
    KDS8::type prefix = 0x0011223344556677;
    flipBytes(prefix);
    REQUIRE(prefix == 0x7766554433221100);
}

TEST_CASE("flipBytes, 16B", "[flipBytes]")
{
    KDS16::type prefix;
    prefix.msbs = 0x0011223344556677;
    prefix.lsbs = 0x8899AABBCCDDEEFF;
    flipBytes(prefix);
    REQUIRE(prefix.msbs == 0x7766554433221100);
    REQUIRE(prefix.lsbs == 0xFFEEDDCCBBAA9988);
}

TEST_CASE("Prefix, 4B, operator lt, long vs. long different keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaa"}, kdb{"bbbbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("Prefix, 4B, operator lt, long vs. long related keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaa"}, kdb{"aaaab"};
    REQUIRE(!(kda < kdb));
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("Prefix, 4B, operator lt, long vs. short keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaa"}, kdb{"bbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("Prefix, 4B, operator lt, short vs. short keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaa"}, kdb{"bbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("Prefix, 16B, operator lt, long vs. long different keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbbbbbbbbbbbbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("Prefix, 16B, operator lt, long vs. long related keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"aaaaaaaaaaaaaaaab"};
    REQUIRE(!(kda < kdb));
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("Prefix, 16B, operator lt, long vs. short keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("Prefix, 16B, operator lt, short vs. short keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaa"}, kdb{"bbbbbbbbbbbbb"};
    REQUIRE(kda < kdb);
    REQUIRE(!(kdb < kda));
    REQUIRE(!(kda < kda));
}

TEST_CASE("Prefix, 16B, operator eq, long vs. long different keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbbbbbbbbbbbbb"};
    REQUIRE(!(kda == kdb));
    REQUIRE(kda == kda);
}

TEST_CASE("Prefix, 16B, operator eq, long vs. long related keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"aaaaaaaaaaaaaaaab"};
    REQUIRE(kda == kdb);
    REQUIRE(kda == kda);
}

TEST_CASE("Prefix, 16B, operator eq, long vs. short keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbb"};
    REQUIRE(!(kda == kdb));
    REQUIRE(kda == kda);
}

TEST_CASE("Prefix, 16B, operator eq, short vs. short keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaa"}, kdb{"bbbbbbbbbbbbb"};
    REQUIRE(!(kda == kdb));
    REQUIRE(kda == kda);
}

TEST_CASE("Prefix, 16B, operator ne, long vs. long different keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbbbbbbbbbbbbb"};
    REQUIRE(kda != kdb);
    REQUIRE(!(kda != kda));
}

TEST_CASE("Prefix, 16B, operator ne, long vs. long related keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"aaaaaaaaaaaaaaaab"};
    REQUIRE(!(kda != kdb));
    REQUIRE(!(kda != kda));
}

TEST_CASE("Prefix, 16B, operator ne, long vs. short keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaaaaaaaaaaaaaaaa"}, kdb{"bbbbbb"};
    REQUIRE(kda != kdb);
    REQUIRE(!(kda != kda));
}

TEST_CASE("Prefix, 16B, operator ne, short vs. short keys", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> kda{"aaa"}, kdb{"bbbbbbbbbbbbb"};
    REQUIRE(kda != kdb);
    REQUIRE(!(kda != kda));
}

TEST_CASE("stringShorterThanKeydomet, 2B", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_16BIT> shorter{string(1, 's')}, longer{string(3, 'l')};
    CAPTURE((void*)(uintptr_t)shorter.getVal());
    REQUIRE(shorter.stringShorterThanKeydomet());
    CAPTURE((void*)(uintptr_t)longer.getVal());
    REQUIRE(!longer.stringShorterThanKeydomet());
}

TEST_CASE("stringShorterThanKeydomet, 4B", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_32BIT> shorter{string(3, 's')}, longer{string(4, 'l')};
    REQUIRE(shorter.stringShorterThanKeydomet());
    REQUIRE(!longer.stringShorterThanKeydomet());
}

TEST_CASE("stringShorterThanKeydomet, 8B", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_64BIT> shorter{string(7, 's')}, longer{string(8, 'l')};
    REQUIRE(shorter.stringShorterThanKeydomet());
    REQUIRE(!longer.stringShorterThanKeydomet());
}

TEST_CASE("stringShorterThanKeydomet, 16B", "[Prefix]")
{
    Prefix<KeydometSize::SIZE_128BIT> shorter{string(15, 's')}, longer{string(16, 'l')};
    REQUIRE(shorter.stringShorterThanKeydomet());
    REQUIRE(!longer.stringShorterThanKeydomet());
}

TEST_CASE("compare equal long keys, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kkkkkkkk"}, k2{"kkkkkkkk"};
    int res = k1.compare(k2);
    REQUIRE(res == 0);
}

TEST_CASE("compare equal short keys, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"k"}, k2{"k"};
    int res = k1.compare(k2);
    REQUIRE(res == 0);
}

TEST_CASE("compare k1 < k2, diff at prefix, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kkkkkkkk"}, k2{"llllllll"};
    int res = k1.compare(k2);
    REQUIRE(res < 0);
}

TEST_CASE("compare k1 > k2, diff at prefix, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kkkkkkkk"}, k2{"jjjjjjjj"};
    int res = k1.compare(k2);
    REQUIRE(res > 0);
}

TEST_CASE("compare k1 < k2, diff at suffix, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kkkkkkkk"}, k2{"kkkkllll"};
    int res = k1.compare(k2);
    REQUIRE(res < 0);
}

TEST_CASE("compare k1 > k2, diff at suffix, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kkkkkkkk"}, k2{"kkkkjjjj"};
    int res = k1.compare(k2);
    REQUIRE(res > 0);
}

TEST_CASE("compare k1 < k2, short keys, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kk"}, k2{"ll"};
    int res = k1.compare(k2);
    REQUIRE(res < 0);
}

TEST_CASE("compare k1 > k2, short keys, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kk"}, k2{"jj"};
    int res = k1.compare(k2);
    REQUIRE(res > 0);
}

TEST_CASE("operator< k1 < k2, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kkkkk"}, k2{"lllll"};
    bool res = k1 < k2;
    REQUIRE(res == true);
}

TEST_CASE("operator< k1 > k2, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kkkkk"}, k2{"jjjjj"};
    bool res = k1 < k2;
    REQUIRE(res == false);
}

TEST_CASE("operator< k1 == k2, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kkkkk"}, k2{"kkkkk"};
    bool res = k1 < k2;
    REQUIRE(res == false);
}

TEST_CASE("operator== k1 < k2, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kkkkk"}, k2{"lllll"};
    bool res = k1 == k2;
    REQUIRE(res == false);
}

TEST_CASE("operator== k1 > k2, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kkkkk"}, k2{"jjjjj"};
    bool res = k1 == k2;
    REQUIRE(res == false);
}

TEST_CASE("operator== k1 == k2, 4B", "[Keydomet]")
{
    Keydomet<const char*, KeydometSize::SIZE_32BIT> k1{"kkkkk"}, k2{"kkkkk"};
    bool res = k1 == k2;
    REQUIRE(res == true);
}

TEST_CASE("associative containers take a const str&", "[makeFindKey]")
{
    using KdmtStr = Keydomet<string, KeydometSize::SIZE_32BIT>;
    set<KdmtStr, less<>> s;
    string lookupStr{"dummy"};
    using FindKeyType = decltype(makeFindKey(s, lookupStr))::StrImp;
    constexpr bool isConstRef = is_same<FindKeyType, const string&>::value;
    CAPTURE(typeid(FindKeyType).name());
    REQUIRE(isConstRef == true);
}

TEST_CASE("associative container key requires no allocation", "[makeFindKey]")
{
    using KdmtStr = Keydomet<string, KeydometSize::SIZE_32BIT>;
    set<KdmtStr, less<>> s;
    string org{"dummy"};
    auto fk = makeFindKey(s, org);
    const string& ref = fk.getStr();
    REQUIRE(&ref == &org);
}

TEST_CASE("sorting multiple keys", "[containers]")
{
    vector<string> orgVals(10 + 100 + 1000);
    // generate single character strings (shorter than KeydometSize): "0" - "9"
    generate_n(orgVals.begin(),       10,  [n = 0] () mutable { return to_string(n++); });
    // generate two character strings (exactly KeydometSize): "00" - "99"
    generate_n(orgVals.begin() + 10,  10,  [n = 0] () mutable { return string{"0"} + to_string(n++); });
    generate_n(orgVals.begin() + 20,  90,  [n = 10] () mutable { return to_string(n++); });
    // generate three character strings (longer than KeydometSize): "000" - "999"
    generate_n(orgVals.begin() + 110, 10,  [n = 0] () mutable { return string{"00"} + to_string(n++); });
    generate_n(orgVals.begin() + 120, 90,  [n = 0] () mutable { return string{"0"} + to_string(n++); });
    generate_n(orgVals.begin() + 210, 900, [n = 100] () mutable { return to_string(n++); });
    // this is the reference sort
    sort(orgVals.begin(), orgVals.end());

    using KdmtView = Keydomet<string_view, KeydometSize::SIZE_16BIT>;
    vector<KdmtView> kdmVals;
    kdmVals.reserve(orgVals.size());
    transform(orgVals.begin(), orgVals.end(), back_inserter(kdmVals), [](const string& v) {
        return KdmtView{v.c_str()};
    });

    std::mt19937 g(std::random_device{}());

    for (int i = 0; i < 100; ++i)
    {
        std::shuffle(kdmVals.begin(), kdmVals.end(), g);
        sort(kdmVals.begin(), kdmVals.end());
        bool eq = equal(orgVals.begin(), orgVals.end(), kdmVals.begin(), kdmVals.end(),
                [](const string& org, const KdmtView& kdm) {
            return kdm.getStr() == org;
        });
        CAPTURE(orgVals);
        CAPTURE(kdmVals);
        REQUIRE(eq);
    }
}

TEST_CASE("populate keydomet set", "[containers]")
{
    using KdmtStr = Keydomet<std::string, KeydometSize::SIZE_16BIT>;
    set<KdmtStr, less<>> kdmtSet;
    vector<string> input({"Ac", "Jg", "OE", "S_", "Uv", "ak", "bT", "in", "s^", "xy"});

    for (size_t i = 0; i < input.size(); ++i)
    {
        kdmtSet.insert(KdmtStr{input[i]});
        REQUIRE(kdmtSet.size() == i + 1);
    }
}

static string getRandKey(size_t len)
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
static string dumpContainer(const T& container)
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
    using KdmtStr = Keydomet<std::string, KeydometSize::SIZE_16BIT>;
    set<KdmtStr, less<>> kdmtSet;
    set<string> strSet;

    constexpr size_t InputsNum = 10, LookupsNum = 10, KeyLen = 2;
    for (size_t i = 0; i < InputsNum; ++i)
    {
        string key = getRandKey(KeyLen);
        kdmtSet.insert(KdmtStr{key});
        strSet.insert(key);
    }
    CHECK(kdmtSet.size() == strSet.size());
    for (size_t i = 0; i < LookupsNum; ++i)
    {
        string key = getRandKey(KeyLen);
        bool foundKdmt = kdmtSet.find(KdmtStr{key}) == kdmtSet.end();
        bool foundStr = strSet.find(key) == strSet.end();
        if (foundKdmt != foundStr)
        {
            string kdmtDump = dumpContainer(kdmtSet);
            string strDump = dumpContainer(strSet);
            CAPTURE(kdmtDump);
            CAPTURE(strDump);
            FAIL();
        }
        REQUIRE(foundKdmt == foundStr);
    }
}

class MyString
{
public:
    MyString(const string& s) : str(s) {}
    bool operator<(const MyString& other) const {
        return str < other.str;
    }
    bool operator==(const MyString& other) const {
        return str == other.str;
    }
    friend const char* getRawStr(const MyString& s)
    {
        return s.str.c_str();
    }

private:
    string str;
};

TEST_CASE("using keydomet with non-standard strings", "[custom strings]")
{
    using KdmtMyStr = Keydomet<MyString, KeydometSize::SIZE_16BIT>;
    set<KdmtMyStr, less<>> kdmtSet;
    MyString str1("str1"), str2("str2"), str3("str3");
    kdmtSet.insert(str3);
    kdmtSet.insert(str1);
    kdmtSet.insert(str2);
    auto iter = kdmtSet.begin();
    REQUIRE(iter->getStr() == str1);
    ++iter;
    REQUIRE(iter->getStr() == str2);
    ++iter;
    REQUIRE(iter->getStr() == str3);
}