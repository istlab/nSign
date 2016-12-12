/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

#include <mutex>
#include <cstdint>
#include <thread>
#include <stddef.h>
#include <stdint.h>

#include "jsrequest.h"
#include "format.h"
#include "jsapi.h"


using namespace js;
using namespace JS;

namespace nSign
{
    JSRequest::JSRequest(JSContext * const cx, JSScript * const script)
    {
        this->context = cx;
        this->script = script;
        this->thread_id = std::this_thread::get_id().hash();
    }


    JSRequest::~JSRequest()
    {
        context = NULL;
        script = NULL;
    }

    std::string JSRequest::toString()
    {
        return fmt::format("JSRequest, Context: {0}, Script: {1}, Thread:{2}", (void *)context, (void *)script, thread_id);
    }

    bool JSRequest::IsPartialMatch(JSContext * cx, size_t threadid) const
    {
        return context == cx && thread_id == threadid;
    }
}