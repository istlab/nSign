/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* JSig.cpp
*
*  Sep 23, 2014: Major rewrite to fix incomplete input parsing issue
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <regex>
#include <cstdlib>
#include "jsig.h"

//#define JSIG_DUMP

namespace nSign {

    inline void switch_state(char read, State *s, State newstate)
    {
#ifdef JSIG_DUMP
        cerr << "Read char: " << read << ", switching (" << *s << "->" << newstate << ")" << "\n";
#endif
        *s = newstate;
    }

    std::vector<std::regex> JSig::regexps = std::vector<std::regex>();

    JSig::JSig(std::istream *stream)
    {
        m_data = stream;
        init_counters();
        m_data->unsetf(std::ios_base::skipws);
        m_data->seekg(0, m_data->end);
        m_filelength = m_data->tellg();
        m_data->seekg(0, m_data->beg);
    }

    JSig::~JSig()
    { }

    void JSig::init_regexps()
    {
        if (JSig::regexps.size() == 0)
        {
            std::vector<std::string> v(keywords, keywords + 82);
            for (std::vector<std::string>::iterator it = v.begin(); it != v.end(); ++it)
            {
                std::string pattern = "[[:space:]]";
                pattern += it->c_str();
                pattern += "[[:space:]]";
                std::regex re;
                re.assign(pattern, std::regex_constants::basic);
                regexps.push_back(re);
            }
        }
    }

    bool JSig::is_regexp(std::istream * m_data)
    {
        char c;
        bool is_regexp = false;
        int _read = 0;

        while (*m_data >> c) {

            if (m_data->bad())
            {
                std::cerr << "bad stream" << std::endl;
                perror("bad stream");
                break;
            }

            if (m_data->fail())
            {
                std::cerr << "failed stream" << std::endl;
                perror("failed stream");
                break;
            }

            if (m_data->eof())
                break;

            lookahead.push_back(c);
            _read++;

            if (c == '\n' || c == '\r') {
                break;
            }

            if (c == ';') {
                break;
            }

            if (c == '/' && _read > 1) {
                is_regexp = true;
                break;
            }
        }

        return is_regexp;
    }

    void JSig::init_counters()
    {
        std::vector<std::string> v(keywords, keywords + 82);
        for (std::vector<std::string>::iterator it = v.begin(); it != v.end(); ++it)
        {
            m_counter[*it] = 0;
        }
    }

    bool JSig::parse_char(char c, char &end, int &num_read, State *s, State *prev, std::string &filecontents)
    {
        switch (c)
        {
        case '"':
        {
            switch (*s)
            {
            case esc:
                switch_state(c, s, *prev);
                return true;
            case none:
                switch_state(c, s, strdq);
                break;
            case strdq:
                switch_state(c, s, none);
                break;
            default:
                break;
            }
            return true;
        }
        case '\'':
        {
            switch (*s)
            {
            case esc:
                switch_state(c, s, *prev);
                break;
            case none:
                switch_state(c, s, strsq);
                break;
            case strsq:
                switch_state(c, s, none);
                break;
            default:
                break;
            }
            return true;
        }
        case '\\':
        {
            if (*s == esc)
            {
                switch_state(c, s, *prev);
            }
            else
            {
                *prev = *s;
                switch_state(c, s, esc);
            }
            return true;
        }
        case '/':
        {
            switch (*s)
            {
            case none:
                if (!is_regexp(m_data))
                    switch_state(c, s, begincomment);
                else
                    switch_state(c, s, regexp);

                while (!lookahead.empty())
                {
                    char la = lookahead.front();
                    lookahead.pop_front();
                    num_read++;
                    parse_char(la, end, num_read, s, prev, filecontents);
                }

                break;
            case begincomment:
                switch_state(c, s, singleline);
                break;
            case endmultiline:
                switch_state(c, s, none);
                break;
            case regexp:
                switch_state(c, s, none);
                break;
            case esc:
                switch_state(c, s, *prev);
                return true;
            default:
                break;
            }
            return true;
        }
        case '*':
        {
            switch (*s)
            {
            case esc:
                switch_state(c, s, *prev);
                return true;
            case begincomment:
                switch_state(c, s, multiline);
                break;
            case multiline:
                *prev = multiline;
                switch_state(c, s, endmultiline);
                break;
            default:
                break;
            }
            return true;
        }
        case '\n': case '\r':
        {
            switch (*s)
            {
            case singleline:
                switch_state(c, s, none);
                break;
            default:
                break;
            }
            return true;
        }
        case '{': case '}':
        case '(': case ')':
        case '[': case ']':
        {
            switch (*s)
            {
            case esc:
                switch_state(c, s, *prev);
                return true;
            case singleline: case multiline: case strsq: case strdq: case regexp:
                break;
            default:
#ifdef JSIG_DUMP
                cerr << "Read char:" << c << '\n';
#endif
                m_sig += c;
                filecontents += " ";
                break;
            }
            break;
        }
        case '+': case '-': case ':':
        case ';': case '=': case ',':
        case '>': case '<': case '.':
        case '!': case '#': case '?':
        case '|': case '&': case '%':
        case '$': case '^':
        {
            switch (*s)
            {
            case esc:
                switch_state(c, s, *prev);
                return true;
            case singleline: case multiline: case strdq: case strsq: case regexp:
                return true;
            case endmultiline:
                //filecontents += ' ';
                switch_state(c, s, *prev);
                return true;
            default:
                filecontents += ' ';
                break;
                //continue;
            }
        }
        default:
        {
            switch (*s)
            {
            case esc:
                switch_state(c, s, *prev);
                return true;
            case begincomment:
                switch_state(c, s, none);
                return true;
            case endmultiline:
                switch_state(c, s, multiline);
                break;
            case singleline: case multiline: case strdq: case strsq:
                break;
            default:
                filecontents += c;
#ifdef JSIG_DUMP
                std::cerr << "Read char:" << c << '\n';
#endif
                if (c == '+' || c == '-' || c == '='
                    || c == '*' || c == '/' || c == ';'
                    || c == '!' || c == ':' || c == '?'
                    || c == '|' || c == '&' || c == '%'
                    || c == '>' || c == '<' || c == ','
                    )
                    filecontents += ' ';
                break;
            }
            return true;
        }
        }
        end = c;

        return false;
    }

    void JSig::calc_sig()
    {
        char c, start, end;
        int num_read = 0;
        std::string filecontents;
        nSign::State s = none;
        nSign::State esc_previous = none;

        while (*m_data >> c)
        {
#ifdef JSIG_DUMP
            cerr << "Read:" << num_read << ", char:'" << c << "'\n";
#endif

            if (m_data->bad())
            {
                std::cerr << "bad stream" << std::endl;
                perror("bad stream");
                break;
            }

            if (m_data->fail())
            {
                std::cerr << "failed stream" << std::endl;
                perror("failed stream");
                break;
            }

            if (m_data->eof() || num_read >= m_filelength)
                break;

            num_read++;
            if (num_read == 1)
                start = c;

            if (parse_char(c, end, num_read, &s, &esc_previous, filecontents))
                continue;
        }

#ifdef JSIG_DUMP
        cerr << filecontents << "\n";
#endif
        //JSON detection, using FF's heuristics
        bool is_json = true;
        if (m_sig.length() > 2 && start == '(' && end == ')') 
        {
            const char *p = m_sig.c_str();
            for (unsigned int i = 1; i < m_sig.length() - 1; i++) 
            {
                if (p[i] != '{' && p[i] != '}' &&
                    p[i] != '[' && p[i] != ']') 
                {
                    is_json = false;
                    break;
                }
            }
        }
        else 
        {
            is_json = false;
        }

        if (is_json && m_sig.length() > 0) 
        {
            m_sig = std::string("JSON");
            return;
        }

        int op_idx = 0;
        std::vector<std::regex>::const_iterator rxend = regexps.end();
        std::string::const_iterator fcstart = filecontents.begin(), fcend = filecontents.end();

        for (std::vector<std::regex>::const_iterator it = regexps.begin(); it != rxend; ++it)
        {
            aggregate_keywords_using_regex(fcstart, fcend, it, op_idx);
            //aggregate_keywords(filecontents, op_idx);
            op_idx++;
        }

        m_sig += "\n";

        std::stringstream ss;

        for (std::map<std::string, int>::iterator i = m_counter.begin(); i != m_counter.end(); ++i) 
        {    
            ss << (i->first);
            ss << ":";
            ss << i->second;
            ss << "\n";
        }
        m_sig.append(ss.str());
    }

    void JSig::aggregate_keywords_using_regex(std::string::const_iterator start, std::string::const_iterator end, std::vector<std::regex>::const_iterator it, int op_idx)
    {
        std::ptrdiff_t const count(std::distance(
            std::sregex_iterator(start, end, *it),
            std::sregex_iterator()));

        m_counter[keywords[op_idx]] = (int)count;
    }

    void JSig::aggregate_keywords(std::string &input, int op_idx)
    {
        int count = 0;
        std::string keyword = " " + keywords[op_idx] + " "; //" " is not as robust as [[:space:]] but it will have to do
        size_t nPos = input.find(keyword, 0);
        while (nPos != std::string::npos)
        {
            count++;
            nPos = input.find(keyword, nPos + 1);
        }
        m_counter[keywords[op_idx]] = count;
    }

    std::string JSig::get_signature()
    {
        calc_sig();
        return this->m_sig;
    }

}

/*
void readfile(ifstream * file)
{
char c = NULL; int count = 0;
file->unsetf(ios_base::skipws);

while (*file >> c)
{
count++;
cerr << c;
}
cerr << std::endl << "read:" << count;
}

int main(int argc, char* argv[])
{
if (argc < 2) {
std::cerr << "Usage: " << argv[0] << " <filename>";
return 1;
}

std::ifstream * file = new std::ifstream(argv[1], std::ios::in | ios::binary);

if (!file->is_open()) {
std::cerr << "Cannot open file " << argv[1];
}

nSign::JSig::init_regexps();

nSign::JSig sig(file);


std::cout << sig.get_signature() << "\n";
file->close();

string s;
cin >> s;

file = new std::ifstream(argv[1], std::ios::in | std::ios::binary);
readfile(file);
file->close();

cin >> s;

std::string js = "([{\"project\":{\"id\":14,\"name\":\"AbiWord\"}},{\"project\":{\"id\":143,\"name\":\"Accerciser\"}}])";

istringstream * iss = new istringstream(js, istringstream::in | ios::binary);
nSign::JSig sig1(iss);
cout << sig1.get_signature() << "\n";

return 0;
}
*/
