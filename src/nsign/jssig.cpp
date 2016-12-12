/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* jssig.cpp
**
The JSSignature implementation
*/

#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include "sha1.h"
#include "jsuri.h"
#include "jssig.h"
#include "jsig.h"
#include "jsstopwatch.h"

namespace nSign {

    std::unordered_map<std::string, std::string> JSSignature::_codesigcache = std::unordered_map<std::string, std::string>();

    JSSignature::JSSignature()
    {
        init("", "", "", 0, Unknown, "", false);
    }

    JSSignature::JSSignature(const std::string source, const std::string codebase, const std::string filename, int linenumber, bool keepPath)
    {
        init(source, codebase, filename, linenumber, Unknown, "", keepPath);
    }

    JSSignature::JSSignature(const std::string source, const std::string codebase, const std::string filename, int linenumber, const std::string evalstack, bool keepPath)
    {
        init(source, codebase, filename, linenumber, Eval, evalstack, keepPath);
    }

    std::string JSSignature::getDomain()
    {
        return _domain;
    }

    std::string JSSignature::getHash()
    {
        if (_hash.empty() && !_json)
        {
            _hash = SHA1::calc(toString());
        }
        return _hash;
    }

    int JSSignature::getSourceLength()
    {
        return _source.size();
    }

    std::string JSSignature::toString()
    {
        if (_sig.empty() && !_json)
        {
            std::stringstream result(std::stringstream::in | std::stringstream::out);

            JSUri curi(_codebase);
            JSUri furi(_filename);

            _domain = curi.getAuthority();
            
            if (_keepPath)
            {
	            result << _domain << "/" << curi.getPath() << " " << furi.getAuthority();
            }
            else
            {
	            result << _domain << " " << furi.getAuthority();
            }
            if ((curi.getHost() != furi.getHost()) && (_linenumber == 1))
                _origin = External;
            else
            {
                if (_domain != furi.getAuthority())
                    _origin |= External;
                else
                    _origin |= Inline;
            }
            result << " " << _origin << " " << std::endl;

            std::string stripped;

            std::string codehash = SHA1::calc(_source);
            std::unordered_map<std::string, std::string>::iterator it = JSSignature::_codesigcache.find(codehash);
            if (it != JSSignature::_codesigcache.end())
            {
                stripped = it->second;
            }
            else
            {
                std::istringstream * iss = new std::istringstream(_source, std::istringstream::in | std::istream::binary);
                JSig *stripper = new JSig(iss);
                if (stripper != NULL)
                {
                    stripped = stripper->get_signature();
                    if (stripped == std::string("JSON"))
                    {
                        delete stripper;
                        //std::cerr << "JSON detected, no sig" << std::endl;
                        _json = true;
                        return std::string();
                    }
                    delete stripper;
                }
                JSSignature::_codesigcache.insert(make_pair(codehash, stripped));
            }
            result << stripped << std::endl;

            if (!_evalstack.empty() && (_origin & Eval) == Eval)
                result << _evalstack << std::endl;

            _sig = result.str();
        }
        return _sig;
    }

    bool JSSignature::isJSON()
    {
        return _json;
    }

    void JSSignature::init(const std::string source, const std::string codebase, const std::string filename, int linenumber, JSSignature::JScriptOrigin origin,  const std::string evalstack, bool keepPath)
    {
        _source = source;
        _codebase = codebase;
        _filename = filename;
        _linenumber = linenumber;
        _evalstack = evalstack;
        _origin = origin;
        _hash = "";
        _sig = "";
        _domain = "";
        _json = false;
        _keepPath = keepPath;
    }
}