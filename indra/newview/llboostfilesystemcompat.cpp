/**
 * @file llboostfilesystemcompat.cpp
 * @brief Windows ABI compatibility shims for legacy colladadom + Boost.Filesystem.
 *
 * The prebuilt Windows colladadom library was compiled with /Zc:wchar_t-
 * (wchar_t is an unsigned short typedef). The viewer and FetchContent-built
 * Boost are compiled with normal /Zc:wchar_t (wchar_t is a native type).
 *
 * That means colladadom asks the linker for Boost.Filesystem symbols whose
 * signatures contain std::basic_string<unsigned short> instead of std::wstring.
 * Boost 1.86 correctly provides the native wchar_t versions for viewer code,
 * but not those legacy unsigned-short-mangled entry points.
 *
 * These two forwarding shims provide the legacy symbols colladadom expects and
 * bridge them to the native Boost.Filesystem path_traits::convert overloads.
 */

#include <boost/filesystem/detail/path_traits.hpp>

#include <cwchar>
#include <locale>
#include <string>

#if defined(_MSC_VER) && defined(_NATIVE_WCHAR_T_DEFINED)

namespace boost::filesystem::detail::path_traits
{
    using ushort_string = std::basic_string<unsigned short,
                                            std::char_traits<unsigned short>,
                                            std::allocator<unsigned short>>;
    using ushort_codecvt = std::codecvt<unsigned short, char, std::mbstate_t>;

    void convert(const char* from,
                 const char* from_end,
                 ushort_string& to,
                 const ushort_codecvt* cvt)
    {
        std::wstring native;
        const auto* native_cvt = reinterpret_cast<const codecvt_type*>(cvt);

        convert(from, from_end, native, native_cvt);

        const auto* first = reinterpret_cast<const unsigned short*>(native.data());
        to.assign(first, first + native.size());
    }

    void convert(const unsigned short* from,
                 const unsigned short* from_end,
                 std::string& to,
                 const ushort_codecvt* cvt)
    {
        const auto* native_from = reinterpret_cast<const wchar_t*>(from);
        const auto* native_end = reinterpret_cast<const wchar_t*>(from_end);
        const auto* native_cvt = reinterpret_cast<const codecvt_type*>(cvt);

        convert(native_from, native_end, to, native_cvt);
    }
}

#endif // defined(_MSC_VER) && defined(_NATIVE_WCHAR_T_DEFINED)
