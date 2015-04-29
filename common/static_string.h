// All credit to Jarod42 from stackoverflow, from
// https://stackoverflow.com/questions/25740734/macro-for-static-stdstring-object-from-literal
// Although it turns out msvc can't actually compile this yet
#pragma once

#include <cstdint>
#include <string>

// Except for the namespace :)
namespace fd {

// Sequence of char
template <char...Cs> struct char_sequence
{
    template <char C> using push_back = char_sequence<Cs..., C>;
};

// Remove all chars from char_sequence from '\0'
template <typename, char...> struct strip_sequence;

template <char...Cs>
struct strip_sequence<char_sequence<>, Cs...>
{
    using type = char_sequence<Cs...>;
};

template <char...Cs, char...Cs2>
struct strip_sequence<char_sequence<'\0', Cs...>, Cs2...>
{
    using type = char_sequence<Cs2...>;
};

template <char...Cs, char C, char...Cs2>
struct strip_sequence<char_sequence<C, Cs...>, Cs2...>
{
    using type = typename strip_sequence<char_sequence<Cs...>, Cs2..., C>::type;
};

// struct to create a std::string
template <typename chars> struct static_string;

template <char...Cs>
struct static_string<char_sequence<Cs...>>
{
    static const std::string str;
};

template <char...Cs>
const
std::string static_string<char_sequence<Cs...>>::str = {Cs...};

// helper to get the i_th character (`\0` for out of bound)
template <std::size_t I, std::size_t N>
constexpr char at(const char (&a)[N]) { return I < N ? a[I] : '\0'; }

// helper to check if the c-string will not be truncated
template <std::size_t max_size, std::size_t N>
constexpr bool check_size(const char (&)[N])
{
    static_assert(N <= max_size, "string too long");
    return N <= max_size;
}

// Helper macros to build char_sequence from c-string
#define PUSH_BACK_8(S, I) \
    ::push_back<at<(I) + 0>(S)>::push_back<at<(I) + 1>(S)> \
    ::push_back<at<(I) + 2>(S)>::push_back<at<(I) + 3>(S)> \
    ::push_back<at<(I) + 4>(S)>::push_back<at<(I) + 5>(S)> \
    ::push_back<at<(I) + 6>(S)>::push_back<at<(I) + 7>(S)>

#define PUSH_BACK_32(S, I) \
        PUSH_BACK_8(S, (I) + 0) PUSH_BACK_8(S, (I) + 8) \
        PUSH_BACK_8(S, (I) + 16) PUSH_BACK_8(S, (I) + 24)

#define PUSH_BACK_128(S, I) \
    PUSH_BACK_32(S, (I) + 0) PUSH_BACK_32(S, (I) + 32) \
    PUSH_BACK_32(S, (I) + 64) PUSH_BACK_32(S, (I) + 96)

// Macro to create char_sequence from c-string (limited to 128 chars) without leading '\0'
#define MAKE_CHAR_SEQUENCE(S) \
    strip_sequence<char_sequence<> \
    PUSH_BACK_128(S, 0) \
    ::push_back<check_size<128>(S) ? '\0' : '\0'> \
    >::type

// Macro to return an static std::string
#define STATIC_STRING(S) static_string<MAKE_CHAR_SEQUENCE(S)>::str

} //namespace fd