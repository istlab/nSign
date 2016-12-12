/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */


/* vim: set ts=4 sw=4 et tw=79 expandtab:
* jsexception.h
*/

#include "jsexception.h"

#ifdef JSENGINE_EXCEPTIONS
#include <exception>

namespace nSign
{
    /* A simple exception class that holds a std::string message. */

    JSException::JSException(const std::string& msg) : _msg(msg)
    {
    }

    JSException::JSException(const char * msg)
    {
        _msg = std::string(msg);
    }

    JSException::JSException(const JSException& exc)
    {
        _msg = std::string(exc._msg);
    }

    JSException& JSException::operator = (const JSException& exc)
    {
        if (&exc != this)
        {
            _msg = exc._msg;
        }
        return *this;
    }

    const char* JSException::what() const throw()
    {
        return _msg.c_str();
    }

}

#endif /* JSENGINE_EXCEPTIONS */
