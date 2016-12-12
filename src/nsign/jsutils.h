/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

#pragma once

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* jsutils.h
*
* Various global utility methods
*/

#ifndef JSUTILS_H_
#define JSUTILS_H_

#include <mutex>
#include <cstdint>
#include <thread>
#include <stddef.h>
#include <stdint.h>
#include "jsatom.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jsstr.h"

namespace nSign {

    extern std::string JS_GetStringBytes(JSContext *cx, JSString * const str);

    extern std::string JS_GetFrameArgs(JSContext *cx, js::AbstractFramePtr fp);

    extern std::string JS_GetFrameArgs(JSContext *cx, const JS::CallArgs &args);
}

#endif