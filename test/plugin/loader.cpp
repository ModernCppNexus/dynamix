// DynaMix
// Copyright (c) 2013-2019 Borislav Stanimirov, Zahary Karadjov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include "dynlib_A.hpp"

#include <dynamix/object.hpp>
#include <dynamix/mutate.hpp>
#include <dynamix/combinators.hpp>
#include <dynamix/define_mixin.hpp>

#include "doctest/doctest.h"

#if defined (_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef HMODULE DynamicLib;

inline DynamicLib LoadDynamicLib(const char* lib)
{
    std::string l;
#if defined(__GNUC__)
    l = "lib";
#endif
    l += lib;
    l += ".dll";
    return LoadLibrary(l.c_str());
}

#define CloseDynamicLib FreeLibrary
#define GetProc GetProcAddress

#else

#include <dlfcn.h>

typedef void* DynamicLib;

inline DynamicLib LoadDynamicLib(const char* lib)
{
    std::string l = "lib";
    l += lib;
#if defined(__APPLE__)
    l += ".dylib";
#else
    l += ".so";
#endif
    return dlopen(l.c_str(), RTLD_NOW);
}
#define CloseDynamicLib dlclose
#define GetProc dlsym

#endif

typedef void(*plugin_proc)(dynamix::object*);

TEST_SUITE_BEGIN("plugins");

using namespace dynamix;
using namespace dynamix::combinators;

DYNAMIX_DECLARE_MIXIN(exe_mixin);

TEST_CASE("lib")
{
    object o;

    mutate(o)
        .add<exe_mixin>();

    CHECK(dl_a_multicast<sum>(o) == 1);

    mutate(o)
        .add<dynlib_a_mixin1>();

    CHECK(dl_a_mixin_specific(o) == 101);
    CHECK(dl_a_multicast<sum>(o) == 12);

    mutate(o)
        .add<dynlib_a_mixin2>();

    CHECK(dl_a_mixin_specific(o) == 102);
    CHECK(dl_a_multicast<sum>(o) == 24);
}

DynamicLib LoadPlugin(const char* name, object& o)
{
    auto plugin = LoadDynamicLib(name);
    CHECK(plugin);

    auto modify = reinterpret_cast<plugin_proc>(GetProc(plugin, "modify_object"));
    CHECK(modify);

    modify(&o);

    return plugin;
}

void ClosePlugin(DynamicLib plugin, object& o)
{
    auto release = reinterpret_cast<plugin_proc>(GetProc(plugin, "release_object"));
    CHECK(release);

    release(&o);

    CloseDynamicLib(plugin);
}

TEST_CASE("plugin")
{
    object o;

    mutate(o)
        .add<exe_mixin>()
        .add<dynlib_a_mixin1>()
        .add<dynlib_a_mixin2>();

    auto p = LoadPlugin("test_plugin_A", o);

    CHECK(dl_a_multicast<sum>(o) == 125);
    CHECK(dl_a_exported(o) == 125);

    // simulate reload of plugin
    ClosePlugin(p, o);

    CHECK(dl_a_multicast<sum>(o) == 24);
#if DYNAMIX_USE_EXCEPTIONS
    CHECK_THROWS_AS(dl_a_exported(o), bad_message_call);
#endif

    // apparently dlclose with gnuc doesn't consider unloading the plugin so file
    // this test works with MSVC and clang
#if defined(_WIN32) || !defined(__GNUC__) || defined(__clang__)
    p = LoadPlugin("test_plugin_Amod", o);

    CHECK(dl_a_multicast<sum>(o) == 126);
    CHECK(dl_a_exported(o) == -126);

    ClosePlugin(p, o);
#endif
}

TEST_CASE("shared")
{
    object o;

    mutate(o)
        .add<exe_mixin>()
        .add<dynlib_a_mixin1>()
        .add<dynlib_a_mixin2>();

    auto p = LoadPlugin("test_plugin_B", o);

    CHECK(dl_a_multicast<sum>(o) == 1025);

    ClosePlugin(p, o);

    CHECK(dl_a_multicast<sum>(o) == 24);
}

class exe_mixin
{
public:
    int dl_a_multicast()
    {
        return 1;
    }
};

DYNAMIX_DEFINE_MIXIN(exe_mixin, dl_a_multicast_msg);
