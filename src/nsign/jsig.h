/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* JSig.h
*
*  Sep 23, 2014: Major rewrite to fix incomplete input parsing issue
*/

#ifndef JSIG_H_
#define JSIG_H_

#include <iostream>
#include <map>
#include <vector>
#include <list>
#include <regex>

namespace nSign {

    const static std::string keywords[82] = {
        "abstract", "array", "as", "block", "boolean", "break", "byte", "case", "catch", "char", 
        "class", "continue", "const", "date", "debugger", "decodeURI", "decodeURIComponent", "default", "delete", "do", 
        "double", "else", "enum", "eval", "encodeURI", "encodeURIComponent", "export", "extends", "false", "final", 
        "finally", "float", "for", "function", "goto", "if", "implements", "import", "in", "instanceof",
        "is", "isFinite", "isNaN", "long", "math", "max", "min", "namespace", "native", "new", 
        "null", "number", "object", "package", "parseFloat", "parseInt", "private", "protected", "public", "RegExp",
        "random", "return", "round", "sort", "static", "string", "super", "switch", "synchronized", "this", 
        "throw", "throws", "transient", "true", "try", "typeof", "use", "var", "void", "volatile", 
        "while", "with"
    };

    typedef enum State
    {
        none, strdq, strsq, begincomment, multiline,
        singleline, endmultiline, regexp, esc
    } State;

    class JSig {

    private:
        std::map<std::string, int> m_counter;
        std::string m_sig;
        std::istream * m_data;
        std::streamsize m_filelength;
        std::list<char> lookahead;
        static std::vector<std::regex> regexps;
        bool is_regexp(std::istream * m_data);
        bool parse_char(char c, char &end, int &num_read, State *s, State *prev, std::string &filecontents);
        void calc_sig();
        void init_counters();
        void aggregate_keywords_using_regex(std::string::const_iterator start, std::string::const_iterator end, std::vector<std::regex>::const_iterator it, int op_idx);
        void aggregate_keywords(std::string &input, int op_idx);
    public:
        JSig(std::istream *);
        virtual ~JSig();
        std::string get_signature();
        static void init_regexps();
    };

}

#endif /* JSIG_H_ */
