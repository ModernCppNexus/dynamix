// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <dynamix/declare_mixin.hpp>

#define DYNAMIX_V1_DECLARE_EXPORTED_MIXIN(export, mixin) \
    DYNAMIX_DECLARE_EXPORTED_MIXIN(export, class mixin)

#define DYNAMIX_V1_DECLARE_MIXIN(mixin) \
    DYNAMIX_V1_DECLARE_EXPORTED_MIXIN(I_DNMX_PP_EMPTY(), class mixin)
