// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef __cxxtest__GlobalFixture_h__
#define __cxxtest__GlobalFixture_h__

#include <cxxtest/LinkedList.h>

namespace CxxTest 
{
    class GlobalFixture : public Link
    {
    public:
        virtual bool setUpWorld();
        virtual bool tearDownWorld();
        virtual bool setUp();
        virtual bool tearDown();
        
        GlobalFixture();
        ~GlobalFixture();
        
        static GlobalFixture *firstGlobalFixture();
        static GlobalFixture *lastGlobalFixture();
        GlobalFixture *nextGlobalFixture();
        GlobalFixture *prevGlobalFixture();

    private:
        static List _list;
    };
}

#endif // __cxxtest__GlobalFixture_h__

