/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* jsstopwatch.cpp
*
*/

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <cstdio>
#include <cstddef>
#include <cstdint>

#include "jsstopwatch.h"
#include "jsenums.h"
#include "prmjtime.h"

namespace nSign {

    JSStopWatch::JSStopWatch()
    {
        Reset();
    }

    bool JSStopWatch::EnterState(JSStopWatchState state)
    {
        std::lock_guard<std::recursive_mutex> lock(sync);

        if (IsInState(state))
        {
            std::cerr << "JSStopwatch: Attempted to enter state:" << enum_value(state) << " while currently in: " << enum_value(current) << std::endl;
            return false;
        }

        previous = current;
        current = (JSStopWatchState)(enum_value(current) | enum_value(state));
        //std::cerr << "JSStopwatch: Entered state:" << enum_value(state) << ", prev: " << enum_value(previous) << ", cur: " << enum_value(current) << std::endl;
        
        /*for (auto &flag : enum_flags(JSStopWatchState::Started, JSStopWatchState::Args))
        {
            if (enum_value_contains_flag(state, flag))
            {
                switch (flag)
                {
                default:
                    break;
                }
            }
        }*/
        
        return true;
    }

    bool JSStopWatch::ExitState(JSStopWatchState state)
    {
        std::lock_guard<std::recursive_mutex> lock(sync);

        if (!IsInState(state))
        {
            std::cerr << "JSStopwatch: Attempted to exit state:" << enum_value(state) << " while currently in: " << enum_value(current) << std::endl;
            return false;
        }

        previous = current;
        current = (JSStopWatchState)(enum_value(current) & ~enum_value(state));
        //std::cerr << "JSStopwatch: Exited state:" << enum_value(state) << ", prev: " << enum_value(previous) << ", cur: " << enum_value(current) << std::endl;
        return true;
    }
        
    bool JSStopWatch::IsInState(JSStopWatchState state)
    {
        return (enum_value(current) & enum_value(state)) == enum_value(state);
    }

    void JSStopWatch::Start()
    {
        if (!EnterState(JSStopWatchState::Started))
        {
            return;
        }
        start_total = PRMJ_Now();
    }

    void JSStopWatch::StartSig()
    {
        if (!EnterState(JSStopWatchState::Signature))
        {
            return;
        }
        start_sig = PRMJ_Now();
    }

    void JSStopWatch::StopSig()
    {
        if (!ExitState(JSStopWatchState::Signature))
        {
            return;
        }
        end_sig = PRMJ_Now();
        overhead += (end_sig - start_sig);
        sigtime += (end_sig - start_sig);
        start_sig = 0;
        end_sig = 0;
    }
    void JSStopWatch::StartDb()
    {
        if (!EnterState(JSStopWatchState::Database))
        {
            return;
        }
        start_db = PRMJ_Now();
    }

    void JSStopWatch::StopDb()
    {
        if (!ExitState(JSStopWatchState::Database))
        {
            return;
        }
        end_db = PRMJ_Now();
        overhead += (end_db - start_db);
        dbtime += (end_db - start_db);
        start_db = 0;
        end_db = 0;
    }

    void JSStopWatch::StartStackTrace()
    {
        if (!EnterState(JSStopWatchState::StackTrace))
        {
            return;
        }
        start_st = PRMJ_Now();
    }

    void JSStopWatch::StopStackTrace()
    {
        if (!ExitState(JSStopWatchState::StackTrace))
        {
            return;
        }
        end_st = PRMJ_Now();
        overhead += (end_st - start_st);
        sttrctime += (end_st - start_st);
        start_st = 0;
        end_st = 0;
    }

    void JSStopWatch::StartScript()
    {
        if (!EnterState(JSStopWatchState::Script))
        {
            return;
        }
        start_scr = PRMJ_Now();
    }

    void JSStopWatch::StopScript()
    {
        if (!ExitState(JSStopWatchState::Script))
        {
            return;
        }
        end_scr = PRMJ_Now();
        scrtime += (end_scr - start_scr);
        start_scr = 0;
        end_scr = 0;
    }

    void JSStopWatch::StartArgs()
    {
        if (!EnterState(JSStopWatchState::Args))
        {
            return;
        }
        start_args = PRMJ_Now();
    }

    void JSStopWatch::StopArgs()
    {
        if (!ExitState(JSStopWatchState::Args))
        {
            return;
        }
        end_args = PRMJ_Now();
        overhead += (end_args - start_args);
        argtime += (end_args - start_args);
        start_args = 0;
        end_args = 0;
    }

    void JSStopWatch::Stop()
    {
        if (!ExitState(JSStopWatchState::Started))
        {
            return;
        }
        end_total = PRMJ_Now();
        totaltime += end_total - start_total;
        end_total = 0;
        start_total = 0;
    }

    void JSStopWatch::Reset()
    {
        current = previous = JSStopWatchState::Stopped;
        start_total = start_sig = start_db = start_scr = start_st = start_args = 0;
        end_total = end_sig = end_db = end_scr = end_st = end_args = 0;
        totaltime = overhead = sigtime = dbtime = sttrctime = scrtime = argtime = 0;
    }

    long JSStopWatch::TotalTime()
    {
        return (long)totaltime;
    }

    long JSStopWatch::TotalOverhead()
    {
        return (long)overhead;
    }

    double JSStopWatch::OverheadRatio()
    {
        if (scrtime > 0)
            return (double)(((double)overhead / scrtime) * 100.0);
        return 0.0;
    }

    double JSStopWatch::AdjustedOverheadRatio()
    {
        if (totaltime > 0 && scrtime > 0)
            return (double)((double)overhead / totaltime) * 100.0;
        return 0.0;
    }

    long JSStopWatch::SigTime()
    {
        return (long)sigtime;
    }

    long JSStopWatch::DbTime()
    {
        return (long)dbtime;
    }

    long JSStopWatch::StackTraceTime()
    {
        return (long)sttrctime;
    }

    long JSStopWatch::ScriptExecTime()
    {
        return (long)scrtime;
    }

    long JSStopWatch::ArgTime()
    {
        return (long)argtime;
    }

    std::string JSStopWatch::toString()
    {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss.precision(3);
        ss << "Total: " << totaltime << ", Overhead: " << overhead;
        ss << " [sig=" << sigtime << ", db=" << dbtime << ", stacktrace=" << sttrctime << ", args=" << argtime << "], ";
        ss << "Script: " << scrtime << ", (" << std::fixed << OverheadRatio() << "%),(" << std::fixed << AdjustedOverheadRatio() << "%)";
        return ss.str();
    }

    void JSStopWatch::Benchmark()
    {
        JSStopWatch sw;
        long count = 1000000;
        sw.Start();
        sw.StartDb();
        bench(500, count);
        sw.StopDb();
        sw.StartSig();
        bench(500, count);
        sw.StopSig();
        bench(1000, count); //no-op
        sw.StartScript();
        bench(1000, count);
        sw.StopScript();
        sw.StartArgs();
        bench(1000, count);
        sw.StopArgs();
        sw.StartStackTrace();
        bench(1000, count);
        sw.StopStackTrace();
        sw.StartDb();
        bench(500, count);
        sw.StopDb();
        sw.StartSig();
        bench(500, count);
        sw.StopSig();
        sw.Stop();
        std::cerr << "JSStopWatch Benchmark:" << sw.toString() << std::endl;
    }

    void JSStopWatch::bench(int rounds, long count)
    {
        for (; rounds > 0; rounds--)
        {
            for (; count > 0;)
                count--;
        }
    }

    void JSStopWatch::StateTransitionTest()
    {
        JSStopWatch sw;
        sw.EnterState(JSStopWatchState::Started); //Started
        sw.EnterState(JSStopWatchState::Database); //Started | Database
        sw.EnterState(JSStopWatchState::Script); // Started | Database | Script
        sw.ExitState(JSStopWatchState::Database); //Started | Script
        sw.ExitState(JSStopWatchState::Script); // Started
        sw.ExitState(JSStopWatchState::Started); //Stopped
        std::cerr << sw.toString() << std::endl;
    }
}