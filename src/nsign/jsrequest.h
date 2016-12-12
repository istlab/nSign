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
* jsrequest.h
*
* Encapsulates a request against a JS API method in order to detect API calls from different threads and JS contexts.
*/

#ifndef JSREQUEST_H_
#define JSREQUEST_H_

#include <mutex>
#include <cstdint>
#include <thread>
#include <stddef.h>
#include <stdint.h>

#include "jsapi.h"

namespace nSign {

    class JSRequest
    {
    private:
        size_t thread_id;
        JSContext * context;
        JSScript * script;
        friend bool operator <(const nSign::JSRequest &left, const nSign::JSRequest &right)
        {
            return (left.thread_id < right.thread_id) ||
                (left.thread_id == right.thread_id && left.context < right.context) ||
                (left.thread_id == right.thread_id && left.context == right.context && left.script < right.script);
        }


    public:
        JSRequest(JSContext * const cx, JSScript * const script);
        ~JSRequest();
        std::string toString();
        bool IsPartialMatch(JSContext * cx, size_t threadid) const;
    };

}

namespace std
{
    template<> struct less<nSign::JSRequest>
    {
        bool operator() (const nSign::JSRequest& left, const nSign::JSRequest& right) const
        {
            return left < right;
        }
    };
}

#endif