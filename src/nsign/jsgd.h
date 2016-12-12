/* 
 * Copyright 2015-2015 Konstantinos Stroggylos, Dimitris Mitropoulos
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 */

/* vim: set ts=4 sw=4 et tw=79 expandtab:
* jsgd.h
*
* A templated destructor for Singletons.
*/

#ifndef JSGD_H_
#define JSGD_H_

namespace nSign {

    template <class T>
    class GenericDestructor
    {
    public:
        GenericDestructor(T* = 0);
        ~GenericDestructor();

        void MarkForDestruction(T*);
    private:

        // Prevent users from making copies of a 
        // GenericDestructor to avoid double deletion:
        GenericDestructor(const GenericDestructor<T>&);
        void operator=(const GenericDestructor<T>&);
    private:
        T* obj;
    };

    template <class T>
    GenericDestructor<T>::GenericDestructor(T* t)
    {
        obj = t;
    }

    template <class T>
    GenericDestructor<T>::~GenericDestructor()
    {
        delete obj;
    }

    template <class T>
    void GenericDestructor<T>::MarkForDestruction(T* t)
    {
        obj = t;
    }

}

#endif