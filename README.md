## Introduction ##

Strings are one of the most commonly used types, yet their implementation is non-trivial. First, strings can store varying amounts of text, requiring dynamic memory management to sometimes be used. Second, strings store their state in an array, and most operations accessing that state require the use of a loop. Lastly, text can come in various encodings, some of which not mapping a single character to a single array element.

Modern C++ addresses some difficulties involved with working with strings. On the performance side, C++11 introduced Short Strings Optimization (SSO), which reduces memory management overhead by having string objects contain a fixed buffer, used for storing short enough strings. When applicable, SSO thus eliminates both allocating buffers on the heap and accessing those buffers using a pointer dereference. However, SSO is only applicable when storing a small enough amount of text, and the use of SSO disallows move semantics. C++17 has introduced string_view, which provides a unified non-owning view of the data stored in either a C++ or C string. While string_view provides useful set of member functions previously available only in std::string yet does not require a std::string to be constructed when a C-string is provided, string_view cannot use SSO hence has a similar access overhead as a C-string.

One of the most common operations involving strings is comparing two and obtaining their relative order. This is done, e.g., when sorting a collection of strings or when traversing an ordered container (std::set, std::map) indexed by strings. String comparison requires comparing the strings character by character, until a mismatch is found. Under the hood, the comparison imposes two forms of overheads:
1. Pointer dereference when accessing the characters array - when SSO is not used, the array is allocated separately than the string object (or pointer) itself, and is not likely to reside on the same cache line. The dereference is might therefore cause a cache miss, requiring an expensive access to memory.
2. Looping in search for the first mismatch - the lookup has to perform multiple operations in addition the actual characters comparison: increasing an index, making sure the loops don't exceed the strings and performing jumps as part of the loop. Worse, exiting a loop usually involves a conditional branch misprediction, as the hardware might guess the loop will continue. When the mismatch is located quickly, branch misprediction rate can become high.

Traversing large ordered containers indexed by strings (and similar operations) is particularly vulnerable to the overheads above. First, when SSO is not leveraged, accessing each string will invoke two cache misses: one when reading the string object, and another when reading the characters array via a pointer. Second, most string comparisons are likely to be resolved quickly. Consider a set implemented as a balanced binary search tree: keys are very different from each other on the top of the tree, and adjacent keys become closer only on the lower part of the tree. Consequently, looking up a particular key will involve comparing to very different keys for a significant part of the traversal. When comparing different keys, the mismatch will be resolved quickly, yielding a high rate of branch mispredictions. Traditional string implementations are therefore sub-optimal when the strings are used as keys in large ordered containers.

## Keydomet - the little string wrapper that makes a big difference ##

Keydomet is a string wrapper, which eliminates much of the two overheads described above: cache miss reaching the characters array and comparison loop. It does so by storing a fixed-size string prefix ("kidomet" in Hebrew) as an integer during construction. The comparison operator is extended appropriately:
1. If this->keydomet != other->keydomet then return this->keydomet - other->keydomet
2. Else resort to full string comparison
So long as compared strings are sufficiently different (namely, have a low rate of prefix collisions), the simple scalar operations prevent the whole string comparison mechanism.

Keydomet is a generic class and is not tied to a particular string type. As long as the prefix can be extracted from the raw string (either using provided adapters to standard strings or dedicated ones), Keydomet can easily be applied. This makes Keydomet-based containers and almost drop-in replacement to string-based containers. Namely, set<string> can easily be replaced with set<Keydomet<string>>, yielding an immediate performance gain on all operations that involve traversal - lookups, insertions and deletion.

In addition to the basic keydomet comparison, a few more optimization knobs are available:
* Keydomet size - the keydomet can be a 2-8 byte long integer. Larger prefixes yield less collisions and less full string comparison, but increase the memory footprint.
* Keydomet deduplication - Since there is a mapping from the string prefix to the keydomet integer, there is no need to compare the strings prefixes upon collision. Instead, given an N byte keydomet, the string comparison can start at byte N+1. Further, the prefix can sometimes be omitted from the characters array. Namely, if the original string is "1234", if the keydomet encodes the "12" part, the characters array need only store the "34" suffix. However, such full deduplication does not allow returning a pointer to the complete string, hence is not enabled by default.
* A Keydomet\<string\> is an object that owns the underlying string. When a lookup is given a string argument (usually as a const reference, string_view or so), it must be converted to a Keydomet\<string\>, which involves expensive allocation and copying. To avoid the overhead, a Keydomet can be compared to any other Keydomet type, even if the underlying string types are different, as long as the strings themselves can be compared. As a result, a Keydomet\<string\> can be compared with a Keydomet\<string_view\>. On containers with transparent comperators, this eliminates the need to construct a full-fledged Keydomet\<string\> for the lookup argument, and instead use a Keydomet\<string_view\> when searching, e.g., a set\<keydomet\<string\>\>.

## Installation ##

To use Keydomet in your code, simply include lib/Keydomet.h and you're done. The complete projects includes unit tests and benchmarks. The project contains the Catch2 header which is used for the unit tests. To run the benchmarks, the google benchmark library should be obtained. The library is included as a git sub-module. To download it, run the init.sh script.

To build the project, use the conventional CMake flow:
mkdir build && cd build
cmake ..
make
The build will produce 3 executables:
1. min_bench - a simple benchmark with no external dependencies
2. tests - unit tests
3. full_bench - the complete set of benchmarks, using both randomly generated strings and a dataset containing Urban Dictionary definitions

## Using Keydomet ##

The Keydomet wrapper takes 2 type arguments:
1. Underlying string implementation, e.g., std::string
2. The amount of string to cache in the Keydomet, using the KeydometSize enum. 32 bits would make a good starting point, but other sizes may turn out to be better in your case.

On the data structure you'd like to optimize, simply replace the string type used as the key with the Keydomet wrapper: instead of map\<string, string\>, use map\<Keydomet\<string, KeyDometSize::SIZE_32BIT\>, string\, std::less\<\>\>. The less\<\> part is required for a transparent comparator to be used, allowing comparison of different types (as long as they support it).

Lastly, use the makeFindKey function to generate a Keydomet lookup key from the original string argument. The function detects whether the container uses a transparent comperator, and if so avoids constructing a new string (as part of the Keydomet), using a Keydomet of a view instead.

## Results ##

## Q&A ##
(TODO)
* the keydomet doesn't change if the string is modified, but keys are never modified anyway.
* keydomet isn't useful in hash tables
* inheritance vs composition
* order preserving hash
* modifying the data structure instead of using keydomet