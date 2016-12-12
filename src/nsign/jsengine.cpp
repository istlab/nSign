/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* JSEngine.cpp
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <thread>
#include <cstdint>
#include <stddef.h>

#include "jsgd.h"
#include "jsengine.h"
#include "jssig.h"
#include "jsuri.h"
#include "sqlite3.h"
#include "jsig.h" /* for regex initialization */
#include "jsstopwatch.h" /* for stopwatch bench */
#include "sha1.h"

//this needs to be used unless the necessary configuration for sqlite is included in the js config file,
//and sqlite3.c is included in sources
#ifdef _MSC_VER
#pragma comment (lib, "sqlite3.lib")
#endif

using namespace std;

namespace nSign {

    JSEngine * JSEngine::instance = NULL;
    GenericDestructor<JSEngine> JSEngine::destructor;

    JSEngine::JSEngine()
    {
        LoadSettings();
        flog.open("jsengine.log");
        requeststopwatches = new map<JSRequest, JSStopWatch *>();
        sigsource = new map<string, string>();
        sighash = new map<string, pair<time_t, set<JSSignature> > >();
        sigcache = new map<string, pair<time_t, set<string> > >();
        sigurls = new map<string, set<string> >();
        sessiondomains = new set<string>();
        if (OpenSignaturesDB())
        {
            if (!CheckSignaturesDBSchema())
            {
                if (!InitSignaturesDB())
                {
                    Log(fmt::format("ERROR: JSEngine failed to initialize Signatures DB: {0}:{1}", sqlite3_errcode(db), sqlite3_errmsg(db)));
                }
            }
            if (!PrepareDBStatements())
            {
                Log(fmt::format("ERROR: JSEngine failed to initialize prepared statements for Signatures DB: {0}:{1}", sqlite3_errcode(db), sqlite3_errmsg(db)));
            }
        }
        else
            Log("ERROR: JSEngine Signatures DB is unavailable:");

        Log(IsInTrainingMode() ? "JSEngine operation mode: training" : "JSEngine operation mode: production");
    }

    JSEngine::~JSEngine()
    {
        DumpSignatures(string("signatures.txt"));
        DumpSignaturesCache();
        delete(sigsource);
        delete(sighash);
        delete(sigcache);
        delete(sigurls);
        ReleaseStopWatches();
        FinalizeDBStatements();
        CloseSignaturesDB();
        flog.close();
    }

    JSEngine * JSEngine::Instance()
    {
        if (instance == NULL)
        {
            instance = new JSEngine();
            destructor.MarkForDestruction(instance);
            JSig::init_regexps();
        }
        return instance;
    }

    bool JSEngine::LoadSettings()
    {
        bool result = true;

        ifstream fsettings("nSign.conf");
        if (fsettings.fail()) {
            cerr << "ERROR: JSEngine: Cannot open config file, using default settings" << endl;
            settings = JSEngineSettings::Default;
            result = false;
        }
        else
        {
            int mode = 0;
            fsettings >> mode;
            settings = (JSEngineSettings)mode;
        }
        fsettings.close();

        return result;
    }

    void JSEngine::Log(string msg)
    {
        std::lock_guard<std::mutex> lock(logsync);
        if (flog.is_open())
        {
            flog << msg << std::endl;
        }
        //cerr << msg << std::endl;
    }

    std::ofstream *JSEngine::LogStream()
    {
        return &flog;
    }

    bool JSEngine::IsInTrainingMode()
    {
        return IsSettingEnabled(JSEngineSettings::Training);
    }

    bool JSEngine::IsSettingEnabled(JSEngineSettings value)
    {
        return ((int)settings & (int)value) == (int)value;
    }

    JSStopWatch * JSEngine::ContextStopWatch(JSContext *context)
    {
        //std::lock_guard<std::recursive_mutex> lock(sync);
        size_t current_thread_id = std::this_thread::get_id().hash();
        JSStopWatch *sw = NULL;
        int matches = 0;

        for (map<JSRequest, JSStopWatch *>::iterator it = requeststopwatches->begin(); it != requeststopwatches->end(); it++)
        {
            if (it->first.IsPartialMatch(context, current_thread_id))
            {
                if (++matches > 1)
                {
                    break;
                }
                sw = it->second;
            }
        }

        if (matches == 1)
        {
            return sw;
        }

        return NULL;
    }

    JSStopWatch * JSEngine::RequestStopWatch(JSRequest &request)
    {
        std::lock_guard<std::recursive_mutex> lock(sync);
        JSStopWatch *sw = NULL;

        map<JSRequest, JSStopWatch *>::iterator it = (requeststopwatches->find(request));

        if (it != requeststopwatches->end())
        {
            sw = it->second;
        }
        //TODO: Get or add
        return sw;
    }

    JSStopWatch * JSEngine::AddRequestStopWatch(JSRequest &request, JSStopWatch *sw)
    {
        std::lock_guard<std::recursive_mutex> lock(sync);
        JSStopWatch * stopwatch = (sw != NULL) ? sw : new JSStopWatch();
        requeststopwatches->insert(make_pair(request, stopwatch));
        return stopwatch;
    }

    JSStopWatch * JSEngine::RemoveRequestStopWatch(JSRequest &request)
    {
        std::lock_guard<std::recursive_mutex> lock(sync);
        JSStopWatch * stopwatch = RequestStopWatch(request);
        if (stopwatch)
        {
            requeststopwatches->erase(request);
        }
        return stopwatch;
    }

    void JSEngine::ReleaseStopWatches()
    {
        //std::lock_guard<std::recursive_mutex> lock(sync);
        if (requeststopwatches == NULL)
            return;

        for (std::map<JSRequest, JSStopWatch *>::iterator it = requeststopwatches->begin(); it != requeststopwatches->end(); it++)
        {
            if ((*it).second != NULL)
            {
                delete ((*it).second);
            }
        }

        requeststopwatches->clear();
        delete(requeststopwatches);
    }

    bool JSEngine::ValidateSignature(string source, string codebase, string
        filename, int line, JSStopWatch * sw, string &hash, bool *is_json)
    {
        return ValidateSignature(source, codebase, filename, line, string(""), sw,
            hash, is_json);
    }

    bool JSEngine::ValidateSignature(string source, string codebase, string
        filename, int line, string evalstack, JSStopWatch *sw, string &hash, bool *is_json)
    {
        //std::lock_guard<std::recursive_mutex> lock(sync);
        bool result = false;
        JSUri uri(codebase);
        string domain = uri.getAuthority();

        sw->StartDb();
        if (sessiondomains->find(domain) == sessiondomains->end())
        {
            /* first visit to this domain for the current session, retrieve its signatures */

            std::pair<time_t, set<string> > domainsigs;
            int domainid;
            time_t dbtimestamp;
            if (RetrieveDomainSignatures(domain, domainsigs, *sigurls))
            {
                /* compare the timestamp of the fetched signatures with the one stored in the db */
                /* The domain is not in the db or the signatures in the db are outdated */
                if (!GetDomainTimestamp(domain, domainid, dbtimestamp) || (domainsigs.first > dbtimestamp))
                {
                    if (domainid > 0)
                        DeleteDomain(domain); /* delete outdated domain and its signatures */

                    InsertDomain(domain, domainsigs.first, domainid);
                    InsertDomainSignatures(domainid, domainsigs.second);
                }
                else
                {
                    if (dbtimestamp == 0)
                        dbtimestamp = time(NULL);

                    domainsigs.first = dbtimestamp;
                    GetDomainSignatures(domain, domainsigs.second);
                }
                (*sigcache)[domain] = domainsigs;
            }
            else
            {
                /* failed to retrieve fresh signatures file, load whatever is already in the db */
                if (GetDomainTimestamp(domain, domainid, dbtimestamp))
                {
                    domainsigs.first = dbtimestamp;
                    GetDomainSignatures(domain, domainsigs.second);
                }
                (*sigcache)[domain] = domainsigs;
            }
            sessiondomains->insert(domain);
        }
        sw->StopDb();
        /* generate the signature and check if it is valid (checking mode) or insert it in the db and cache (training mode)*/
        bool keepPath = IsSettingEnabled(JSEngineSettings::KeepUrlPath);
        sw->StartSig();
        JSSignature signature(source, codebase, filename, line, evalstack, keepPath);
        string sig = signature.getHash();
        string s = signature.toString();
        sw->StopSig();

        if (s.empty())
        { //JSON file
            *is_json = true;
            hash = "";
            return true;
        }

        sigsource->insert(make_pair(sig, s));
        hash.assign(sig);

        std::pair<time_t, set<string> > *ds;
        ds = &((*sigcache)[domain]);

        if (IsInTrainingMode())
        {
            sw->StartDb();
            if (ds->second.insert(sig).second)
            {
                int domainid;
                time_t dbtimestamp;
                if (!GetDomainTimestamp(domain, domainid, dbtimestamp))
                {
                    dbtimestamp = time(NULL);
                    InsertDomain(domain, dbtimestamp, domainid);
                }
                InsertDomainSignature(domainid, sig);
            }
            sw->StopDb();
            result = true;
        }
        else
        {
            if (ds->second.find(sig) != ds->second.end())
                result = true; /* valid signature */
            else
            {
                result = false;
                Log("Possible XSS attack detected.");
            }
        }
        return result;
    }

    bool JSEngine::ValidateSignatureUrl(string const &hash, JSUri &url, JSStopWatch *sw)
    {
        //std::lock_guard<std::recursive_mutex> lock(sync);
        bool result = false;
        string urlsig = SHA1::calc(url.getAuthority());

        string sig(hash, 40);

        sw->StartDb();
        if (IsInTrainingMode())
        {
            if (sigurls->find(sig) == sigurls->end())
                sigurls->insert(make_pair(sig, set<string>()));

            map<string, set<string>>::iterator it = (sigurls->find(sig));
            if (it->second.insert(urlsig).second)
            {
                InsertSignatureUrl(sig, urlsig);
            }
            result = true;
        }
        else
        {
            if (sigurls->find(sig) == sigurls->end())
                result = false;
            else
            {
                map<string, set<string>>::iterator it = (sigurls->find(sig));
                if ((it->second.find(urlsig) != it->second.end()))
                {
                    //found associated url
                    result = true;
                }
                else
                {
                    Log(fmt::format("Possible XSS attack detected from URL:{0}", url.toString()));
                    result = false;
                }
            }
        }
        sw->StopDb();

        return result;
    }

    void JSEngine::DumpSignatures(string filename)
    {
        if (!fout.is_open())
        {
            fout.open(filename);
        }
        for (std::map<std::string, std::string>::iterator it = sigsource->begin(); it != sigsource->end(); it++)
        {
            fout << (*it).first << "\t" << (*it).second << std::endl;
        }
        fout.close();
    }

    void JSEngine::DumpSignaturesCache()
    {
        for (map<string, pair<time_t, set<string> > >::iterator it = sigcache->begin(); it != sigcache->end(); it++)
        {
            string domain = (*it).first;
            if (domain.empty())
                continue;

            string filename = "cache_";
            filename += domain;
            filename += ".txt";

            ofstream fout(filename);

            time_t timestamp = (*it).second.first;
            fout << timestamp << endl;

            for (set<string>::iterator sit = (*it).second.second.begin(); sit != (*it).second.second.end(); sit++)
            {
                fout << (*sit) << endl;

                if (sigurls->find(*sit) != sigurls->end())
                {
                    set<string> surls = (sigurls->find(*sit))->second;

                    for (set<string>::iterator urlit = surls.begin(); urlit != surls.end(); urlit++)
                    {
                        fout << ">" << (*urlit) << std::endl;
                    }
                }
            }
            fout.close();
        }
    }

    string JSEngine::CalculateSignature(string source, string codebase, string filename, int line)
    {
        return CalculateSignature(source, codebase, filename, line, string(""));
    }

    string JSEngine::CalculateSignature(string source, string codebase, string filename, int line, string evalstack)
    {
        bool keepPath = IsSettingEnabled(JSEngineSettings::KeepUrlPath);
        JSSignature sig(source, codebase, filename, line, evalstack, keepPath);
        pair<map<string, string>::iterator, bool> ret;
        std::string s = sig.toString();
        std::string h = sig.getHash();
        ret = sigsource->insert(std::pair<string, string>(h, s));
        /*if(ret.second)
        {
        int id = 0;
        InsertDomain(sig.getDomain(), time(NULL), id);
        }*/
        return s;
    }

    bool JSEngine::OpenSignaturesDB()
    {
        int flags = SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
        if (sqlite3_open_v2("signatures.db", &db, flags, NULL) != SQLITE_OK)
        {
            if (db != NULL)
            {
                Log(fmt::format("JSEngine: Open or create db failed: {0}:{1}", sqlite3_errcode(db), sqlite3_errmsg(db)));
                sqlite3_close(db);
            }
            db = NULL;
            return false;
        }
        char *err = 0;
        sqlite3_exec(db, "PRAGMA journal_mode=off;", NULL, 0, &err);
        sqlite3_exec(db, "PRAGMA synchronous=0;", NULL, 0, &err);
        sqlite3_exec(db, "PRAGMA temp_store=3;", NULL, 0, &err);
        return true;
    }

    static int CheckSignaturesDBSchemaCallback(void *unused, int argc, char **argv, char **colNames)
    {
        if (argc > 1)
            return 1;
        if (strcmp("4", argv[0]) != 0)
            return 2;
        return 0;
    }

    bool JSEngine::CheckSignaturesDBSchema()
    {
        const char *query = "SELECT count(*) AS tablecount FROM sqlite_master WHERE name in ('domain', 'signature', 'urls', 'bench');";
        int rc = 0;
        char *err = 0;
        rc = sqlite3_exec(db, query, CheckSignaturesDBSchemaCallback, 0, &err);
        if (rc != SQLITE_OK)
        {
            sqlite3_free(err);
            return false;
        }
        return true;
    }

    void JSEngine::CloseSignaturesDB()
    {
        if (db)
        {
            sqlite3_close(db);
        }
    }

    bool JSEngine::InitSignaturesDB()
    {
        int rc;
        char *err = 0;

        rc = sqlite3_exec(db, "CREATE TABLE domain(id INTEGER PRIMARY KEY, domainname VARCHAR(100) NOT NULL, tld VARCHAR(100), timestamp INTEGER NOT NULL);", NULL, NULL, &err);
        if (rc != SQLITE_OK)
        {
            sqlite3_free(err);
            return false;
        }

        rc = sqlite3_exec(db, "CREATE UNIQUE INDEX ix_domain ON domain (domainname ASC);", NULL, NULL, &err);
        if (rc != SQLITE_OK)
        {
            sqlite3_free(err);
            return false;
        }

        rc = sqlite3_exec(db, "CREATE TABLE signature ( domainid INTEGER, hash CHAR(41), PRIMARY KEY (domainid, hash), FOREIGN KEY(domainid) REFERENCES domain(id) ON UPDATE CASCADE ON DELETE CASCADE);", NULL, NULL, &err);
        if (rc != SQLITE_OK)
        {
            sqlite3_free(err);
            return false;
        }

        rc = sqlite3_exec(db, "CREATE TABLE bench (db INTEGER, hash CHAR(41), script INTEGER, sig INTEGER, stack INTEGER, args INTEGER, type CHAR(5));", NULL, NULL, &err);
        if (rc != SQLITE_OK)
        {
            sqlite3_free(err);
            return false;
        }

        rc = sqlite3_exec(db, "CREATE INDEX bench_idx ON bench(hash ASC);", NULL, NULL, &err);
        if (rc != SQLITE_OK)
        {
            sqlite3_free(err);
            return false;
        }

        rc = sqlite3_exec(db, "CREATE TABLE urls (hash CHAR(41), url CHAR(41), PRIMARY KEY (hash, url), FOREIGN KEY(hash) REFERENCES signature(hash) ON UPDATE CASCADE ON DELETE CASCADE);", NULL, NULL, &err);
        if (rc != SQLITE_OK)
        {
            sqlite3_free(err);
            return false;
        }

        rc = sqlite3_exec(db, "CREATE INDEX url_idx ON url(hash ASC);", NULL, NULL, &err);
        if (rc != SQLITE_OK)
        {
            sqlite3_free(err);
            return false;
        }

        return true;
    }

    bool JSEngine::PrepareDBStatements()
    {
        const char* insertdomain_query = "INSERT INTO domain(id, domainname, timestamp) VALUES(NULL, ?, ?);";
        const char* getdomain_query = "SELECT id, domainname, timestamp FROM domain WHERE domainname = ?;";
        const char* deletedomain_query = "DELETE FROM domain WHERE id = ?;";
        const char *getdomainsigs_query = "SELECT hash FROM signature WHERE domainid = (SELECT id from domain where domainname = ?);";
        const char *insertdomainsig_query = "INSERT INTO signature(domainid, hash) VALUES(?, ?);";
        const char *insertbench_query = "INSERT INTO bench (hash, sig, db, stack, script, args, type) VALUES(?, ?, ?, ?, ?, ?, ?);";
        const char *getsigurls_query = "SELECT hash, url FROM urls WHERE hash = ?;";
        const char *insertsigurl_query = "INSERT INTO urls(hash, url) VALUES(?, ?);";
        const char *deletesigurls_query = "DELETE FROM urls WHERE hash = ?;";

        if (sqlite3_prepare_v2(db, insertdomain_query, -1, &insertdomain_stmt, 0) != SQLITE_OK)
            return false;
        if (sqlite3_prepare_v2(db, getdomain_query, -1, &getdomain_stmt, 0) != SQLITE_OK)
            return false;
        if (sqlite3_prepare_v2(db, deletedomain_query, -1, &deletedomain_stmt, 0) != SQLITE_OK)
            return false;
        if (sqlite3_prepare_v2(db, getdomainsigs_query, -1, &getdomainsigs_stmt, 0) != SQLITE_OK)
            return false;
        if (sqlite3_prepare_v2(db, insertdomainsig_query, -1, &insertdomainsig_stmt, 0) != SQLITE_OK)
            return false;
        if (sqlite3_prepare_v2(db, insertbench_query, -1, &insertbench_stmt, 0) != SQLITE_OK)
            return false;
        if (sqlite3_prepare_v2(db, getsigurls_query, -1, &getsigurls_stmt, 0) != SQLITE_OK)
            return false;
        if (sqlite3_prepare_v2(db, insertsigurl_query, -1, &insertsigurl_stmt, 0) != SQLITE_OK)
            return false;
        if (sqlite3_prepare_v2(db, deletesigurls_query, -1, &deletesigurls_stmt, 0) != SQLITE_OK)
            return false;

        return true;
    }

    bool JSEngine::FinalizeDBStatements()
    {
        bool result = true;

        if (sqlite3_finalize(insertdomain_stmt) != SQLITE_OK)
            result = false;
        if (sqlite3_finalize(getdomain_stmt) != SQLITE_OK)
            result = false;
        if (sqlite3_finalize(deletedomain_stmt) != SQLITE_OK)
            result = false;
        if (sqlite3_finalize(getdomainsigs_stmt) != SQLITE_OK)
            result = false;
        if (sqlite3_finalize(insertdomainsig_stmt) != SQLITE_OK)
            result = false;
        if (sqlite3_finalize(insertbench_stmt) != SQLITE_OK)
            result = false;
        if (sqlite3_finalize(getsigurls_stmt) != SQLITE_OK)
            result = false;
        if (sqlite3_finalize(insertsigurl_stmt) != SQLITE_OK)
            result = false;
        if (sqlite3_finalize(deletesigurls_stmt) != SQLITE_OK)
            result = false;

        return result;
    }

    bool JSEngine::GetDomainTimestamp(const string domainName, int & domainid, time_t & timestamp)
    {
        bool result = false;
        if (sqlite3_bind_text(getdomain_stmt, 1, domainName.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
            return result;

        int rc = sqlite3_step(getdomain_stmt);
        if (rc == SQLITE_ROW)
        {
            domainid = sqlite3_column_int(getdomain_stmt, 0);
            timestamp = (time_t)sqlite3_column_int64(getdomain_stmt, 1);
            rc = sqlite3_step(getdomain_stmt); /* TODO: assert rc == SQLITE_DONE */
            result = true;
        }
        sqlite3_reset(getdomain_stmt);
        sqlite3_clear_bindings(getdomain_stmt);
        return result;
    }

    bool JSEngine::GetDomainSignatures(const string domainName, set<string> & sigs)
    {
        bool result = false;
        if (sqlite3_bind_text(getdomainsigs_stmt, 1, domainName.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
            return result;

        int rc = 0;
        while ((rc = sqlite3_step(getdomainsigs_stmt)) == SQLITE_ROW)
        {
            char *hash = (char *)sqlite3_column_text(getdomainsigs_stmt, 0);
            sigs.insert(string(hash));
        }
        if (rc == SQLITE_DONE)
            result = true;
        sqlite3_reset(getdomainsigs_stmt);
        sqlite3_clear_bindings(getdomainsigs_stmt);
        return result;
    }

    bool JSEngine::InsertDomain(const string domainName, const time_t timestamp, int & domainid)
    {
        bool result = false;

        if (sqlite3_exec(db, "BEGIN", 0, 0, 0) != SQLITE_OK)
        {
            domainid = -1;
            return result;
        }

        if ((sqlite3_bind_text(insertdomain_stmt, 1, domainName.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
            || (sqlite3_bind_int64(insertdomain_stmt, 2, (sqlite3_int64)timestamp) != SQLITE_OK))
            return result;

        int rc = 0;
        if ((rc = sqlite3_step(insertdomain_stmt)) == SQLITE_DONE)
        {
            result = true;
            domainid = (int)sqlite3_last_insert_rowid(db);
        }
        sqlite3_reset(insertdomain_stmt);
        sqlite3_clear_bindings(insertdomain_stmt);
        sqlite3_exec(db, "COMMIT", 0, 0, 0);

        return result;
    }

    bool JSEngine::InsertDomainSignature(const string domainName, const string sig)
    {
        bool result = false;

        int domainID = 0;
        time_t timestamp;
        if (GetDomainTimestamp(domainName, domainID, timestamp))
            result = InsertDomainSignature(domainID, sig);

        return result;
    }

    bool JSEngine::InsertDomainSignature(int domainID, const string sig)
    {
        bool result = true;

        if ((sqlite3_bind_int(insertdomainsig_stmt, 1, domainID) != SQLITE_OK) ||
            (sqlite3_bind_text(insertdomainsig_stmt, 2, sig.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK))
        {
            result = false;
        }
        else if (sqlite3_step(insertdomainsig_stmt) != SQLITE_DONE)
        {
            result = false;
        }
        sqlite3_reset(insertdomainsig_stmt);
        sqlite3_clear_bindings(insertdomainsig_stmt);
        return result;
    }

    bool JSEngine::InsertDomainSignatures(const string domainName, set<string> &sigs)
    {
        bool result = false;

        int domainID = 0;
        time_t timestamp;
        if (GetDomainTimestamp(domainName, domainID, timestamp))
            result = InsertDomainSignatures(domainID, sigs);

        return result;
    }

    bool JSEngine::InsertDomainSignatures(int domainID, set<string> &sigs)
    {
        bool result = true;
        int err;

        for (set<string>::iterator it = sigs.begin(); it != sigs.end(); it++)
        {
            if ((sqlite3_bind_int(insertdomainsig_stmt, 1, domainID) != SQLITE_OK) ||
                (sqlite3_bind_text(insertdomainsig_stmt, 2, (*it).c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK))
            {
                result = false;
            }
            else if ((err = sqlite3_step(insertdomainsig_stmt)) != SQLITE_DONE)
            {
                //cerr << "Error inserting signature:" << (*it).c_str() << " for domain:" << domainID << " sqlite error:" << err << endl;
                result = false;
            }
            sqlite3_reset(insertdomainsig_stmt);
            sqlite3_clear_bindings(insertdomainsig_stmt);
        }

        return result;
    }

    bool JSEngine::InsertSignatureUrls(const string sig, set<string> &urls)
    {
        bool result = true;
        int err;

        for (set<string>::iterator it = urls.begin(); it != urls.end(); it++)
        {
            if ((sqlite3_bind_text(insertsigurl_stmt, 1, sig.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) ||
                (sqlite3_bind_text(insertsigurl_stmt, 2, (*it).c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK))
            {
                result = false;
            }
            else if ((err = sqlite3_step(insertsigurl_stmt)) != SQLITE_DONE)
            {
                //cerr << "Error inserting signature url:" << (*it).c_str() << " for signature:" << sig << " sqlite error:" << err << endl;
                result = false;
            }
            sqlite3_reset(insertsigurl_stmt);
            sqlite3_clear_bindings(insertsigurl_stmt);
        }

        return result;
    }

    bool JSEngine::InsertSignatureUrl(const string sig, const string url)
    {
        bool result = true;
        int err;

        if ((sqlite3_bind_text(insertsigurl_stmt, 1, sig.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) ||
            (sqlite3_bind_text(insertsigurl_stmt, 2, url.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK))
        {
            result = false;
        }
        else if ((err = sqlite3_step(insertsigurl_stmt)) != SQLITE_DONE)
        {
            //cerr << "Error inserting signature url:" << url << " for signature:" << sig << " sqlite error:" << err << endl;
            result = false;
        }
        sqlite3_reset(insertsigurl_stmt);
        sqlite3_clear_bindings(insertsigurl_stmt);

        return result;
    }

    bool JSEngine::GetSignatureUrls(const string sig, set<string> &urls)
    {
        bool result = true;
        if (sqlite3_bind_text(getsigurls_stmt, 1, sig.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
            return result;

        int rc = 0;
        while ((rc = sqlite3_step(getsigurls_stmt)) == SQLITE_ROW)
        {
            char *url = (char *)sqlite3_column_text(getsigurls_stmt, 0);
            urls.insert(string(url));
        }
        if (rc == SQLITE_DONE)
            result = true;
        sqlite3_reset(getsigurls_stmt);
        sqlite3_clear_bindings(getsigurls_stmt);
        return result;
    }

    bool JSEngine::DeleteDomain(const string domainName)
    {
        bool result = false;
        if (sqlite3_bind_text(deletedomain_stmt, 1, domainName.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
            return result;

        if (sqlite3_step(deletedomain_stmt) == SQLITE_DONE)
        {
            result = true;
        }
        sqlite3_reset(deletedomain_stmt);
        sqlite3_clear_bindings(deletedomain_stmt);
        return result;
    }

    /* mock-up: reads signature files from disk instead of fetching it over https (filename must be equal to the domain name) */
    bool JSEngine::RetrieveDomainSignatures(const string domainName, pair<time_t, set<string> > & sigs, map<string, set<string>> & urls)
    {
        bool result = false;

        string filename = "cache_";
        filename += domainName;
        filename += ".txt";

        ifstream fsigs(filename.c_str());
        if (fsigs.fail())
            return result;

        int64_t ts;
        fsigs >> ts;
        sigs.first = (time_t)ts;

        string sig;
        string input;

        while (!fsigs.eof())
        {
            fsigs >> input;
            if (!input.empty())
            {
                if (input.at(0) == '>')
                {
                    //it's a url
                    string urlsig = input.substr(1);

                    if (urls.find(sig) == urls.end())
                    {
                        urls.insert(std::pair<string, set<string>>(sig, set<string>()));
                    }
                    urls[sig].insert(urlsig);
                }
                else
                {
                    sig = string(input);
                    sigs.second.insert(sig);
                }
            }

            if (fsigs.eof())
                break;
        }

        fsigs.close();
        result = true;

        return result;
    }

    bool JSEngine::AddBenchResult(const string scripthash, const string type, JSStopWatch *sw)
    {
        if (scripthash.length() < 40)
            return false;

        if (!IsSettingEnabled(JSEngineSettings::Benchmark))
            return true;

        bool result = false;
        int err = 0;
        //Fields: (hash, sig, db, stack, script, args, type)   

        if (
            (sqlite3_bind_text(insertbench_stmt, 1, scripthash.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) ||
            (sqlite3_bind_int(insertbench_stmt, 2, sw->SigTime()) != SQLITE_OK) ||
            (sqlite3_bind_int(insertbench_stmt, 3, sw->DbTime()) != SQLITE_OK) ||
            (sqlite3_bind_int(insertbench_stmt, 4, sw->StackTraceTime()) != SQLITE_OK) ||
            (sqlite3_bind_int(insertbench_stmt, 5, sw->ScriptExecTime()) != SQLITE_OK) ||
            (sqlite3_bind_int(insertbench_stmt, 6, sw->ArgTime()) != SQLITE_OK) ||
            (sqlite3_bind_text(insertbench_stmt, 7, type.c_str(), type.length(), SQLITE_TRANSIENT) != SQLITE_OK)
            )
        {
            Log("Error preparing measurement stmt");
            result = false;
        }
        else if ((err = sqlite3_step(insertbench_stmt)) != SQLITE_DONE)
        {
            Log(fmt::format("Error inserting measurement for sig: {0} : {1}", scripthash, err));
            result = false;
        }
        sqlite3_reset(insertbench_stmt);
        sqlite3_clear_bindings(insertbench_stmt);
        //sw->Reset();
        return result;
    }

}