//
// Copyright(c) 2019 Eran Gilad, https://github.com/erangi/kdmt
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "InputProvider.h"

#include <random>
#include <fstream>
#include <unordered_set>
#include <memory>
#include <iostream>
#if (__cplusplus < 201703L) && !(defined(__clang__) && __clang_major__ > 7)
    #include <experimental/string_view>
    using std::experimental::string_view;
#else
    #include <string_view>
#endif

using namespace std;

static constexpr char first_char = 'A', last_char = 'z';

size_t calc_key_len(size_t keys_num)
{
    size_t chars_num = last_char - first_char + 1;
    size_t key_len = (size_t)ceil(log(keys_num) / log(chars_num));
    return key_len;
}

namespace imp
{

    string get_rand_str(size_t len)
    {
        static mt19937 gen{random_device{}()};
        uniform_int_distribution<short> dis(first_char, last_char);
        string s;
        s.reserve(len);
        for (size_t i = 0; i < len; ++i)
            s += (char)dis(gen);
        return s;
    }

    string get_seq_str(size_t len, size_t extra_len, size_t seq)
    {
        static constexpr size_t char_range = last_char - first_char + 1;
        string s;
        s.reserve(len + extra_len);
        while (seq > 0)
        {
            s += char(first_char + seq % char_range);
            seq /= char_range;
        }
        if (extra_len > 0)
            return s + string(extra_len, '-');
        else
            return s;
    }

    const vector<string>& rand_keys_provider::get_keys(uint32_t keys_num, keys_use use)
    {
        uint64_t cache_key = ((uint64_t) keys_num << 32) | (key_len + extra_len);
        auto& cached = use == keys_use::BUILD_CONTAINER ? build_cache[cache_key] : ops_cache[cache_key];
        if (cached.size() != keys_num)
        {
            cached.resize(keys_num);
            size_t seq = 0;
            generate(cached.begin(), cached.end(), [this, &seq] {
                return get_seq_str(key_len, extra_len, seq++); // avoid duplicates - shuffled below
//            return get_rand_str(key_len);
            });
            shuffle(cached.begin(), cached.end(), mt19937{random_device{}()});
        }
        assert(cached.size() == keys_num);
        return cached;
    }

    rand_keys_provider* get_rand_keys_provider(uint32_t key_len, uint32_t extra_len)
    {
        static unordered_map<size_t, unordered_map<size_t, unique_ptr<rand_keys_provider>>> providers;
        unique_ptr<rand_keys_provider>& provider = providers[key_len][extra_len];
        if (!provider)
            provider = make_unique<rand_keys_provider>(key_len, extra_len);
        return provider.get();
    }

    vector<string> dataset_keys_provider::read_dataset(const string& file)
    {
        ifstream fin{file};
        if (!fin)
            throw std::system_error(errno, std::system_category(), "Error opening dataset file " + file);
        unordered_set<string_view> lines_set; // use a set to ensure uniqueness of keys
        vector<string> lines;
        string line;
        while (getline(fin, line))
        {
            if (lines_set.find(line) != lines_set.end())
                continue;
            size_t lastCapacity = lines.capacity();
            lines.emplace_back(line);
            if (lastCapacity < lines.capacity())
            {
                // vector was resized. all string views pointing to SSO now invalid, set must be rebuilt
                lines_set.clear();
                lines_set.insert(lines.begin(), lines.end());
            }
            lines_set.insert(*lines.rbegin());
        }
        shuffle(lines.begin(), lines.end(), mt19937{random_device{}()});
        assert(lines.size() == lines_set.size());
        return lines;
    }

    const vector<string>& dataset_keys_provider::get_keys(uint32_t keys_num, keys_use use)
    {
        uint64_t cache_key = keys_num;
        auto& cached = use == keys_use::BUILD_CONTAINER ? build_cache[cache_key] : ops_cache[cache_key];
        if (cached.size() != keys_num)
        {
            assert(keys_num <= raw_content.size()); // if fails, the dataset input file is too small
            cached.resize(keys_num);
            copy(raw_content.begin(), raw_content.begin() + keys_num, cached.begin());
            shuffle(cached.begin(), cached.end(), mt19937{random_device{}()});
        }
        assert(cached.size() == keys_num);
        return cached;
    }

    dataset_keys_provider* get_dataset_keys_provider(string file_name)
    {
        static unordered_map<string, unique_ptr<dataset_keys_provider>> providers;
        unique_ptr<dataset_keys_provider>& provider = providers[file_name];
        if (!provider)
            provider = make_unique<dataset_keys_provider>(file_name);
        return provider.get();
    }

}