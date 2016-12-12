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

#ifndef JSEXCEPTION_H_
#define JSEXCEPTION_H_

#ifdef JSENGINE_EXCEPTIONS
#include <exception>
#include <string>
#endif

namespace nSign {

//#define JSENGINE_EXCEPTIONS
#ifdef JSENGINE_EXCEPTIONS

    /* A simple exception class that holds a std::string message. */
    class JSException : public std::exception
    {
    public:
        JSException(const std::string& msg);
        /// Creates an exception.

        JSException(const char * msg);
        /// Creates an exception.

        JSException(const JSException& exc);
        /// Copy constructor.

        ~JSException() throw(){}
        /// Destroys the exception.

        JSException& operator = (const JSException& exc);
        /// Assignment operator.

        virtual const char* what() const throw();
        /// Returns a static string describing the exception.

        const std::string& message() const;
        /// Returns the message text.

    private:
        std::string _msg;
    };

    /* inline methods */

    inline const std::string& JSException::message() const
    {
        return _msg;
    }

#endif /* JSENGINE_EXCEPTIONS */

}

#endif /* JSEXCEPTION_H_ */