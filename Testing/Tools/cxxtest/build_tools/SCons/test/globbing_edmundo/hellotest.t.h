// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**
 * \file
 * The test file.
 */
/****************************************************
 * Author: Edmundo LOPEZ
 * email:  lopezed5@etu.unige.ch
 *
 * **************************************************/

#include <cxxtest/TestSuite.h>
#include <hello.hh>
      

class helloTestSuite : public CxxTest::TestSuite 
  {
    public:
    void testFoo()
      {
        Hello h;
        TS_ASSERT_EQUALS (h.foo(2,2), 4);
      }
  };
