/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
JSeh.h
*/

#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <iostream>
#include "jseh.h"

#ifdef BOOST_EXCEPTION_HANDLER

namespace boost
{
    void throw_exception(std::exception const &e)
    {
        std::cout << "Boost Exception:" << e.what() << std::endl;
    }
}

#endif
