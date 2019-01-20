//
// Copyright(c) 2019 Eran Gilad, https://github.com/erangi/kdmt
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include <random>
#include <unordered_map>
#include <cstdint>
#include <sstream>
#include <atomic>
#include <experimental/string_view>

#include "Keydomet.h"
#include "InputProvider.h"

#include "benchmark/benchmark.h"

using namespace std;
using namespace kdmt;
using experimental::string_view;

template<template<class, class...> class Container, class StrType, class... Args>
bool lookup(Container<StrType, Args...>& s, const string& key)
{
    auto iter = s.find(key);
    return iter != s.end();
}

// specialized for keydomet-ed containers, which take a keydomet for lookups
template<template<class, class... KdmtArgs> class Container, class KdmtStrT, KeydometSize KdmtSize, class... Args>
bool lookup(Container<Keydomet<KdmtStrT, KdmtSize>, Args...>& s, const string& key)
{
    auto hkey = makeFindKey(s, key);
    auto iter = s.find(hkey);
    return iter != s.end();
}

enum Ops { Lookups, Mix };
enum SSO { Use, Exceed };

struct ContainerSize { int64_t v; };
struct OpKeysNum { int64_t v; };

template<KeydometSize KdmtSize, class StrT>
void KeydometBench(benchmark::State& state, Ops opsMix, ContainerSize containerSize, OpKeysNum opKeysNum,
        InputProvider<Keydomet<StrT, KdmtSize>>& input)
{
    using KdmtStr = Keydomet<StrT, KdmtSize>;
    const size_t prevUsedPrefix = KdmtStr::usedPrefix();
    const size_t prevUsedStr = KdmtStr::usedString();
    set<KdmtStr, less<>> container(input.getContainer(containerSize.v));
    const vector<string>& opKeys = input.getKeys(opKeysNum.v, KeysUse::BENCH_OPS);
    size_t ops = 0, found = 0;
    if (opsMix == Ops::Lookups)
    {
        for (auto _ : state)
        {
            auto findKey = makeFindKey(container, opKeys[ops++ % opKeys.size()]);
            static_assert(is_same<decltype(findKey), Keydomet<const StrT&, KdmtSize>>::value, "");
            found += container.find(findKey) != container.end() ? 1 : 0;
        }
    }
    else
    {
        for (auto _ : state)
        {
            const string& opKey = opKeys[ops++ % opKeys.size()];
            if (ops & 0x1)
            {
                auto findKey = makeFindKey(container, opKey);
                static_assert(is_same<decltype(findKey), Keydomet<const StrT&, KdmtSize>>::value, "");
                found += container.find(findKey) != container.end() ? 1 : 0;
            }
            else
            {
                if (ops & 0x10)
                {
                    auto delKey = makeFindKey(container, opKey);
                    static_assert(is_same<decltype(delKey), Keydomet<const StrT&, KdmtSize>>::value, "");
                    auto iter = container.find(delKey);
                    if (iter != container.end())
                        container.erase(iter);
                }
                else
                {
                    container.insert(KdmtStr{opKey});
                }
            }
        }
    }
    state.counters["1-lookups_found"] = benchmark::Counter{(double)found, benchmark::Counter::kAvgIterations};
    double kdmtUseRate = double(KdmtStr::usedPrefix() - prevUsedPrefix) /
            ((KdmtStr::usedPrefix() - prevUsedPrefix) + (KdmtStr::usedString() - prevUsedStr));
    state.counters["2-keydomet_use_rate"] = kdmtUseRate;
}

template<template<typename...> class Container, typename... CArgs>
string dump(const Container<CArgs...>& container, string delim)
{
    stringstream ss;
    for (const auto& val : container)
        ss << val << delim;
    return ss.str();
}

template<typename StrT>
void StringBench(benchmark::State& state, Ops opsMix, ContainerSize containerSize, OpKeysNum opKeysNum,
        InputProvider<string>& input)
{
    const set<string, less<>>& strContainer = input.getContainer(containerSize.v);
    set<StrT, less<>> container{strContainer.begin(), strContainer.end()};
    const vector<string>& opKeys = input.getKeys(opKeysNum.v, KeysUse::BENCH_OPS);
    size_t ops = 0, found = 0;
    if (opsMix == Ops::Lookups)
    {
        for (auto _ : state)
        {
            found += container.find(opKeys[ops++ % opKeys.size()]) != container.end() ? 1 : 0;
        }
    }
    else
    {
        for (auto _ : state)
        {
            const string& opKey = opKeys[ops++ % opKeys.size()];
            if (ops & 0x1)
            {
                found += container.find(opKey) != container.end() ? 1 : 0;
            }
            else
            {
                if (ops & 0x10)
                {
                    container.erase(opKey);
                }
                else
                {
                    container.insert(opKey);
                }
            }
        }
    }
    state.counters["1-lookups_found"] = benchmark::Counter{(double)found, benchmark::Counter::kAvgIterations};
}

const char* datasetFile = "datasets/2.5M keys.csv";

template<typename StrT>
void getRandBenchArgs(const benchmark::State& state, SSO sso,
        ContainerSize& containerSize, OpKeysNum& opKeysNum, std::unique_ptr<InputProvider<StrT>>& provider)
{
    containerSize.v = state.range(0);
    opKeysNum.v = state.range(1);
    uint32_t keyLen = calcKeyLen(max(containerSize.v, opKeysNum.v));
    size_t extraLen = sso == SSO::Use ? 0 : sizeof(string) - keyLen;
    provider = getRandInput<StrT>(keyLen, extraLen);
}

void BM_WarmupSsoOn(benchmark::State& state)
{
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<string>> provider;
    getRandBenchArgs(state, SSO::Use, containerSize, opKeysNum, provider);
    StringBench<string>(state, Ops::Lookups, containerSize, opKeysNum, *provider);
}

void BM_WarmupSsoOff(benchmark::State& state)
{
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<string>> provider;
    getRandBenchArgs(state, SSO::Exceed, containerSize, opKeysNum, provider);
    StringBench<string>(state, Ops::Lookups, containerSize, opKeysNum, *provider);
}

void BM_WarmupDataset(benchmark::State& state)
{
    auto provider = getDatasetInput<string>(datasetFile);
    StringBench<string>(state, Ops::Lookups, ContainerSize{state.range(0)}, OpKeysNum{state.range(1)}, *provider);
}

template<KeydometSize KdmtSize, class StrT>
void BM_KeydometAllOpsSsoOn(benchmark::State& state)
{
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<Keydomet<StrT, KdmtSize>>> provider;
    getRandBenchArgs(state, SSO::Use, containerSize, opKeysNum, provider);
    KeydometBench<KdmtSize, StrT>(state, Ops::Mix, containerSize, opKeysNum, *provider);
}

template<KeydometSize KdmtSize, class StrT>
void BM_KeydometAllOpsSsoOff(benchmark::State& state)
{
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<Keydomet<StrT, KdmtSize>>> provider;
    getRandBenchArgs(state, SSO::Exceed, containerSize, opKeysNum, provider);
    KeydometBench<KdmtSize, StrT>(state, Ops::Mix, containerSize, opKeysNum, *provider);
}

template<KeydometSize KdmtSize, class StrT>
void BM_KeydometAllOpsDataset(benchmark::State& state)
{
    auto provider = getDatasetInput<Keydomet<StrT, KdmtSize>>(datasetFile);
    KeydometBench<KdmtSize, StrT>(state, Ops::Mix, ContainerSize{state.range(0)}, OpKeysNum{state.range(1)}, *provider);
}

template<KeydometSize KdmtSize, class StrT>
void BM_KeydometLookupsDataset(benchmark::State& state)
{
    auto provider = getDatasetInput<Keydomet<StrT, KdmtSize>>(datasetFile);
    KeydometBench<KdmtSize, StrT>(state, Ops::Lookups, ContainerSize{state.range(0)}, OpKeysNum{state.range(1)}, *provider);
}

template<KeydometSize KdmtSize, class StrT>
void BM_KeydometLookupsSsoOn(benchmark::State& state)
{
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<Keydomet<StrT, KdmtSize>>> provider;
    getRandBenchArgs(state, SSO::Use, containerSize, opKeysNum, provider);
    KeydometBench<KdmtSize, StrT>(state, Ops::Lookups, containerSize, opKeysNum, *provider);
}

template<KeydometSize KdmtSize, class StrT>
void BM_KeydometLookupsSsoOff(benchmark::State& state)
{
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<Keydomet<StrT, KdmtSize>>> provider;
    getRandBenchArgs(state, SSO::Exceed, containerSize, opKeysNum, provider);
    KeydometBench<KdmtSize, StrT>(state, Ops::Lookups, containerSize, opKeysNum, *provider);
}

void BM_StringAllOpsSsoOn(benchmark::State& state)
{
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<string>> provider;
    getRandBenchArgs(state, SSO::Use, containerSize, opKeysNum, provider);
    StringBench<string>(state, Ops::Mix, containerSize, opKeysNum, *provider);
}

void BM_StringAllOpsSsoOff(benchmark::State& state)
{
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<string>> provider;
    getRandBenchArgs(state, SSO::Exceed, containerSize, opKeysNum, provider);
    StringBench<string>(state, Ops::Mix, containerSize, opKeysNum, *provider);
}

void BM_StringLookupsSsoOn(benchmark::State& state)
{
    cerr << "state.counters.size() = " << state.counters.size() << endl;//XXX
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<string>> provider;
    getRandBenchArgs(state, SSO::Use, containerSize, opKeysNum, provider);
    StringBench<string>(state, Ops::Lookups, containerSize, opKeysNum, *provider);
}

void BM_StringLookupsSsoOff(benchmark::State& state)
{
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<string>> provider;
    getRandBenchArgs(state, SSO::Exceed, containerSize, opKeysNum, provider);
    StringBench<string>(state, Ops::Lookups, containerSize, opKeysNum, *provider);
}

void BM_StringAllOpsDataset(benchmark::State& state)
{
    auto provider = getDatasetInput<string>(datasetFile);
    StringBench<string>(state, Ops::Mix, ContainerSize{state.range(0)}, OpKeysNum{state.range(1)}, *provider);
}

void BM_StringLookupsDataset(benchmark::State& state)
{
    auto provider = getDatasetInput<string>(datasetFile);
    StringBench<string>(state, Ops::Lookups, ContainerSize{state.range(0)}, OpKeysNum{state.range(1)}, *provider);
}

void BM_StringViewLookups(benchmark::State& state)
{
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<string>> provider;
    getRandBenchArgs(state, SSO::Use, containerSize, opKeysNum, provider);
    StringBench<string_view>(state, Ops::Lookups, containerSize, opKeysNum, *provider);
}

void BM_StringViewAllOps(benchmark::State& state)
{
    ContainerSize containerSize;
    OpKeysNum opKeysNum;
    std::unique_ptr<InputProvider<string>> provider;
    getRandBenchArgs(state, SSO::Use, containerSize, opKeysNum, provider);
    StringBench<string_view>(state, Ops::Mix, containerSize, opKeysNum, *provider);
}

void BM_StringViewLookupsDataset(benchmark::State& state)
{
    auto provider = getDatasetInput<string>(datasetFile);
    StringBench<string_view>(state, Ops::Lookups, ContainerSize{state.range(0)}, OpKeysNum{state.range(1)}, *provider);
}

void BM_StringViewAllOpsDataset(benchmark::State& state)
{
    auto provider = getDatasetInput<string>(datasetFile);
    StringBench<string_view>(state, Ops::Mix, ContainerSize{state.range(0)}, OpKeysNum{state.range(1)}, *provider);
}

template<KeydometSize KdmtSize>
void BM_KeydometCreation(benchmark::State& state)
{
    using KeydometType = typename KeydometStorage<KdmtSize>::type;
    size_t strLen = state.range(0);
    string source(strLen, 'e');
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(
                strToPrefix<KeydometType>(source)
        );
    }
}

//constexpr size_t IterationsNum = 3'000;
//constexpr size_t ContainerSize = 2'000;
//constexpr size_t OpsKeysNumber = 3'000;
constexpr size_t IterationsNum = 300'000;
constexpr size_t ContainerSize = 200'000;
constexpr size_t OpsKeysNumber = 300'000;
//constexpr size_t IterationsNum = 1'000'000;
//constexpr size_t ContainerSize = 1'000'000;
//constexpr size_t OpsKeysNumber = 1'000'000;
//constexpr size_t IterationsNum = 1'500'000;
//constexpr size_t ContainerSize = 1'000'000;
//constexpr size_t OpsKeysNumber = 1'500'000;
//constexpr size_t IterationsNum = 3'000'000;
//constexpr size_t ContainerSize = 2'000'000;
//constexpr size_t OpsKeysNumber = 3'000'000;

constexpr size_t Repeats = 5;
constexpr KeydometSize BenchKdmtSize = KeydometSize::SIZE_32BIT; //TODO consider benchmarking all sizes

#define BENCH_KeydometCreation  1
#define BENCH_Warmup            1
#define BENCH_StdString         1
#define BENCH_StdStringView     1
#define BENCH_Keydomet          1
#define BENCH_LookupsOnly       1
#define BENCH_AllOps            1
#define BENCH_SsoOn             1
#define BENCH_SsoOff            1
#define BENCH_RandInput         1
#define BENCH_Dataset           1

#define BenchConfig(reps) \
        -> Iterations(IterationsNum) \
        -> Args({ContainerSize, OpsKeysNumber}) \
        -> Repetitions(reps) \
        -> ReportAggregatesOnly(true)

#define KdmtCreationConf() \
         -> Range(1, 128)

#if BENCH_KeydometCreation
BENCHMARK_TEMPLATE(BM_KeydometCreation, KeydometSize::SIZE_16BIT) KdmtCreationConf();
BENCHMARK_TEMPLATE(BM_KeydometCreation, KeydometSize::SIZE_32BIT) KdmtCreationConf();
BENCHMARK_TEMPLATE(BM_KeydometCreation, KeydometSize::SIZE_64BIT) KdmtCreationConf();
BENCHMARK_TEMPLATE(BM_KeydometCreation, KeydometSize::SIZE_128BIT) KdmtCreationConf();
#endif

#if BENCH_Warmup
#if BENCH_SsoOn && BENCH_RandInput
BENCHMARK(BM_WarmupSsoOn) BenchConfig(1);
#endif
#if BENCH_SsoOff && BENCH_RandInput
BENCHMARK(BM_WarmupSsoOff) BenchConfig(1);
#endif
#if BENCH_Dataset
BENCHMARK(BM_WarmupDataset) BenchConfig(1);
#endif
#endif // BENCH_Warmup

#if BENCH_StdString
#if BENCH_RandInput
#if BENCH_LookupsOnly
#if BENCH_SsoOn
BENCHMARK(BM_StringLookupsSsoOn) BenchConfig(Repeats);
#endif // BENCH_SsoOn
#if BENCH_SsoOff
BENCHMARK(BM_StringLookupsSsoOff) BenchConfig(Repeats);
#endif // BENCH_SsoOff
#endif // BENCH_LookupsOnly
#if BENCH_AllOps
#if BENCH_SsoOn
BENCHMARK(BM_StringAllOpsSsoOn) BenchConfig(Repeats);
#endif // BENCH_SsoOn
#if BENCH_SsoOff
BENCHMARK(BM_StringAllOpsSsoOff) BenchConfig(Repeats);
#endif // BENCH_SsoOff
#endif // BENCH_AllOps
#endif // BENCH_RandInput
#if BENCH_Dataset
#if BENCH_LookupsOnly
BENCHMARK(BM_StringLookupsDataset) BenchConfig(Repeats);
#endif // BENCH_LookupsOnly
#if BENCH_AllOps
BENCHMARK(BM_StringAllOpsDataset) BenchConfig(Repeats);
#endif // BENCH_AllOps
#endif // BENCH_Dataset
#endif // BENCH_StdString

#if BENCH_StdStringView
#if BENCH_RandInput
#if BENCH_LookupsOnly
BENCHMARK(BM_StringViewLookups) BenchConfig(Repeats);
#endif // BENCH_LookupsOnly
#if BENCH_AllOps
BENCHMARK(BM_StringViewAllOps) BenchConfig(Repeats);
#endif // BENCH_AllOps
#endif // BENCH_RandInput
#if BENCH_Dataset
#if BENCH_LookupsOnly
BENCHMARK(BM_StringViewLookupsDataset) BenchConfig(Repeats);
#endif // BENCH_LookupsOnly
#if BENCH_AllOps
BENCHMARK(BM_StringViewAllOpsDataset) BenchConfig(Repeats);
#endif // BENCH_AllOps
#endif // BENCH_Dataset
#endif // BENCH_StdStringView

#if BENCH_Keydomet
#if BENCH_RandInput
#if BENCH_LookupsOnly
#if BENCH_SsoOn
BENCHMARK_TEMPLATE(BM_KeydometLookupsSsoOn, BenchKdmtSize, std::string) BenchConfig(Repeats);
#endif // BENCH_SsoOn
#if BENCH_SsoOff
BENCHMARK_TEMPLATE(BM_KeydometLookupsSsoOff, BenchKdmtSize, std::string) BenchConfig(Repeats);
#endif // BENCH_SsoOff
#endif // BENCH_LookupsOnly
#if BENCH_AllOps
#if BENCH_SsoOn
BENCHMARK_TEMPLATE(BM_KeydometAllOpsSsoOn, BenchKdmtSize, std::string) BenchConfig(Repeats);
#endif // BENCH_SsoOn
#if BENCH_SsoOff
BENCHMARK_TEMPLATE(BM_KeydometAllOpsSsoOff, BenchKdmtSize, std::string) BenchConfig(Repeats);
#endif // BENCH_SsoOff
#endif // BENCH_AllOps
#endif // BENCH_RandInput
#if BENCH_Dataset
#if BENCH_LookupsOnly
BENCHMARK_TEMPLATE(BM_KeydometLookupsDataset, BenchKdmtSize, std::string) BenchConfig(Repeats);
#endif // BENCH_LookupsOnly
#if BENCH_AllOps
BENCHMARK_TEMPLATE(BM_KeydometAllOpsDataset, BenchKdmtSize, std::string) BenchConfig(Repeats);
#endif // BENCH_AllOps
#endif // BENCH_Dataset
#endif // BENCH_Keydomet

class ConsoleReporter2 : public ::benchmark::ConsoleReporter {

private:
    void PrintRunData(const Run& report) override;

};

void ConsoleReporter2::PrintRunData(const Run & run) {
    if (run.benchmark_name.find("/repeats:") != run.benchmark_name.npos &&
            run.benchmark_name.find("_mean") == run.benchmark_name.npos &&
            run.benchmark_name.find("_median") == run.benchmark_name.npos)
        return;

    ConsoleReporter::PrintRunData(run);
}

//BENCHMARK_MAIN();

int main(int argc, char** argv)
{
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;
    ::benchmark::RunSpecifiedBenchmarks(new ConsoleReporter2, nullptr);
}
