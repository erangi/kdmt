
#include "Timer.h"
#include "Keydomet.h"

#include <set>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <random>

using namespace std;
using namespace kdmt;

//constexpr auto KeydometSizeToUse = KeydometSize::SIZE_128BIT;
//constexpr auto KeydometSizeToUse = KeydometSize::SIZE_64BIT;
constexpr auto KeydometSizeToUse = KeydometSize::SIZE_32BIT;
//constexpr auto KeydometSizeToUse = KeydometSize::SIZE_16BIT;

using KeydometStr = Keydomet<std::string, KeydometSizeToUse>;

using KdmtSet = set<KeydometStr, less<>>;

string getRandStr(size_t len)
{
    static mt19937 gen{random_device{}()};
    uniform_int_distribution<short> dis('A', 'z');
    string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i)
        s += (char)dis(gen);
    return s;
}

vector<string> getInput(size_t keysNum, size_t keyLen)
{
    vector<string> input{keysNum};
    generate(input.begin(), input.end(), [keyLen] {
        return getRandStr(keyLen);
    });
    return input;
}

template<template<class, typename...> class Container, typename StrT, KeydometSize Size, typename... ContainerArgs>
void buildContainer(Container<Keydomet<StrT, Size>, ContainerArgs...>& container, const vector<string>& input)
{
    transform(input.begin(), input.end(), std::inserter(container, container.begin()), [](const string& str) {
        return Keydomet<StrT, Size>{str};
    });
}

template<template<class, typename...> class Container, typename... ContainerArgs>
void buildContainer(Container<string, ContainerArgs...>& container, const vector<string>& input)
{
    transform(input.begin(), input.end(), std::inserter(container, container.begin()), [](const string& str) {
        return str;
    });
}

void print(const KdmtSet& s)
{
    cout << "set has " << s.size() << " keys:" << endl;
    for (const KeydometStr& hk : s)
        cout << hk << " --> " << hk.getPrefix().getVal() << endl;
}

template<template<class, class...> class Container, class StrType, class... Args>
bool lookup(Container<StrType, Args...>& s, const string& key)
{
    auto iter = s.find(key);
    return iter != s.end();
}

// specialized for keydomet-ed containers, which take a keydomet for lookups
template<template<class, class...> class Container, class... Args>
bool lookup(Container<KeydometStr, Args...>& s, const string& key)
{
    auto hkey = makeFindKey(s, key);
    auto iter = s.find(hkey);
    return iter != s.end();
}

template<class KeydometCont, class StdStringCont>
void benchmark(const vector<string>& input, const vector<string>& lookups)
{
    KeydometCont pkc;
    StdStringCont ssc;
    cout << "building containers..." << endl;
    buildContainer(pkc, input);
    buildContainer(ssc, input);
    cout << "running series of " << lookups.size() << " lookups..." << endl;
    TimerMS timer(TimerStart::Now);
    for (const string& s : lookups)
        lookup(pkc, s);
    auto elapsed = timer.elapsedStr();
    cout << "prefixed strings: " << elapsed << endl;
    cout << "\tused keydomet: " << KeydometStr::usedPrefix() << ", used str: " << KeydometStr::usedString() << endl;
    timer.start();
    for (const string& s : lookups)
        lookup(ssc, s);
    elapsed = timer.elapsedStr();
    cout << "regular strings: " << elapsed << endl;
    cout << "benchmark completed." << endl;
}

int main()
{
    const size_t inputSize = 1'000'000,
            lookupsNum = 1'000'000,
            strLen = 16;
    cout << "String types sizes: Keydomet = " << sizeof(KeydometStr) << "B (" << (int)KeydometSizeToUse <<
            "B keydomet), std::string = " << sizeof(string) << "B" << endl;
    cout << "generating " << inputSize << " input strings of size " << strLen << "..." << endl;
    vector<string> input = getInput(inputSize, strLen);
    cout << "generating " << lookupsNum << " lookup strings of size " << strLen << "..." << endl;
    vector<string> lookups = getInput(lookupsNum, strLen);
    cout << "=== testing sets ===" << endl;
    benchmark<KdmtSet, set<string>>(input, lookups);
    benchmark<KdmtSet, set<string>>(input, lookups);
    benchmark<KdmtSet, set<string>>(input, lookups);
    return 0;
}