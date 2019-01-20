#include "InputProvider.h"

#include <random>
#include <fstream>
#include <unordered_set>
#include <memory>
#include <experimental/string_view>
#include <iostream>

using namespace std;
using experimental::string_view;

static constexpr char firstChar = 'A', lastChar = 'z';

size_t calcKeyLen(size_t keysNum)
{
    size_t charsNum = lastChar - firstChar + 1;
    size_t keyLen = (size_t)ceil(log(keysNum) / log(charsNum));
    return keyLen;
}

namespace imp
{

    string getRandStr(size_t len)
    {
        static mt19937 gen{random_device{}()};
        uniform_int_distribution<short> dis(firstChar, lastChar);
        string s;
        s.reserve(len);
        for (size_t i = 0; i < len; ++i)
            s += (char)dis(gen);
        return s;
    }

    string getSeqStr(size_t len, size_t extraLen, size_t seq)
    {
        static constexpr size_t charRange = lastChar - firstChar + 1;
        string s;
        s.reserve(len + extraLen);
        while (seq > 0)
        {
            s += char(firstChar + seq % charRange);
            seq /= charRange;
        }
        if (extraLen > 0)
            return s + string(extraLen, '-');
        else
            return s;
    }

    const vector<string>& RandKeysProvider::getKeys(uint32_t keysNum, KeysUse keysUse)
    {
        uint64_t cacheKey = ((uint64_t) keysNum << 32) | (keyLen + extraLen);
        auto& cached = keysUse == KeysUse::BUILD_CONTAINER ? buildCache[cacheKey] : opsCache[cacheKey];
        if (cached.size() != keysNum)
        {
            cached.resize(keysNum);
            size_t seq = 0;
            generate(cached.begin(), cached.end(), [this, &seq] {
                return getSeqStr(keyLen, extraLen, seq++); // avoid duplicates - shuffled below
//            return getRandStr(keyLen);
            });
            shuffle(cached.begin(), cached.end(), mt19937{random_device{}()});
        }
        assert(cached.size() == keysNum);
        return cached;
    }

    RandKeysProvider* getRandKeysProvider(uint32_t keyLen, uint32_t extraLen)
    {
        static unordered_map<size_t, unordered_map<size_t, unique_ptr<RandKeysProvider>>> providers;
        unique_ptr<RandKeysProvider>& provider = providers[keyLen][extraLen];
        if (!provider)
            provider = make_unique<RandKeysProvider>(keyLen, extraLen);
        return provider.get();
    }

    vector<string> DatasetKeysProvider::readDataset(const string& file)
    {
        ifstream fin{file};
        if (!fin)
            throw std::system_error(errno, std::system_category(), "Error opening dataset file " + file);
        unordered_set<string_view> linesSet; // use a set to ensure uniqueness of keys
        vector<string> lines;
        string line;
        while (getline(fin, line))
        {
            if (linesSet.find(line) != linesSet.end())
                continue;
            size_t lastCapacity = lines.capacity();
            lines.emplace_back(line);
            if (lastCapacity < lines.capacity())
            {
                // vector was resized. all string views pointing to SSO now invalid, set must be rebuilt
                linesSet.clear();
                linesSet.insert(lines.begin(), lines.end());
            }
            linesSet.insert(*lines.rbegin());
        }
        shuffle(lines.begin(), lines.end(), mt19937{random_device{}()});
        assert(lines.size() == linesSet.size());
        return lines;
    }

    const vector<string>& DatasetKeysProvider::getKeys(uint32_t keysNum, KeysUse keysUse)
    {
        uint64_t cacheKey = keysNum;
        auto& cached = keysUse == KeysUse::BUILD_CONTAINER ? buildCache[cacheKey] : opsCache[cacheKey];
        if (cached.size() != keysNum)
        {
            assert(keysNum <= rawContent.size()); // if fails, the dataset input file is too small
            cached.resize(keysNum);
            copy(rawContent.begin(), rawContent.begin() + keysNum, cached.begin());
            shuffle(cached.begin(), cached.end(), mt19937{random_device{}()});
        }
        assert(cached.size() == keysNum);
        return cached;
    }

    DatasetKeysProvider* getDatasetKeysProvider(string fileName)
    {
        static unordered_map<string, unique_ptr<DatasetKeysProvider>> providers;
        unique_ptr<DatasetKeysProvider>& provider = providers[fileName];
        if (!provider)
            provider = make_unique<DatasetKeysProvider>(fileName);
        return provider.get();
    }

}