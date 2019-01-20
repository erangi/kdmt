
#ifndef KEYDOMET_INPUTPROVIDER_H
#define KEYDOMET_INPUTPROVIDER_H

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <cassert>
#include <memory>

size_t calcKeyLen(size_t keysNum);

enum KeysUse { BUILD_CONTAINER, BENCH_OPS };

namespace imp
{
    class KeysProvider
    {
    public:

        virtual const std::vector<std::string>& getKeys(uint32_t keysNum, KeysUse keysUse) = 0;
        virtual uint32_t getKeyLen() const { return 0; }

    };

    class RandKeysProvider : public KeysProvider
    {
        std::unordered_map<uint64_t, std::vector<std::string>> buildCache;
        std::unordered_map<uint64_t, std::vector<std::string>> opsCache;
        uint32_t keyLen;
        uint32_t extraLen;

    public:
        RandKeysProvider(uint32_t keyLen_, uint32_t extraLen_) : keyLen{keyLen_}, extraLen{extraLen_} {}
        const std::vector<std::string>& getKeys(uint32_t keysNum, KeysUse keysUse) override;
        virtual uint32_t getKeyLen() const override { return keyLen + extraLen; }

    };

    RandKeysProvider* getRandKeysProvider(uint32_t keyLen, uint32_t extraLen);

    class DatasetKeysProvider : public KeysProvider
    {
        std::unordered_map<uint64_t, std::vector<std::string>> buildCache;
        std::unordered_map<uint64_t, std::vector<std::string>> opsCache;
        std::vector<std::string> rawContent;

        std::vector<std::string> readDataset(const std::string& file);

    public:
        explicit DatasetKeysProvider(std::string file) : rawContent{readDataset(file)} {}
        const std::vector<std::string>& getKeys(uint32_t keysNum, KeysUse keysUse) override;

    };

    DatasetKeysProvider* getDatasetKeysProvider(std::string fileName);

}

template<class StrT>
class InputProvider;

template<class StrT>
std::unique_ptr<InputProvider<StrT>> getRandInput(uint32_t keyLen, uint32_t extraLen)
{
    static std::unordered_map<uint64_t, std::set<StrT, std::less<>>> containersCache;
    return std::unique_ptr<InputProvider<StrT>>{
            // make_unique + private c'tor = no love... gotta new explicitly
            new InputProvider<StrT>{imp::getRandKeysProvider(keyLen, extraLen), &containersCache}
    };
}

template<class StrT>
std::unique_ptr<InputProvider<StrT>> getDatasetInput(std::string fileName)
{
    using InputCache = std::unordered_map<uint64_t, std::set<StrT, std::less<>>>;
    static std::unordered_map<std::string, InputCache> fileContainersCache;
    return std::unique_ptr<InputProvider<StrT>>{
            // make_unique + private c'tor = no love... gotta new explicitly
            new InputProvider<StrT>{imp::getDatasetKeysProvider(fileName), &fileContainersCache[fileName]}
    };
}

template<class StrT>
class InputProvider
{

    imp::KeysProvider* keysProvider;
    using ContainersCache = std::unordered_map<uint64_t, std::set<StrT, std::less<>>>;
    ContainersCache* containersCache;

    InputProvider(imp::KeysProvider* provider, ContainersCache* cache) :
            keysProvider{provider}, containersCache{cache} {}
    std::unique_ptr<InputProvider<StrT>> makeUnique(imp::KeysProvider* provider, ContainersCache* cache)
    {
        return std::make_unique<InputProvider<StrT>>(provider, cache);
    }
    friend std::unique_ptr<InputProvider<StrT>> getRandInput<StrT>(uint32_t keyLen, uint32_t extraLen);
    friend std::unique_ptr<InputProvider<StrT>> getDatasetInput<StrT>(std::string);

public:

    const std::vector<std::string>& getKeys(uint32_t keysNum, KeysUse keysUse)
    {
        return keysProvider->getKeys(keysNum, keysUse);
    }

    const std::set<StrT, std::less<>>& getContainer(uint64_t containerSize)
    {
        uint64_t cacheKey = ((uint64_t)containerSize << 32) | keysProvider->getKeyLen();
        auto& cached = (*containersCache)[cacheKey];
        if (cached.size() != containerSize)
        {
            assert(cached.size() == 0);
            const auto& input = getKeys(containerSize, KeysUse::BUILD_CONTAINER);
            std::transform(input.begin(), input.begin() + containerSize,
                    std::inserter(cached, cached.begin()), [](const std::string& str) {
                return StrT{str};
            });
            assert(cached.size() == containerSize);
        }
        return cached;
    }

};

#endif //KEYDOMET_INPUTPROVIDER_H
