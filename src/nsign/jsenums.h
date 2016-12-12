/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* enums.h
*/

#ifndef JSENUMS_H_
#define JSENUMS_H_

#include <type_traits>
#include <vector>
/*
//use this when constexpr is available

template <typename T>
constexpr typename std::underlying_type<T>::type enum_value(T val)
{
    return static_cast<typename std::underlying_type<T>::type>(val);
}
*/

template<typename T> 
auto enum_value(T t) -> typename std::underlying_type<T>::type
{
    return static_cast<typename std::underlying_type<T>::type>(t);
}

template<typename T> 
bool enum_value_is_single_flag(T t)
{
    typename std::underlying_type<T>::type value = enum_value(t);
    return value > 0 && !(value & (value ? 1));
}

template<typename T>
bool enum_value_contains_flag(T t, T flag)
{
    typename std::underlying_type<T>::type value = enum_value(t), flagvalue = enum_value(flag);
    return (value & flagvalue) == flagvalue;
}

template<typename T>
std::vector<typename std::underlying_type<T>::type> enum_flag_values(T min, T max)
{
    typename std::underlying_type<T>::type minvalue = enum_value(min), maxvalue = enum_value(max), curvalue = minvalue;
    std::vector<typename std::underlying_type<T>::type> result;
    while (curvalue <= maxvalue)
    {
        result.push_back(curvalue);
        curvalue *= 2;
    }

    return result;
}

template<typename T>
std::vector<T> enum_flags(T min, T max)
{
    typename std::underlying_type<T>::type minvalue = enum_value(min), maxvalue = enum_value(max), curvalue = minvalue;
    std::vector<T> result;
    while (curvalue <= maxvalue)
    {
        result.push_back((typename T)curvalue);
        curvalue *= 2;
    }

    return result;
}

#endif