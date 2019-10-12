//
// Copyright(c) 2019 Eran Gilad, https://github.com/erangi/kdmt
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include <random>
#include <unordered_map>
#include <cstdint>
#include <sstream>
#include <atomic>
#if (__cplusplus < 201703L) && !(defined(__clang__) && __clang_major__ > 7)
    #include <experimental/string_view>
    using std::experimental::string_view;
#else
    #include <string_view>
#endif

#include "Keydomet.h"
#include "InputProvider.h"

#include "benchmark/benchmark.h"

using namespace std;
using namespace kdmt;

template<template<class, class...> class Container, class StrType, class... Args>
bool lookup(Container<StrType, Args...>& s, const string& key)
{
    auto iter = s.find(key);
    return iter != s.end();
}

// specialized for keydomet-ed containers, which take a keydomet for lookups
template<template<class, class... KdmtArgs> class Container, class KdmtStrT, prefix_size KdmtSize, class... Args>
bool lookup(Container<keydomet<KdmtStrT, KdmtSize>, Args...>& s, const string& key)
{
    auto hkey = make_find_key(s, key);
    auto iter = s.find(hkey);
    return iter != s.end();
}

enum ops { Lookups, Mix };
enum sso { Use, Exceed };

struct container_size { int64_t v; };
struct op_keys_num { int64_t v; };

template<prefix_size KdmtSize, class StrT, bool SsoOpt>
void keydomet_bench(benchmark::State& state, ops ops_mix, container_size container_size, op_keys_num op_key_num,
        input_provider<keydomet<StrT, KdmtSize, SsoOpt>>& input)
{
    using kdmt_str = keydomet<StrT, KdmtSize, SsoOpt>;
    const size_t prev_used_prefix = kdmt_str::used_prefix();
    const size_t prev_used_str = kdmt_str::used_string();
    set<kdmt_str, less<>> container(input.get_container(container_size.v));
    const vector<string>& op_keys = input.get_keys(op_key_num.v, keys_use::BENCH_OPS);
    size_t ops = 0, found = 0;
    if (ops_mix == ops::Lookups)
    {
        for (auto _ : state)
        {
            auto find_key = make_key_view(container, op_keys[ops++ % op_keys.size()]);
            static_assert(is_same<decltype(find_key), keydomet<const StrT&, KdmtSize, SsoOpt>>::value, "");
            found += container.find(find_key) != container.end() ? 1 : 0;
        }
    }
    else
    {
        for (auto _ : state)
        {
            const string& op_key = op_keys[ops++ % op_keys.size()];
            if (ops & 0x1)
            {
                auto find_key = make_key_view(container, op_key);
                static_assert(is_same<decltype(find_key), keydomet<const StrT&, KdmtSize, SsoOpt>>::value, "");
                found += container.find(find_key) != container.end() ? 1 : 0;
            }
            else
            {
                if (ops & 0x10)
                {
                    auto del_key = make_key_view(container, op_key);
                    static_assert(is_same<decltype(del_key), keydomet<const StrT&, KdmtSize, SsoOpt>>::value, "");
                    auto iter = container.find(del_key);
                    if (iter != container.end())
                        container.erase(iter);
                }
                else
                {
                    container.insert(kdmt_str{op_key});
                }
            }
        }
    }
    state.counters["1-lookups_found"] = benchmark::Counter{(double)found, benchmark::Counter::kAvgIterations};
    double kdmt_use_rate = double(kdmt_str::used_prefix() - prev_used_prefix) /
            ((kdmt_str::used_prefix() - prev_used_prefix) + (kdmt_str::used_string() - prev_used_str));
    state.counters["2-keydomet_use_rate"] = kdmt_use_rate;
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
void string_bench(benchmark::State& state, ops ops_mix, container_size container_size, op_keys_num op_key_num,
        input_provider<string>& input)
{
    const set<string, less<>>& str_container = input.get_container(container_size.v);
    set<StrT, less<>> container{str_container.begin(), str_container.end()};
    const vector<string>& op_keys = input.get_keys(op_key_num.v, keys_use::BENCH_OPS);
    size_t ops = 0, found = 0;
    if (ops_mix == ops::Lookups)
    {
        for (auto _ : state)
        {
            found += container.find(op_keys[ops++ % op_keys.size()]) != container.end() ? 1 : 0;
        }
    }
    else
    {
        for (auto _ : state)
        {
            const string& op_key = op_keys[ops++ % op_keys.size()];
            if (ops & 0x1)
            {
                found += container.find(op_key) != container.end() ? 1 : 0;
            }
            else
            {
                if (ops & 0x10)
                {
                    container.erase(op_key);
                }
                else
                {
                    container.insert(op_key);
                }
            }
        }
    }
    state.counters["1-lookups_found"] = benchmark::Counter{(double)found, benchmark::Counter::kAvgIterations};
}

const char* datasetFile = "datasets/2.5M keys.csv";

template<typename StrT>
void get_rand_bench_args(const benchmark::State& state, sso is_sso,
        container_size& container_size, op_keys_num& op_key_num, std::unique_ptr<input_provider<StrT>>& provider)
{
    container_size.v = state.range(0);
    op_key_num.v = state.range(1);
    uint32_t key_len = calc_key_len(max(container_size.v, op_key_num.v));
    size_t extra_len = is_sso == sso::Use ? 0 : sizeof(string) - key_len;
    provider = get_rand_input<StrT>(key_len, extra_len);
}

void BM_WarmupSsoOn(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<string>> provider;
    get_rand_bench_args(state, sso::Use, container_size, op_key_num, provider);
    string_bench<string>(state, ops::Lookups, container_size, op_key_num, *provider);
    cerr << "Types sizes:\n" <<
            "\tstd::string = " << sizeof(string) << "B\n" <<
            "\tstd::string_view = " << sizeof(string_view) << "B\n" <<
            "\tkeydomet<string, 4B> = " << sizeof(keydomet<string, prefix_size::SIZE_32BIT>) << "B" << endl;
}

void BM_WarmupSsoOff(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<string>> provider;
    get_rand_bench_args(state, sso::Exceed, container_size, op_key_num, provider);
    string_bench<string>(state, ops::Lookups, container_size, op_key_num, *provider);
}

void BM_WarmupDataset(benchmark::State& state)
{
    auto provider = get_dataset_input<string>(datasetFile);
    string_bench<string>(state, ops::Lookups, container_size{state.range(0)}, op_keys_num{state.range(1)}, *provider);
}

template<prefix_size KdmtSize, class StrT, bool SsoOpt>
void BM_KeydometAllOpsSsoOn(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<keydomet<StrT, KdmtSize, SsoOpt>>> provider;
    get_rand_bench_args(state, sso::Use, container_size, op_key_num, provider);
    keydomet_bench<KdmtSize, StrT, SsoOpt>(state, ops::Mix, container_size, op_key_num, *provider);
}

template<prefix_size KdmtSize, class StrT, bool SsoOpt>
void BM_KeydometAllOpsSsoOff(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<keydomet<StrT, KdmtSize, SsoOpt>>> provider;
    get_rand_bench_args(state, sso::Exceed, container_size, op_key_num, provider);
    keydomet_bench<KdmtSize, StrT, SsoOpt>(state, ops::Mix, container_size, op_key_num, *provider);
}

template<prefix_size KdmtSize, class StrT, bool SsoOpt>
void BM_KeydometAllOpsDataset(benchmark::State& state)
{
    auto provider = get_dataset_input<keydomet<StrT, KdmtSize, SsoOpt>>(datasetFile);
    keydomet_bench<KdmtSize, StrT, SsoOpt>(state, ops::Mix, container_size{state.range(0)}, op_keys_num{state.range(1)}, *provider);
}

template<prefix_size KdmtSize, class StrT, bool SsoOpt>
void BM_KeydometLookupsDataset(benchmark::State& state)
{
    auto provider = get_dataset_input<keydomet<StrT, KdmtSize, SsoOpt>>(datasetFile);
    keydomet_bench<KdmtSize, StrT, SsoOpt>(state, ops::Lookups, container_size{state.range(0)}, op_keys_num{state.range(1)}, *provider);
}

template<prefix_size KdmtSize, class StrT, bool SsoOpt>
void BM_KeydometLookupsSsoOn(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<keydomet<StrT, KdmtSize, SsoOpt>>> provider;
    get_rand_bench_args(state, sso::Use, container_size, op_key_num, provider);
    keydomet_bench<KdmtSize, StrT, SsoOpt>(state, ops::Lookups, container_size, op_key_num, *provider);
}

template<prefix_size KdmtSize, class StrT, bool SsoOpt>
void BM_KeydometLookupsSsoOff(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<keydomet<StrT, KdmtSize, SsoOpt>>> provider;
    get_rand_bench_args(state, sso::Exceed, container_size, op_key_num, provider);
    keydomet_bench<KdmtSize, StrT, SsoOpt>(state, ops::Lookups, container_size, op_key_num, *provider);
}

void BM_StringAllOpsSsoOn(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<string>> provider;
    get_rand_bench_args(state, sso::Use, container_size, op_key_num, provider);
    string_bench<string>(state, ops::Mix, container_size, op_key_num, *provider);
}

void BM_StringAllOpsSsoOff(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<string>> provider;
    get_rand_bench_args(state, sso::Exceed, container_size, op_key_num, provider);
    string_bench<string>(state, ops::Mix, container_size, op_key_num, *provider);
}

void BM_StringLookupsSsoOn(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<string>> provider;
    get_rand_bench_args(state, sso::Use, container_size, op_key_num, provider);
    string_bench<string>(state, ops::Lookups, container_size, op_key_num, *provider);
}

void BM_StringLookupsSsoOff(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<string>> provider;
    get_rand_bench_args(state, sso::Exceed, container_size, op_key_num, provider);
    string_bench<string>(state, ops::Lookups, container_size, op_key_num, *provider);
}

void BM_StringAllOpsDataset(benchmark::State& state)
{
    auto provider = get_dataset_input<string>(datasetFile);
    string_bench<string>(state, ops::Mix, container_size{state.range(0)}, op_keys_num{state.range(1)}, *provider);
}

void BM_StringLookupsDataset(benchmark::State& state)
{
    auto provider = get_dataset_input<string>(datasetFile);
    string_bench<string>(state, ops::Lookups, container_size{state.range(0)}, op_keys_num{state.range(1)}, *provider);
}

void BM_StringViewLookups(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<string>> provider;
    get_rand_bench_args(state, sso::Use, container_size, op_key_num, provider);
    string_bench<string_view>(state, ops::Lookups, container_size, op_key_num, *provider);
}

void BM_StringViewAllOps(benchmark::State& state)
{
    container_size container_size;
    op_keys_num op_key_num;
    std::unique_ptr<input_provider<string>> provider;
    get_rand_bench_args(state, sso::Use, container_size, op_key_num, provider);
    string_bench<string_view>(state, ops::Mix, container_size, op_key_num, *provider);
}

void BM_StringViewLookupsDataset(benchmark::State& state)
{
    auto provider = get_dataset_input<string>(datasetFile);
    string_bench<string_view>(state, ops::Lookups, container_size{state.range(0)}, op_keys_num{state.range(1)}, *provider);
}

void BM_StringViewAllOpsDataset(benchmark::State& state)
{
    auto provider = get_dataset_input<string>(datasetFile);
    string_bench<string_view>(state, ops::Mix, container_size{state.range(0)}, op_keys_num{state.range(1)}, *provider);
}

template<prefix_size KdmtSize>
void BM_KeydometCreation(benchmark::State& state)
{
    using keydomet_type = typename prefix_storage<KdmtSize>::type;
    size_t strLen = state.range(0);
    string source(strLen, 'e');
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(
                str_to_prefix<keydomet_type>(source)
        );
    }
}

//constexpr size_t IterationsNum = 3'000;
//constexpr size_t container_size = 2'000;
//constexpr size_t OpsKeysNumber = 3'000;
constexpr size_t IterationsNum = 300'000;
constexpr size_t container_size = 200'000;
constexpr size_t OpsKeysNumber = 300'000;
//constexpr size_t IterationsNum = 1'000'000;
//constexpr size_t container_size = 1'000'000;
//constexpr size_t OpsKeysNumber = 1'000'000;
//constexpr size_t IterationsNum = 1'500'000;
//constexpr size_t container_size = 1'000'000;
//constexpr size_t OpsKeysNumber = 1'500'000;
//constexpr size_t IterationsNum = 3'000'000;
//constexpr size_t container_size = 2'000'000;
//constexpr size_t OpsKeysNumber = 3'000'000;

constexpr size_t Repeats = 5;
constexpr prefix_size BenchKdmtSize = prefix_size::SIZE_32BIT; //TODO consider benchmarking all sizes

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
#define BENCH_SsoOpt            1

#define BenchConfig(reps) \
        -> Iterations(IterationsNum) \
        -> Args({container_size, OpsKeysNumber}) \
        -> Repetitions(reps) \
        -> ReportAggregatesOnly(true)

#define KdmtCreationConf() \
         -> Range(1, 128)

#if BENCH_KeydometCreation
BENCHMARK_TEMPLATE(BM_KeydometCreation, prefix_size::SIZE_16BIT) KdmtCreationConf();
BENCHMARK_TEMPLATE(BM_KeydometCreation, prefix_size::SIZE_32BIT) KdmtCreationConf();
BENCHMARK_TEMPLATE(BM_KeydometCreation, prefix_size::SIZE_64BIT) KdmtCreationConf();
BENCHMARK_TEMPLATE(BM_KeydometCreation, prefix_size::SIZE_128BIT) KdmtCreationConf();
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
BENCHMARK_TEMPLATE(BM_KeydometLookupsSsoOn, BenchKdmtSize, std::string, false) BenchConfig(Repeats);
#if BENCH_SsoOpt
BENCHMARK_TEMPLATE(BM_KeydometLookupsSsoOn, BenchKdmtSize, std::string, true) BenchConfig(Repeats);
#endif
#endif // BENCH_SsoOn
#if BENCH_SsoOff
BENCHMARK_TEMPLATE(BM_KeydometLookupsSsoOff, BenchKdmtSize, std::string, false) BenchConfig(Repeats);
#if BENCH_SsoOpt
BENCHMARK_TEMPLATE(BM_KeydometLookupsSsoOff, BenchKdmtSize, std::string, true) BenchConfig(Repeats);
#endif
#endif // BENCH_SsoOff
#endif // BENCH_LookupsOnly
#if BENCH_AllOps
#if BENCH_SsoOn
BENCHMARK_TEMPLATE(BM_KeydometAllOpsSsoOn, BenchKdmtSize, std::string, false) BenchConfig(Repeats);
#if BENCH_SsoOpt
BENCHMARK_TEMPLATE(BM_KeydometAllOpsSsoOn, BenchKdmtSize, std::string, true) BenchConfig(Repeats);
#endif
#endif // BENCH_SsoOn
#if BENCH_SsoOff
BENCHMARK_TEMPLATE(BM_KeydometAllOpsSsoOff, BenchKdmtSize, std::string, false) BenchConfig(Repeats);
#if BENCH_SsoOpt
BENCHMARK_TEMPLATE(BM_KeydometAllOpsSsoOff, BenchKdmtSize, std::string, true) BenchConfig(Repeats);
#endif
#endif // BENCH_SsoOff
#endif // BENCH_AllOps
#endif // BENCH_RandInput
#if BENCH_Dataset
#if BENCH_LookupsOnly
BENCHMARK_TEMPLATE(BM_KeydometLookupsDataset, BenchKdmtSize, std::string, false) BenchConfig(Repeats);
#if BENCH_SsoOpt
BENCHMARK_TEMPLATE(BM_KeydometLookupsDataset, BenchKdmtSize, std::string, true) BenchConfig(Repeats);
#endif
#endif // BENCH_LookupsOnly
#if BENCH_AllOps
BENCHMARK_TEMPLATE(BM_KeydometAllOpsDataset, BenchKdmtSize, std::string, false) BenchConfig(Repeats);
#if BENCH_SsoOpt
BENCHMARK_TEMPLATE(BM_KeydometAllOpsDataset, BenchKdmtSize, std::string, true) BenchConfig(Repeats);
#endif
#endif // BENCH_AllOps
#endif // BENCH_Dataset
#endif // BENCH_Keydomet

class ConsoleReporter2 : public ::benchmark::ConsoleReporter {

private:
    void PrintRunData(const Run& report) override;

};

void ConsoleReporter2::PrintRunData(const Run & run) {
    string run_name = run.benchmark_name();
    if (run_name.find("/repeats:") != run_name.npos &&
            run_name.find("_mean") == run_name.npos &&
            run_name.find("_median") == run_name.npos)
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
