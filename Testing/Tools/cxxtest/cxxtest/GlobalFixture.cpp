// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef __cxxtest__GlobalFixture_cpp__
#define __cxxtest__GlobalFixture_cpp__

#include <cxxtest/GlobalFixture.h>

namespace CxxTest 
{
    bool GlobalFixture::setUpWorld() { return true; }
    bool GlobalFixture::tearDownWorld() { return true; }
    bool GlobalFixture::setUp() { return true; }
    bool GlobalFixture::tearDown() { return true; }
        
    GlobalFixture::GlobalFixture() { attach( _list ); }
    GlobalFixture::~GlobalFixture() { detach( _list ); }
        
    GlobalFixture *GlobalFixture::firstGlobalFixture() { return (GlobalFixture *)_list.head(); }
    GlobalFixture *GlobalFixture::lastGlobalFixture() { return (GlobalFixture *)_list.tail(); }
    GlobalFixture *GlobalFixture::nextGlobalFixture() { return (GlobalFixture *)next(); }
    GlobalFixture *GlobalFixture::prevGlobalFixture() { return (GlobalFixture *)prev(); }
}

#endif // __cxxtest__GlobalFixture_cpp__

