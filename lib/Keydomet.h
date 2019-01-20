//
// Copyright(c) 2019 Eran Gilad, https://github.com/erangi/kdmt
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef KEYDOMET_KEYDOMET_H
#define KEYDOMET_KEYDOMET_H

#include <iostream>
#include <string>
#include <cstring>
#include <utility>
#include <type_traits>
#include <map>
#include <set>

//TODO optimize keydomet generation for SSO
//TODO consider implications of storage type when char type is signed vs. unsigned
//TODO make keydomet usage stats optional
//TODO measure memory footprint in term of cache lines on various benchmarks.
//     mostly, eval the effect of going from a 32B string object to 40B with keydomet, ruining cache line packing.
//TODO consider placing the keydomet in the SSO buffer when the string is long (and not using keydomet otherwise).
//     since keydomet isn't much better than SSO, the memory overhead reduction could outweigh not using keydomet on
//     short strings. should probably be a compile time optimization choice.

namespace kdmt
{

    enum class KeydometSize : uint8_t
    {
        SIZE_16BIT  = 2,
        SIZE_32BIT  = 4,
        SIZE_64BIT  = 8,
        SIZE_128BIT = 16
    };

    template<KeydometSize>
    struct KeydometStorage; // Unexpected KeydometSize value!

    template<> struct KeydometStorage<KeydometSize::SIZE_16BIT>  { using type = uint16_t; };
    template<> struct KeydometStorage<KeydometSize::SIZE_32BIT>  { using type = uint32_t; };
    template<> struct KeydometStorage<KeydometSize::SIZE_64BIT>  { using type = uint64_t; };

    struct kdmt128_t
    {
        using HalfType = typename KeydometStorage<KeydometSize::SIZE_64BIT>::type;
        HalfType msbs;
        HalfType lsbs;

        kdmt128_t() = default;
        constexpr explicit kdmt128_t(HalfType l) : msbs{0}, lsbs{l} {}
        constexpr kdmt128_t(HalfType m, HalfType l) : msbs{m}, lsbs{l} {}
        constexpr kdmt128_t operator&(const kdmt128_t& other) const
        {
            return {(msbs & other.msbs), (lsbs & other.lsbs)};
        }
        constexpr bool operator<(const kdmt128_t& other) const
        {
            return (msbs < other.msbs) || (msbs == other.msbs && lsbs < other.lsbs);
        }
        constexpr bool operator==(const kdmt128_t& other) const
        {
            return (msbs == other.msbs) && (lsbs == other.lsbs);
        }
        constexpr bool operator!=(const kdmt128_t& other) const
        {
            return (msbs != other.msbs) || (lsbs != other.lsbs);
        }
    };
    template<> struct KeydometStorage<KeydometSize::SIZE_128BIT> { using type = kdmt128_t; };

    //
    // Helper functions to provide access to the raw c-string array.
    // New string types should add an overload, which uses that type's API.
    //
    inline const char* getRawStr(const char* str) { return str; }
    template<typename StrT>
    inline std::enable_if_t<std::is_same<decltype(std::declval<StrT>().data()), const char*>::value, const char*>
    getRawStr(const StrT& str) { return str.data(); }

    //
    // Helper function for swapping bytes, turning the big endian order in the string into
    // a proper little endian number. For now, only GCC is supported due to the use of intrinsics.
    //
    inline void flipBytes(uint16_t& val) { val = __builtin_bswap16(val); }
    inline void flipBytes(uint32_t& val) { val = __builtin_bswap32(val); }
    inline void flipBytes(uint64_t& val) { val = __builtin_bswap64(val); }
    inline void flipBytes(kdmt128_t& val) {
        val.lsbs = __builtin_bswap64(val.lsbs);
        val.msbs = __builtin_bswap64(val.msbs);
    }

    //
    // Helper function that turns the string's keydomet into a number.
    // TODO - Can be optimized, e.g., by using a cast when the string's buffer is known to be large enough
    // (namely, SSO is used; zero padding still required based on actual string size).
    //
    template<typename KeydometT, typename StrImp>
    inline KeydometT strToPrefix(const StrImp& str)
    {
        const char* cstr = getRawStr(str);
        KeydometT trg;
        strncpy((char*)&trg, cstr, sizeof(trg)); // short strings are padded with zeros
        flipBytes(trg);
        return trg;
    }

    template<KeydometSize SIZE>
    class Prefix
    {

    public:

        using PrefixType = typename KeydometStorage<SIZE>::type;

        explicit Prefix(std::nullptr_t) noexcept {} // only exists to allow actual initialization *after* string construction

        template<typename StrImp>
        explicit Prefix(const StrImp& str) : val{strToPrefix<PrefixType>(str)}
        {
        }

        Prefix(const Prefix& other) : val(other.val)
        {}

        Prefix(Prefix&&) noexcept = default;

        Prefix& operator=(Prefix&&) noexcept = default;
        Prefix& operator=(const Prefix&) noexcept = default;

        PrefixType getVal() const
        {
            return val;
        }

        bool operator<(const Prefix<SIZE>& other) const
        {
            return this->val < other.val;
        }

        bool operator==(const Prefix<SIZE>& other) const
        {
            return this->val == other.val;
        }

        bool operator!=(const Prefix<SIZE>& other) const
        {
            return this->val != other.val;
        }

        bool stringShorterThanKeydomet() const
        {
            // if the last byte/char of the keydomet is all zeros, the string must
            // have been shorter than the keydomet's capacity.
            // note: this won't always be correct when working with Unicode strings!
            constexpr PrefixType LastByteMask{0xFFUL};
            return (val & LastByteMask) == PrefixType{0UL};
        }

    private:

        PrefixType val; // non-const to allow move assignment (useful in vector resizing and algorithms)

    };

    template<class StrImp_, KeydometSize Size_>
    class Keydomet
    {

    public:

        using StrImp = StrImp_;
        static constexpr KeydometSize Size = Size_;

        Keydomet(const StrImp& s) : prefix{s}, str{s}
        {
        }

        Keydomet(std::remove_reference_t<StrImp>&& s) : prefix{static_cast<const StrImp&>(s)}, str{s}
        {
        }

        template<typename... Args, class StrT = StrImp>
        Keydomet(std::enable_if<std::is_reference<StrT>::value, Args...> args) : prefix{nullptr}, str(std::forward<Args>(args)...)
        {
            prefix = Prefix<Size_>{str}; // must be done here due to initialization order
        }

        template<typename OtherImp, KeydometSize OtherSize>
        friend class Keydomet;

        template<typename Imp>
        int compare(const Keydomet<Imp, Size_>& other) const
        {
            if (this->prefix != other.prefix)
            {
                ++usedPrefix();
                return diffAsOneOrMinusOne(this->prefix, other.prefix);
            }
            else
            {
                if (this->prefix.stringShorterThanKeydomet())
                {
                    ++usedPrefix();
                    return 0;
                }
                ++usedString();
                return strcmp(getRawStr(str) + sizeof(Size_), getRawStr(other.str) + sizeof(Size_));
            }
        }

        template<typename Imp>
        bool operator<(const Keydomet<Imp, Size_>& other) const
        {
            return compare(other) < 0;
        }

        template<typename Imp>
        bool operator==(const Keydomet<Imp, Size_>& other) const
        {
            return compare(other) == 0;
        }

        const Prefix<Size_>& getPrefix() const
        {
            return prefix;
        }

        const StrImp_& getStr() const
        {
            return str;
        }

        static size_t& usedPrefix() { static size_t counter = 0; return counter; }
        static size_t& usedString() { static size_t counter = 0; return counter; }

    private:

        // using composition instead of inheritance (from StrImp) to make sure
        // the keydomet is at the beginning of the object rather than at the end
        Prefix<Size_> prefix;
        StrImp_ str;

        static int diffAsOneOrMinusOne(const Prefix<Size_>& v1, const Prefix<Size_>& v2)
        {
            // return -1 if v1 < v2 and 1 otherwise
            // this is done without conditional branches and without risking wrap-around
            return ((int)!(v1 < v2) << 1) - 1;
        }

    };

    inline std::ostream& operator<<(std::ostream& os, const kdmt128_t& val)
    {
        os << val.msbs << val.lsbs;
        return os;
    }

    template<typename StrImp, KeydometSize Size>
    inline std::ostream& operator<<(std::ostream& os, const Keydomet<StrImp, Size>& hk)
    {
        const StrImp& str = hk.getStr();
        os << str;
        return os;
    }

    namespace imp
    {
        // Searching associative containers (namely, sorted rather than hashed) could only be done using
        // the exact key type till C++14. From C++14, any type that can be compared with the key type can
        // be used. For instance, a std::string_view can be used in order to find a std::string key.
        // this is very useful when a key is given as a std::string but a Keydomet should be constructed
        // in order to do the lookup: instead of creating a Keydomet that will own a copy of the std::string
        // (with all the mess involved with the creation of the copy - allocation etc.), a Keydomet holding a
        // const string reference is used, merely referencing the original std::string.
        // To use the above feature, the comparator used by the container must be transparent, namely have
        // a is_transparent member. The code below detects the presence of that member, and the result is
        // used to determine whether a Keydomet or KeydometView should be constructed for lookups.
        template<typename...>
        using VoidT = void;

        struct NonTransparent {};

        auto getComparator(...) -> NonTransparent;

        template<template<class, class...> class Container, class KD, class... Args>
        auto getComparator(const Container<KD, Args...>&) -> typename Container<KD, Args...>::key_compare;

        template<class Comparator, class = VoidT<>>
        struct HasTransparentFlagHelper : std::false_type {};

        template<class Comparator>
        struct HasTransparentFlagHelper<Comparator, VoidT<typename Comparator::is_transparent>> : std::true_type {};

        template<template<class, class...> class Container, class KD, class... Args>
        auto isTransparent(const Container<KD, Args...>& c) ->
        std::enable_if_t<HasTransparentFlagHelper<decltype(getComparator(c))>::value, std::true_type>;

        auto isTransparent(...) -> std::false_type;

        template<bool IsRef>
        void verifyContainerUsesTransparentComperator(std::enable_if_t<IsRef, int> = 0) {}

        template<bool IsRef>
        void verifyContainerUsesTransparentComperator(std::enable_if_t<!IsRef, int> = 0)
        {
            // Containers such as map and set should use /transparent comparators/, which allow keys of different types
            // to be compared. This eliminates the need to create owning Keydomet objects.
            // Easiest way to do it: use the std::less<> comparator - std::set<KdmtStr, std::less<>>;
            int CheckOutCommentAboveMe;
        }
    }

    template<class StrT, template<class, class...> class Container, KeydometSize Size, class... Args>
    inline auto makeFindKey(const Container<Keydomet<StrT, Size>, Args...>& s, const StrT& key)
    {
        // associative containers (maps, sets) can use a transparent comparator. such a comperator can
        // compare the internal key type with other types (as long as there's an appropriate operator).
        // this allows such containers to hold Keydomet<string> but search using Keydomet<const string&>, saving the allocation.
        // *** Note: to make a container associative, define it with the less<> comparator:
        // *** using KeydometSet = std::set<KdmtStr, std::less<>>;
        using KeydometStrType = std::conditional_t<decltype(imp::isTransparent(s))::value,
                Keydomet<const StrT&, Size>,
                Keydomet<StrT, Size>
        >;
        imp::verifyContainerUsesTransparentComperator<decltype(imp::isTransparent(s))::value>();
        return KeydometStrType{key};
    }

}

#endif //KEYDOMET_KEYDOMET_H
