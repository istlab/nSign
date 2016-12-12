/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
JSUri.h
*/
#ifndef JSURI_H
#define JSURI_H

#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <stdio.h>
#include "format.h"

namespace nSign
{
    /* helper template methods */

    /* Replaces all characters in str with their lower-case counterparts. */
    template <class T> T& toLowerInPlace(T& str)
    {
        typename T::iterator it = str.begin();
        typename T::iterator end = str.end();

        while (it != end) { *it = std::tolower(*it); ++it; }
        return str;
    }

    /*
    A Uniform Resource Identifier, as specified in RFC 3986.
    */
    class JSUri
    {
    public:
        JSUri();
        explicit JSUri(const std::string& uri);
        explicit JSUri(const char* uri);
        virtual ~JSUri(){}

        std::string toString() const;
        const std::string& getScheme() const; /* Returns the scheme part of the URI. */
        const std::string& getHost() const; /* Returns the host part of the URI. */
        std::string getAuthority() const; /* Returns the authority part (userInfo, host and port) of the URI, ommiting well known ports. */
        const std::string& getPathQueryFragment() const; /* Returns the path, query and fragment part of the URI. */
        const std::string& getPath() const; /* Returns the path part of the URI. */

        static std::vector<JSUri> * extractUris(const std::string input); /* Performs URI extraction from text input. */

        bool operator == (const JSUri& uri) const; /* Returns true if both URIs are identical, false otherwise. */
        bool operator == (const std::string& uri) const; /* Parses the given URI and returns true if both URIs are identical, false otherwise. */
        bool operator != (const JSUri& uri) const;
        bool operator != (const std::string& uri) const;

    protected:
        bool equals(const JSUri& uri) const; /* Returns true if both uri's are equivalent. */
        bool isWellKnownPort() const; /* Returns true if the URI's port number is a well-known one, e.g. 80 for http */
        unsigned short getWellKnownPort() const; /* Returns the well known port for the scheme of the URI */
        void parse(const std::string& uri); /* Parses and assigns an URI from the given string. Throws a SyntaxException if the uri is not valid. */
        void parseAuthority(std::string::const_iterator& it, const std::string::const_iterator& end); /* Parses and sets the user-info, host and port from the given data. */
        void parseHostAndPort(std::string::const_iterator& it, const std::string::const_iterator& end); /* Parses and sets the host and port from the given data. */
        void parsePathQueryFragment(std::string::const_iterator& it, const std::string::const_iterator& end); /* Sets the path, query and fragment part from the given data */
        void parsePath(std::string::const_iterator& it, const std::string::const_iterator& end); /* Parses and sets the path from the given data.*/

        static const std::string RESERVED_PATH;
        static const std::string RESERVED_QUERY;
        static const std::string RESERVED_FRAGMENT;
        static const std::string ILLEGAL;
        static const std::string EXTRACT;

        static std::string formatHex(long value, int width);
        /* URI-encodes the given string by escaping reserved and non-ASCII
           characters. The encoded string is appended to encodedStr. */
        static void encode(const std::string& str, const std::string& reserved, std::string& encodedStr);
        /* URI-decodes the given string by replacing percent-encoded characters with the actual character.
           The decoded string is appended to decodedStr.*/
        static void decode(const std::string& str, std::string& decodedStr);
        static bool tryParse(const std::string& s, int& value); /* Tries to parse an integer value in decimal notation from the given string. */
        static void appendNumber(std::string& str, int value); /* Appends a number to a string */

    private:
        std::string    _scheme;
        std::string    _userInfo;
        std::string    _host;
        unsigned short _port;
        std::string    _pathqueryfragment;
        std::string    _path;
        static std::regex regexp;
    };

    /*
    inline functions
    */

    inline const std::string& JSUri::getScheme() const
    {
        return _scheme;
    }

    inline const std::string& JSUri::getHost() const
    {
        return _host;
    }

    inline const std::string& JSUri::getPath() const
    {
        return _path;
    }

    inline const std::string& JSUri::getPathQueryFragment() const
    {
        return _pathqueryfragment;
    }

    inline std::string JSUri::formatHex(long value, int width)
    {
        return fmt::sprintf("%0*lX", width, value);
    }

}

#endif 
