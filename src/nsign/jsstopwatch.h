/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* jsstopwatch.h
*
*/

#ifndef JSSTOPWATCH_H_
#define JSSTOPWATCH_H_

#include <iostream>
#include <string>
#include <stddef.h>
#include <cstdint>
#include <mutex>

namespace nSign {

    /* The state of the stopwatch. All values are binary flags. */
    enum class JSStopWatchState : int
    {
        Stopped = 0x00,
        Started = 0x01,
        Signature = 0x02,
        Database = 0x04,
        StackTrace = 0x08,
        Script = 0x10,
        Args = 0x20
    };

    class JSStopWatch
    {
    private:
        std::recursive_mutex sync;
        JSStopWatchState current, previous;
        int64_t start_total, start_sig, start_db, start_st, start_scr, start_args;
        int64_t end_total, end_sig, end_db, end_st, end_scr, end_args;
        int64_t totaltime, sigtime, dbtime, sttrctime, scrtime, argtime, overhead;
        static void bench(int rounds, long count);
        /* Causes the state to transition (adds the @state to the current stopwatch state).
        Returns false if the transition is invalid. */
        bool EnterState(JSStopWatchState state);
        /* Causes the state to transition (removes the @state from the current stopwatch state).
        Returns false if the transition is invalid. */
        bool ExitState(JSStopWatchState state);
        /* Returns a boolean value indicating if @state is currently set in the current stopwatch state */
        bool IsInState(JSStopWatchState state);

    public:
        JSStopWatch();
        virtual ~JSStopWatch(){}

        /* Overall timer*/
        void Start();
        void Stop();
        /* Signature extraction */
        void StartSig();
        void StopSig();
        /* Signature store/retrieve*/
        void StartDb();
        void StopDb();
        /* Stack trace production */
        void StartStackTrace();
        void StopStackTrace();
        /* Script execution */
        void StartScript();
        void StopScript();
        /* String argument parsing */
        void StartArgs();
        void StopArgs();

        std::string toString();

        void Reset();
        long TotalTime();
        long TotalOverhead();
        long SigTime();
        long DbTime();
        long StackTraceTime();
        long ScriptExecTime();
        long ArgTime();
        double OverheadRatio();
        double AdjustedOverheadRatio();
        static void Benchmark();
        static void StateTransitionTest();
    };

}

#endif /* JSSTOPWATCH_H_ */
