//
// Copyright(c) 2019 Eran Gilad, https://github.com/erangi/kdmt
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef KEYDOMET_INPUTPROVIDER_H
#define KEYDOMET_INPUTPROVIDER_H

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <cassert>
#include <memory>

size_t calc_key_len(size_t keys_num);

enum keys_use { BUILD_CONTAINER, BENCH_OPS };

namespace imp
{
    class keys_provider
    {
    public:

        virtual const std::vector<std::string>& get_keys(uint32_t keys_num, keys_use use) = 0;
        virtual uint32_t get_key_len() const { return 0; }
        virtual ~keys_provider() = default;

    };

    class rand_keys_provider : public keys_provider
    {
        std::unordered_map<uint64_t, std::vector<std::string>> build_cache;
        std::unordered_map<uint64_t, std::vector<std::string>> ops_cache;
        uint32_t key_len;
        uint32_t extra_len;

    public:
        rand_keys_provider(uint32_t key_len_, uint32_t extra_len_) : key_len{key_len_}, extra_len{extra_len_} {}
        const std::vector<std::string>& get_keys(uint32_t keys_num, keys_use use) override;
        virtual uint32_t get_key_len() const override { return key_len + extra_len; }

    };

    rand_keys_provider* get_rand_keys_provider(uint32_t key_len, uint32_t extra_len);

    class dataset_keys_provider : public keys_provider
    {
        std::unordered_map<uint64_t, std::vector<std::string>> build_cache;
        std::unordered_map<uint64_t, std::vector<std::string>> ops_cache;
        std::vector<std::string> raw_content;

        std::vector<std::string> read_dataset(const std::string& file);

    public:
        explicit dataset_keys_provider(std::string file) : raw_content{read_dataset(file)} {}
        const std::vector<std::string>& get_keys(uint32_t keys_num, keys_use use) override;

    };

    dataset_keys_provider* get_dataset_keys_provider(std::string file_name);

}

template<class StrT>
class input_provider;

template<class StrT>
std::unique_ptr<input_provider<StrT>> get_rand_input(uint32_t key_len, uint32_t extra_len)
{
    static std::unordered_map<uint64_t, std::set<StrT, std::less<>>> containers_cache;
    return std::unique_ptr<input_provider<StrT>>{
            // make_unique + private c'tor = no love... gotta new explicitly
            new input_provider<StrT>{imp::get_rand_keys_provider(key_len, extra_len), &containers_cache}
    };
}

template<class StrT>
std::unique_ptr<input_provider<StrT>> get_dataset_input(std::string file_name)
{
    using input_cache = std::unordered_map<uint64_t, std::set<StrT, std::less<>>>;
    static std::unordered_map<std::string, input_cache> file_containers_cache;
    return std::unique_ptr<input_provider<StrT>>{
            // make_unique + private c'tor = no love... gotta new explicitly
            new input_provider<StrT>{imp::get_dataset_keys_provider(file_name), &file_containers_cache[file_name]}
    };
}

template<class StrT>
class input_provider
{

    imp::keys_provider* key_provider;
    using containers_cache = std::unordered_map<uint64_t, std::set<StrT, std::less<>>>;
    containers_cache* container_cache;

    input_provider(imp::keys_provider* provider, containers_cache* cache) :
            key_provider{provider}, container_cache{cache} {}
    std::unique_ptr<input_provider<StrT>> make_unique(imp::keys_provider* provider, containers_cache* cache)
    {
        return std::make_unique<input_provider<StrT>>(provider, cache);
    }
    friend std::unique_ptr<input_provider<StrT>> get_rand_input<StrT>(uint32_t key_len, uint32_t extra_len);
    friend std::unique_ptr<input_provider<StrT>> get_dataset_input<StrT>(std::string);

public:

    const std::vector<std::string>& get_keys(uint32_t keys_num, keys_use use)
    {
        return key_provider->get_keys(keys_num, use);
    }

    const std::set<StrT, std::less<>>& get_container(uint64_t container_size)
    {
        uint64_t cache_key = ((uint64_t)container_size << 32) | key_provider->get_key_len();
        auto& cached = (*container_cache)[cache_key];
        if (cached.size() != container_size)
        {
            assert(cached.size() == 0);
            const auto& input = get_keys(container_size, keys_use::BUILD_CONTAINER);
            std::transform(input.begin(), input.begin() + container_size,
                    std::inserter(cached, cached.begin()), [](const std::string& str) {
                return StrT{str};
            });
            assert(cached.size() == container_size);
        }
        return cached;
    }

};

#endif //KEYDOMET_INPUTPROVIDER_H
