/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* jssig.h
* 
* The JS Signature class
*/

#ifndef JSSIG_H
#define JSSIG_H

#include <string>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

namespace nSign {

    class JSSignature
    {
    public:
        enum JScriptOrigin
        {
            Unknown = 0x0,
            Inline = 0x1,
            External = 0x2,
            Eval = 0x4
        };
        JSSignature();
        JSSignature(const std::string source, const std::string codebase, const std::string filename, int linenumber, bool keepPath);
        JSSignature(const std::string source, const std::string codebase, const std::string filename, int linenumber, const std::string evalstack, bool keepPath);
        virtual ~JSSignature(){}

        std::string toString();
        std::string getDomain();
        std::string getHash();
        int getSourceLength();
        bool isJSON();

    private:
        std::string     _source;
        std::string     _codebase;
        std::string     _filename;
        unsigned int    _linenumber;
        std::string     _evalstack;
        int             _origin;
        std::string     _sig;
        std::string     _hash;
        std::string     _domain;
        bool            _json;
        bool			_keepPath;

        void init(const std::string source, const std::string codebase, const std::string filename, int linenumber, JSSignature::JScriptOrigin origin, const std::string evalstack, bool keepPath);

        static std::unordered_map<std::string, std::string> _codesigcache;
    };

}
#endif 
