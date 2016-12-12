/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* jsengine.h
*/

#ifndef JSENGINE_H_
#define JSENGINE_H_

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <ctime>
#include <mutex>
#include <cstdint>
#include <thread>
#include "sqlite3.h"
#include "jssig.h"
#include "jsgd.h"
#include "jsstopwatch.h"
#include "jsuri.h"
#include "jsrequest.h"

namespace nSign {

    enum JSEngineSettings
    {
        None = 0x0,
        Training = 0x01,
        KeepFullScriptOrigin = 0x02, /* When set the entire codebase url is used for the signature, otherwise just the domain */
        ParseFunctionArguments = 0x04, /* When set string arguments passed to compiled functions will be parsed for urls */
        PreventXSS = 0x08, /* When not set XSS attacks will just be flagged / logged during production mode */
        Benchmark = 0x10, /* When not set, AddBenchResult will return immediately */
        Default = Training | ParseFunctionArguments,
        KeepUrlPath = 0x20 /* When set, both the url authority (domain/port) and path are used for the signature, otherwise just the domain */
        //TODO: add other optional features here
    };

    class JSEngine
    {

    private:
        /* constructors and instance management */
        JSEngine();
        JSEngine(JSEngine const&); // Don't allow copy construction
        void operator=(JSEngine const&); // Don't allow static instance reassignment
        static JSEngine *instance;
        static GenericDestructor<JSEngine> destructor;

        /* configuration data structures and caches */
        std::ofstream fout;
        std::ifstream fin;
        std::ofstream flog;
        JSEngineSettings settings;
        std::recursive_mutex sync;
        std::mutex logsync;
        std::map<JSRequest, JSStopWatch *> *requeststopwatches; /* JSRequest => stopwatch* */
        std::map<std::string, std::string> *sigsource; /* signature hash => signature source */
        std::map<std::string, std::pair<time_t, std::set<JSSignature>>> *sighash; /* domain => <retrieval time, set of signatures>*/
        std::map<std::string, std::pair<time_t, std::set<std::string>>> *sigcache; /* domain => <retrieval time, set of signature hashes>*/
        std::map<std::string, std::set<std::string>> *sigurls; /* signature hash => url whitelist. */
        std::set<std::string> *sessiondomains; /* holds the domains which we have visited in the current browser session */
        
        /* signature database associated members */
        sqlite3 *db;
        sqlite3_stmt *insertdomain_stmt;
        sqlite3_stmt *getdomain_stmt;
        sqlite3_stmt *deletedomain_stmt;
        sqlite3_stmt *getdomainsigs_stmt;
        sqlite3_stmt *insertdomainsig_stmt;
        sqlite3_stmt *insertbench_stmt;
        sqlite3_stmt *getsigurls_stmt;
        sqlite3_stmt *insertsigurl_stmt;
        sqlite3_stmt *deletesigurls_stmt;

        /* private methods */
        void DumpSignatures(std::string filename);
        void DumpSignaturesCache();
        std::string CalculateSignature(std::string source, std::string codebase, std::string filename, int line);
        std::string CalculateSignature(std::string source, std::string codebase, std::string filename, int line, std::string evalstack);
        bool LoadSettings();
        bool OpenSignaturesDB();
        void CloseSignaturesDB();
        bool CheckSignaturesDBSchema();
        bool InitSignaturesDB();
        bool PrepareDBStatements();
        bool FinalizeDBStatements();
        void ReleaseStopWatches();
        bool GetDomainTimestamp(const std::string domainName, int &domainid, time_t &timestamp);
        bool GetDomainSignatures(const std::string domainName, std::set<std::string> &sigs);
        bool InsertDomain(const std::string domainName, const time_t timestamp, int &domainid);
        bool InsertDomainSignature(const std::string domainName, const std::string sig);
        bool InsertDomainSignature(int domainID, const std::string sig);
        bool InsertDomainSignatures(const std::string domainName, std::set<std::string> &sigs);
        bool InsertDomainSignatures(int domainID, std::set<std::string> &sigs);
        bool InsertSignatureUrl(const std::string sig, const std::string url);
        bool InsertSignatureUrls(const std::string sig, std::set<std::string> &urls);
        bool GetSignatureUrls(const std::string sig, std::set<std::string> &urls);
        bool DeleteDomain(const std::string domainName); /* deletes a domain and its associated signatures and urls */
        bool RetrieveDomainSignatures(const std::string domainName, std::pair<time_t, std::set<std::string>> &sigs, std::map<std::string, std::set<std::string>> &urls); /* fetches the signatures file from the web server */

    protected:
        friend class GenericDestructor < JSEngine > ;

    public:
        virtual ~JSEngine();
        static JSEngine *Instance();
        bool IsInTrainingMode();
        bool IsSettingEnabled(JSEngineSettings value);
        JSStopWatch * ContextStopWatch(JSContext *context);
        JSStopWatch * RequestStopWatch(JSRequest &request);
        JSStopWatch * AddRequestStopWatch(JSRequest &request, JSStopWatch *sw);
        JSStopWatch * RemoveRequestStopWatch(JSRequest &request);
        bool ValidateSignature(std::string source, std::string codebase, std::string filename, int line, JSStopWatch *sw, std::string &hash, bool *is_json);
        bool ValidateSignature(std::string source, std::string codebase, std::string filename, int line, std::string evalstack, JSStopWatch *sw, std::string &hash, bool *is_json);
        bool ValidateSignatureUrl(std::string const &hash, JSUri &url, JSStopWatch *sw);
        void Log(std::string msg);
        std::ofstream *LogStream();
        bool AddBenchResult(const std::string scripthash, const std::string type, JSStopWatch *sw);
    };

}

#endif /* JSENGINE_H_ */
