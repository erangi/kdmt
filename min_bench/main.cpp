//
// Copyright(c) 2019 Eran Gilad, https://github.com/erangi/kdmt
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Timer.h"
#include "Keydomet.h"

#include <set>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <random>

using namespace std;
using namespace kdmt;

//constexpr auto keydomet_size_to_use = prefix_size::SIZE_128BIT;
//constexpr auto keydomet_size_to_use = prefix_size::SIZE_64BIT;
constexpr auto keydomet_size_to_use = prefix_size::SIZE_32BIT;
//constexpr auto keydomet_size_to_use = prefix_size::SIZE_16BIT;

using keydomet_str = keydomet<std::string, keydomet_size_to_use>;

using kdmt_set = set<keydomet_str, less<>>;

string get_rand_str(size_t len)
{
    static mt19937 gen{random_device{}()};
    uniform_int_distribution<short> dis('A', 'z');
    string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i)
        s += (char)dis(gen);
    return s;
}

vector<string> get_input(size_t keys_num, size_t key_len)
{
    vector<string> input{keys_num};
    generate(input.begin(), input.end(), [key_len] {
        return get_rand_str(key_len);
    });
    return input;
}

template<template<class, typename...> class Container, typename StrT, prefix_size Size, typename... ContainerArgs>
void build_container(Container<keydomet<StrT, Size>, ContainerArgs...>& container, const vector<string>& input)
{
    transform(input.begin(), input.end(), std::inserter(container, container.begin()), [](const string& str) {
        return keydomet<StrT, Size>{str};
    });
}

template<template<class, typename...> class Container, typename... ContainerArgs>
void build_container(Container<string, ContainerArgs...>& container, const vector<string>& input)
{
    transform(input.begin(), input.end(), std::inserter(container, container.begin()), [](const string& str) {
        return str;
    });
}

void print(const kdmt_set& s)
{
    cout << "set has " << s.size() << " keys:" << endl;
    for (const keydomet_str& hk : s)
        cout << hk << " --> " << hk.getPrefix().get_val() << endl;
}

template<template<class, class...> class Container, class StrType, class... Args>
bool lookup(Container<StrType, Args...>& s, const string& key)
{
    auto iter = s.find(key);
    return iter != s.end();
}

// specialized for keydomet-ed containers, which take a keydomet for lookups
template<template<class, class...> class Container, class... Args>
bool lookup(Container<keydomet_str, Args...>& s, const string& key)
{
    auto hkey = make_find_key(s, key);
    auto iter = s.find(hkey);
    return iter != s.end();
}

template<class KeydometCont, class StdStringCont>
void benchmark(const vector<string>& input, const vector<string>& lookups)
{
    KeydometCont pkc;
    StdStringCont ssc;
    cout << "building containers..." << endl;
    build_container(pkc, input);
    build_container(ssc, input);
    cout << "running series of " << lookups.size() << " lookups..." << endl;
    timer_ms timer(timer_start::Now);
    for (const string& s : lookups)
        lookup(pkc, s);
    auto elapsed = timer.elapsed_str();
    cout << "prefixed strings: " << elapsed << endl;
    cout << "\tused keydomet: " << keydomet_str::used_prefix() << ", used str: " << keydomet_str::used_string() << endl;
    timer.start();
    for (const string& s : lookups)
        lookup(ssc, s);
    elapsed = timer.elapsed_str();
    cout << "regular strings: " << elapsed << endl;
    cout << "benchmark completed." << endl;
}

int main()
{
    const size_t input_size = 1'000'000,
            lookups_num = 1'000'000,
            str_len = 16;
    cout << "String types sizes: keydomet = " << sizeof(keydomet_str) << "B (" << (int)keydomet_size_to_use <<
            "B keydomet), std::string = " << sizeof(string) << "B" << endl;
    cout << "generating " << input_size << " input strings of size " << str_len << "..." << endl;
    vector<string> input = get_input(input_size, str_len);
    cout << "generating " << lookups_num << " lookup strings of size " << str_len << "..." << endl;
    vector<string> lookups = get_input(lookups_num, str_len);
    cout << "=== testing sets ===" << endl;
    // benchmark<kdmt_set, set<string>>(input, lookups);
    // benchmark<kdmt_set, set<string>>(input, lookups);
    // benchmark<kdmt_set, set<string>>(input, lookups);
    return 0;
}