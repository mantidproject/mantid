// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/CorelliPowderCalibrationLoad.h"
#include "MantidKernel/Logger.h"

#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

///
/// The base class CxxTest doc is available at
/// http://cxxtest.com/guide.html
class CorelliPowderCalibrationLoadTest : public CxxTest::TestSuite {
    public:
        void testName() {
            CorelliPowderCalibrationLoad corelliLoader;
            TS_ASSERT_EQUALS(corelliLoader.name(), "CorelliPowderCalibrationLoad");
        }

        void testInit() {
            CorelliPowderCalibrationLoad corelliLoader;
            corelliLoader.initialize();
            TS_ASSERT(corelliLoader.isInitialized());
        }

        //TODO: more unit test to come
};