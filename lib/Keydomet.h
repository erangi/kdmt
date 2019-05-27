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

    enum class prefix_size : uint8_t
    {
        SIZE_16BIT  = 2,
        SIZE_32BIT  = 4,
        SIZE_64BIT  = 8,
        SIZE_128BIT = 16
    };

    template<prefix_size>
    struct prefix_storage; // Unexpected prefix_size value!

    template<> struct prefix_storage<prefix_size::SIZE_16BIT>  { using type = uint16_t; };
    template<> struct prefix_storage<prefix_size::SIZE_32BIT>  { using type = uint32_t; };
    template<> struct prefix_storage<prefix_size::SIZE_64BIT>  { using type = uint64_t; };

    struct kdmt128_t
    {
        using half_type = typename prefix_storage<prefix_size::SIZE_64BIT>::type;
        half_type msbs;
        half_type lsbs;

        kdmt128_t() = default;
        constexpr explicit kdmt128_t(half_type l) : msbs{0}, lsbs{l} {}
        constexpr kdmt128_t(half_type m, half_type l) : msbs{m}, lsbs{l} {}
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
    template<> struct prefix_storage<prefix_size::SIZE_128BIT> { using type = kdmt128_t; };

    //
    // Helper functions to provide access to the raw c-string array.
    // New string types should add an overload, which uses that type's API.
    //
    inline const char* get_raw_str(const char* str) { return str; }
    template<typename StrT>
    inline std::enable_if_t<std::is_same<decltype(std::declval<StrT>().data()), const char*>::value, const char*>
    get_raw_str(const StrT& str) { return str.data(); }

    //
    // Helper function for swapping bytes, turning the big endian order in the string into
    // a proper little endian number. For now, only GCC is supported due to the use of intrinsics.
    //
    inline void flip_bytes(uint16_t& val) { val = __builtin_bswap16(val); }
    inline void flip_bytes(uint32_t& val) { val = __builtin_bswap32(val); }
    inline void flip_bytes(uint64_t& val) { val = __builtin_bswap64(val); }
    inline void flip_bytes(kdmt128_t& val) {
        val.lsbs = __builtin_bswap64(val.lsbs);
        val.msbs = __builtin_bswap64(val.msbs);
    }

    //
    // Helper function that turns the string's keydomet into a number.
    // TODO - Can be optimized, e.g., by using a cast when the string's buffer is known to be large enough
    // (namely, SSO is used; zero padding still required based on actual string size).
    //
    template<typename KeydometT, typename StrImp>
    inline KeydometT str_to_prefix(const StrImp& str)
    {
        const char* cstr = get_raw_str(str);
        KeydometT trg;
#if defined(__GNUC__) && (__GNUC__ >= 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif // __GNUC__
        strncpy((char*)&trg, cstr, sizeof(trg)); // short strings are padded with zeros
#if defined(__GNUC__) && (__GNUC__ >= 8)
#pragma GCC diagnostic pop
#endif // __GNUC__
        flip_bytes(trg);
        return trg;
    }

    template<prefix_size SIZE>
    class prefix_rep
    {

    public:

        using prefix_type = typename prefix_storage<SIZE>::type;

        explicit prefix_rep(std::nullptr_t) noexcept {} // only exists to allow actual initialization *after* string construction

        template<typename StrImp>
        explicit prefix_rep(const StrImp& str) : val{str_to_prefix<prefix_type>(str)}
        {
        }

        prefix_rep(const prefix_rep& other) : val(other.val)
        {}

        prefix_rep(prefix_rep&&) noexcept = default;

        prefix_rep& operator=(prefix_rep&&) noexcept = default;
        prefix_rep& operator=(const prefix_rep&) noexcept = default;

        prefix_type get_val() const
        {
            return val;
        }

        bool operator<(const prefix_rep<SIZE>& other) const
        {
            return this->val < other.val;
        }

        bool operator==(const prefix_rep<SIZE>& other) const
        {
            return this->val == other.val;
        }

        bool operator!=(const prefix_rep<SIZE>& other) const
        {
            return this->val != other.val;
        }

        bool string_shorter_than_prefix() const
        {
            // if the last byte/char of the keydomet is all zeros, the string must
            // have been shorter than the keydomet's capacity.
            // note: this won't always be correct when working with Unicode strings!
            constexpr prefix_type LastByteMask{0xFFUL};
            return (val & LastByteMask) == prefix_type{0UL};
        }

    private:

        prefix_type val; // non-const to allow move assignment (useful in vector resizing and algorithms)

    };

    template<class StrImp, prefix_size PrefixSize>
    class prefix_str
    {

    public:

        prefix_str(const StrImp& s) : prefix{s}, str{s}
        {
        }

        prefix_str(std::remove_reference_t<StrImp>&& s) : prefix{static_cast<const StrImp&>(s)}, str{s}
        {
        }

        template<typename... Args, class StrT = StrImp>
        prefix_str(std::enable_if<std::is_reference<StrT>::value, Args...> args) : prefix{nullptr}, str(std::forward<Args>(args)...)
        {
            prefix = prefix_rep<PrefixSize>{str}; // must be done here due to initialization order
        }

        prefix_rep<PrefixSize>& get_prefix() { return prefix; }
        const prefix_rep<PrefixSize>& get_prefix() const { return prefix; }

        StrImp& get_str() { return str; }
        const StrImp& get_str() const { return str; }

    private:

        // the order is important to make sure the prefix is placed in front of the string
        prefix_rep<PrefixSize> prefix;
        StrImp str;
    };

    template<class StrImp, prefix_size PrefixSize>
    class keydomet : private prefix_str<StrImp, PrefixSize>
    {

    public:

        using str_imp = StrImp;
        static constexpr prefix_size size = PrefixSize;

        keydomet(const str_imp& s) :
                prefix_str<StrImp, PrefixSize>{s}
        {
        }

        keydomet(std::remove_reference_t<str_imp>&& s) :
                prefix_str<StrImp, PrefixSize>{s}
        {
        }

        template<typename... Args, class StrT = str_imp>
        keydomet(std::enable_if<std::is_reference<StrT>::value, Args...> args) :
                prefix_str<StrImp, PrefixSize>(std::forward<Args>(args)...)
        {
        }

        template<typename OtherImp, prefix_size OtherSize>
        friend class keydomet;

        template<typename Imp>
        int compare(const keydomet<Imp, PrefixSize>& other) const
        {
            if (this->get_prefix() != other.get_prefix())
            {
                ++used_prefix();
                return diff_as_one_or_minus_one(this->get_prefix(), other.get_prefix());
            }
            else
            {
                if (this->get_prefix().string_shorter_than_prefix())
                {
                    ++used_prefix();
                    return 0;
                }
                ++used_string();
                return strcmp(get_raw_str(get_str()) + sizeof(PrefixSize), get_raw_str(other.get_str()) + sizeof(PrefixSize));
            }
        }

        template<typename Imp>
        bool operator<(const keydomet<Imp, PrefixSize>& other) const
        {
            return compare(other) < 0;
        }

        template<typename Imp>
        bool operator==(const keydomet<Imp, PrefixSize>& other) const
        {
            return compare(other) == 0;
        }

        const prefix_rep<PrefixSize>& get_prefix() const
        {
            return prefix_str<StrImp, PrefixSize>::get_prefix();
        }

        const StrImp& get_str() const
        {
            return prefix_str<StrImp, PrefixSize>::get_str();
        }

        static size_t& used_prefix() { static size_t counter = 0; return counter; }
        static size_t& used_string() { static size_t counter = 0; return counter; }

    private:

        static int diff_as_one_or_minus_one(const prefix_rep<PrefixSize>& v1, const prefix_rep<PrefixSize>& v2)
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

    template<typename StrImp, prefix_size Size>
    inline std::ostream& operator<<(std::ostream& os, const keydomet<StrImp, Size>& hk)
    {
        const StrImp& str = hk.get_str();
        os << str;
        return os;
    }

    namespace imp
    {
        // Searching associative containers (namely, sorted rather than hashed) could only be done using
        // the exact key type till C++14. From C++14, any type that can be compared with the key type can
        // be used. For instance, a std::string_view can be used in order to find a std::string key.
        // this is very useful when a key is given as a std::string but a keydomet should be constructed
        // in order to do the lookup: instead of creating a keydomet that will own a copy of the std::string
        // (with all the mess involved with the creation of the copy - allocation etc.), a keydomet holding a
        // const string reference is used, merely referencing the original std::string.
        // To use the above feature, the comparator used by the container must be transparent, namely have
        // a is_transparent member. The code below detects the presence of that member, and the result is
        // used to determine whether a keydomet or KeydometView should be constructed for lookups.
        template<typename...>
        using void_t = void;

        struct non_transparent {};

        auto get_comparator(...) -> non_transparent;

        template<template<class, class...> class Container, class KD, class... Args>
        auto get_comparator(const Container<KD, Args...>&) -> typename Container<KD, Args...>::key_compare;

        template<class Comparator, class = void_t<>>
        struct has_transparent_flag_helper : std::false_type {};

        template<class Comparator>
        struct has_transparent_flag_helper<Comparator, void_t<typename Comparator::is_transparent>> : std::true_type {};

        template<template<class, class...> class Container, class KD, class... Args>
        auto is_transparent(const Container<KD, Args...>& c) ->
        std::enable_if_t<has_transparent_flag_helper<decltype(get_comparator(c))>::value, std::true_type>;

        auto is_transparent(...) -> std::false_type;

        template<class IsTrans>
        void verify_container_uses_transparent_comperator()
        {
            // Containers such as map and set should use /transparent comparators/, which allow keys of different types
            // to be compared. This eliminates the need to create owning keydomet objects.
            // Easiest way to do it: use the std::less<> comparator - std::set<KdmtStr, std::less<>>;
            IsTrans::CheckOutCommentAboveMe();
        }

        template<>
        void verify_container_uses_transparent_comperator<std::true_type>() {}
    }

    template<class StrT, template<class, class...> class Container, prefix_size Size, class... Args>
    inline auto make_find_key(const Container<keydomet<StrT, Size>, Args...>& s, const StrT& key)
    {
        // associative containers (maps, sets) can use a transparent comparator. such a comperator can
        // compare the internal key type with other types (as long as there's an appropriate operator).
        // this allows such containers to hold keydomet<string> but search using keydomet<const string&>, saving the allocation.
        // *** Note: to make a container associative, define it with the less<> comparator:
        // *** using keydomet_set = std::set<kdmt_str, std::less<>>;
        using keydomet_str_type = std::conditional_t<decltype(imp::is_transparent(s))::value,
                keydomet<const StrT&, Size>,
                keydomet<StrT, Size>
        >;
        imp::verify_container_uses_transparent_comperator<decltype(imp::is_transparent(s))>();
        return keydomet_str_type{key};
    }

}

#endif //KEYDOMET_KEYDOMET_H
