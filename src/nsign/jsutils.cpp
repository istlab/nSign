/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* jsutils.cpp
*
* Implementation of methods defined in jsutils.h
*/

#include "jsatom.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsutils.h"
#include <string>
#include <sstream>
#include "vm/Stack.h"
#include "vm/ArgumentsObject.h"
#include "vm/ScopeObject.h"

namespace nSign {

    std::string JS_GetStringBytes(JSContext *cx, JSString * const str)
    {
        std::string result("");
        size_t size;
        if (str && !str->empty())
        {
            size = str->length();
            char *bytes = new char[size * 2 + 1];

            if (js::DeflateStringToBuffer(cx, str->getCharsZ(cx), size, bytes, &size))
            {
                result.append(bytes, size);
            }
            delete[] bytes;
        }
        return result;
    }

    std::string JS_GetFrameArgs(JSContext *cx, js::AbstractFramePtr fp)
    {
        if (!fp.hasArgs())
            return std::string("");

        js::Value value;
        value.setUndefined();
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        for (unsigned int arg = 0; arg < fp.numActualArgs(); arg++)
        {
            JSScript * script = fp.script();
            /*if (script->formalIsAliased(arg))
            {
                for (js::AliasedFormalIter fi(script);; fi++) {
                    if (fi.frameIndex() == arg)
                    {
                        value = fp.callObj().aliasedVar(fi);
                        break;
                    }
                }
            }
            else */ if (script->argsObjAliasesFormals() && fp.hasArgsObj())
            {
                value = fp.argsObj().arg(arg);
            }
            else
            {
                value = fp.unaliasedActual(arg, js::MaybeCheckAliasing::DONT_CHECK_ALIASING);
            }

            if (value.isString())
            {
                JSString *val = value.toString();
                ss << JS_GetStringBytes(cx, val) << std::endl;
            }
        }

        return ss.str();
    }

    std::string JS_GetFrameArgs(JSContext *cx, const JS::CallArgs &args)
    {
        if (args.length() == 0)
            return "";

        js::Value value;
        value.setUndefined();
        std::stringstream ss(std::stringstream::in | std::stringstream::out);

        for (unsigned int i = 0; i < args.length(); i++)
        {
            value = args.get(i);
            if (value.isString())
            {
                JSString *val = value.toString();
                ss << JS_GetStringBytes(cx, val) << std::endl;
            }
        }

        return ss.str();
    }

}
