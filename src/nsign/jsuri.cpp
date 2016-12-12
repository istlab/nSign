/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
JSUri.cpp
*/

#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <sstream>
#include <vector>
#include <regex>

#include "jsuri.h"
//#include "jsexception.h"
#include <stdio.h>

namespace nSign {

	int ThrowException(const std::string &msg)
	{
#ifdef JSENGINE_EXCEPTIONS
		//throw new JSException(msg);
#endif
		return -1;
	}

	int ThrowException(const char *msg)
	{
#ifdef JSENGINE_EXCEPTIONS
		//throw new JSException(msg);
#endif
		return -1;
	}

	const std::string JSUri::RESERVED_PATH = "?#";
	const std::string JSUri::RESERVED_QUERY = "#";
	const std::string JSUri::RESERVED_FRAGMENT = "";
	const std::string JSUri::ILLEGAL = "%<>{}|\\\"^`";
	const std::string JSUri::EXTRACT = "((?:[\\w-]+://?|www[.])[^\\s()<>]+(?:\\([\\w\\d]+\\)|([^[:punct:]\\s]|/)))";

	std::regex JSUri::regexp = std::regex(JSUri::EXTRACT);

	/* constructors */
	JSUri::JSUri() :
		_port(0)
	{
	}

	JSUri::JSUri(const std::string& uri) :
		_port(0)
	{
		parse(uri);
	}

	JSUri::JSUri(const char* uri) :
		_port(0)
	{
		parse(std::string(uri));
	}

	std::string JSUri::toString() const
	{
		std::string uri;
		uri += _scheme;
		uri += ':';
		std::string auth = getAuthority();
		if (!auth.empty() || _scheme == "file")
		{
			uri.append("//");
			uri.append(auth);
		}
		if (!_pathqueryfragment.empty())
		{
			if (!auth.empty() && _pathqueryfragment[0] != '/')
				uri += '/';
			encode(_pathqueryfragment, RESERVED_PATH, uri);
		}
		return uri;
	}

	void JSUri::encode(const std::string& str, const std::string& reserved, std::string& encodedStr)
	{
		for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
		{
			char c = *it;
			if ((c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') ||
				c == '-' || c == '_' ||
				c == '.' || c == '~')
			{
				encodedStr += c;
			}
			else if (c <= 0x20 || c >= 0x7F || ILLEGAL.find(c) != std::string::npos || reserved.find(c) != std::string::npos)
			{
				encodedStr += '%';
				encodedStr += formatHex((long)(unsigned char)c, 2);
			}
			else encodedStr += c;
		}
	}

	void JSUri::decode(const std::string& str, std::string& decodedStr)
	{
		std::string::const_iterator it = str.begin();
		std::string::const_iterator end = str.end();
		while (it != end)
		{
			char c = *it++;
			if (c == '%')
			{
				if (it == end)
				{
					ThrowException("SyntaxException:URI encoding: no hex digit following percent sign");
					return;
				}
				char hi = *it++;
				if (it == end)
				{
					ThrowException("SyntaxException:URI encoding: two hex digits must follow percent sign");
					return;
				}
				char lo = *it++;
				if (hi >= '0' && hi <= '9')
					c = hi - '0';
				else if (hi >= 'A' && hi <= 'F')
					c = hi - 'A' + 10;
				else if (hi >= 'a' && hi <= 'f')
					c = hi - 'a' + 10;
				else
				{
					ThrowException("SyntaxException:URI encoding: not a hex digit");
					return;
				}
				c *= 16;
				if (lo >= '0' && lo <= '9')
					c += lo - '0';
				else if (lo >= 'A' && lo <= 'F')
					c += lo - 'A' + 10;
				else if (lo >= 'a' && lo <= 'f')
					c += lo - 'a' + 10;
				else
				{
					ThrowException("SyntaxException:URI encoding: not a hex digit");
					return;
				}
			}
			decodedStr += c;
		}
	}

	bool JSUri::isWellKnownPort() const
	{
		return _port == getWellKnownPort();
	}

	unsigned short JSUri::getWellKnownPort() const
	{
		if (_scheme == "ftp")
			return 21;
		else if (_scheme == "ssh")
			return 22;
		else if (_scheme == "telnet")
			return 23;
		else if (_scheme == "http")
			return 80;
		else if (_scheme == "nntp")
			return 119;
		else if (_scheme == "ldap")
			return 389;
		else if (_scheme == "https")
			return 443;
		else
			return 0;
	}

	std::string JSUri::getAuthority() const
	{
		std::string auth;
		if (!_userInfo.empty())
		{
			auth.append(_userInfo);
			auth += '@';
		}
		auth.append(_host);
		if (_port && !isWellKnownPort())
		{
			auth += ':';
			appendNumber(auth, _port);
		}
		return auth;
	}

	void JSUri::parse(const std::string& uri)
	{
		std::string::const_iterator it = uri.begin();
		std::string::const_iterator end = uri.end();
		if (it == end) return;
		if (*it != '/' && *it != '.' && *it != '?' && *it != '#')
		{
			std::string scheme;
			while (it != end && *it != ':' && *it != '?' && *it != '#' && *it != '/') scheme += *it++;
			if (it != end && *it == ':')
			{
				++it;
				if (it == end)
				{
					ThrowException("SyntaxException:URI scheme must be followed by authority or path");
					return;
				}

				_scheme = scheme;
				toLowerInPlace(_scheme);
				if (_port == 0)
					_port = getWellKnownPort();

				if (*it == '/')
				{
					++it;
					if (it != end && *it == '/')
					{
						++it;
						parseAuthority(it, end);
					}
					else --it;
				}
				parsePathQueryFragment(it, end);
			}
			else
			{
				it = uri.begin();
				parsePathQueryFragment(it, end);
			}
		}
		else parsePathQueryFragment(it, end);
	}

	void JSUri::parseAuthority(std::string::const_iterator& it, const std::string::const_iterator& end)
	{
		std::string userInfo;
		std::string part;
		while (it != end && *it != '/' && *it != '?' && *it != '#')
		{
			if (*it == '@')
			{
				userInfo = part;
				part.clear();
			}
			else part += *it;
			++it;
		}
		std::string::const_iterator pbeg = part.begin();
		std::string::const_iterator pend = part.end();
		parseHostAndPort(pbeg, pend);
		_userInfo = userInfo;
	}

	void JSUri::parseHostAndPort(std::string::const_iterator& it, const std::string::const_iterator& end)
	{
		if (it == end) return;
		std::string host;
		if (*it == '[')
		{
			// IPv6 address
			while (it != end && *it != ']') host += *it++;
			if (it == end)
			{
				ThrowException("SyntaxException:unterminated IPv6 address");
				return;
			}
			host += *it++;
		}
		else
		{
			while (it != end && *it != ':') host += *it++;
		}
		if (it != end && *it == ':')
		{
			++it;
			std::string port;
			while (it != end) port += *it++;
			if (!port.empty())
			{
				int nport = 0;
				if (tryParse(port, nport) && nport > 0 && nport < 65536)
					_port = (unsigned short)nport;
				else
				{
					ThrowException("SyntaxException:bad or invalid port number");
					return;
				}
			}
			else _port = getWellKnownPort();
		}
		else _port = getWellKnownPort();
		_host = host;
		toLowerInPlace(_host);
	}

	void JSUri::parsePathQueryFragment(std::string::const_iterator& it, const std::string::const_iterator& end)
	{
		if (it == end) return;
		_pathqueryfragment.clear();
		
		if (*it != '?' && *it != '#')
			parsePath(it, end);

		while (it != end)
			_pathqueryfragment += *it++;
	}

	void JSUri::parsePath(std::string::const_iterator& it, const std::string::const_iterator& end)
	{
		if (it == end) return;
		_path.clear();
		std::string path;
		while (it != end && *it != '?' && *it != '#')
		{
			path += *it++;
		}
		decode(path, _path);
	}

    bool JSUri::tryParse(const std::string& s, int& value)
    {
#ifdef _MSC_VER
        char temp;
        return sscanf_s(s.c_str(), "%d%c", &value, &temp) == 1;
#else
        std::stringstream str(s);
        str >> value;
        return true;
#endif
    }

    void JSUri::appendNumber(std::string& str, int value)
    {
        str.append(fmt::format("{0}", value));
    }

    std::vector<JSUri> * JSUri::extractUris(const std::string input)
    {
        std::vector<JSUri> * result = new std::vector<JSUri>();
        
        for (std::sregex_iterator it(input.begin(), input.end(), regexp), end; it != end; ++it) 
        {
#ifdef JSENGINE_EXCEPTIONS
            try
            {
#endif
                JSUri *uri = new JSUri(it->str());
                if (uri)
                    result->push_back(*uri);
#ifdef JSENGINE_EXCEPTIONS
            }
            catch (...)
            {
                continue;
            }
#endif
            //std::cout << "Found at: " << it->position() << std::endl;
        }

        return result;
    }
}