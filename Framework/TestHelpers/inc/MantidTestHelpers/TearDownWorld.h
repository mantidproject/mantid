// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TEARDOWNWORLD_H_
#define TEARDOWNWORLD_H_

#include <cxxtest/GlobalFixture.h>

// This file defines a set of CxxTest::GlobalFixture classes that
// are used to control various aspects of the global test setUp and tearDown
// process

/**
 * Defines a CxxTest::GlobalFixture that clears the AlgorithmManager
 * when its tearDownWorld() method is called.
 */
class ClearAlgorithmManager : public CxxTest::GlobalFixture {
  bool tearDownWorld() override;
};

//-----------------------------------------------------------------------------

/**
 * Defines a CxxTest::GlobalFixture that clears the AnalysisDataService
 * when its tearDownWorld() method is called.
 */
class ClearADS : public CxxTest::GlobalFixture {
  bool tearDownWorld() override;
};

//-----------------------------------------------------------------------------

/**
 * Defines a CxxTest::GlobalFixture that clears the PropertyManagerDataService
 * when its tearDownWorld() method is called.
 */
class ClearPropertyManagerDataService : public CxxTest::GlobalFixture {
  bool tearDownWorld() override;
};

#endif // TEARDOWNWORLD_H_
