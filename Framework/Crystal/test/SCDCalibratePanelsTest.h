// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * SCDCalibratePanelsTest.h
 *
 *  Created on: Mar 12, 2012
 *      Author: ruth
 */

#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include <cxxtest/TestSuite.h>
#include <filesystem>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace std;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::Crystal;

class SCDCalibratePanelsTest : public CxxTest::TestSuite {

public:
  void test_TOPAZ_5637() {
    // load a peaks file
    std::shared_ptr<Algorithm> alg = AlgorithmFactory::Instance().create("LoadIsawPeaks", 1);
    alg->initialize();
    alg->setPropertyValue("Filename", "Peaks5637.integrate");
    alg->setPropertyValue("OutputWorkspace", "TOPAZ_5637");
    TS_ASSERT(alg->execute());
    PeaksWorkspace_sptr pws = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>("TOPAZ_5637");
    size_t numberPeaks = pws->getNumberPeaks();
    std::vector<int> notBank47;
    for (int i = 0; i < int(numberPeaks); i++)
      if (pws->getPeak(i).getBankName() != "bank47")
        notBank47.emplace_back(i);
    pws->removePeaks(std::move(notBank47));

    // run the calibration
    alg = AlgorithmFactory::Instance().create("SCDCalibratePanels", 1);
    alg->initialize();
    // Peakws->setName("PeaksWsp");
    alg->setPropertyValue("PeakWorkspace", "TOPAZ_5637");
    alg->setProperty("a", 4.75);
    alg->setProperty("b", 4.75);
    alg->setProperty("c", 13.0);
    alg->setProperty("alpha", 90.0);
    alg->setProperty("beta", 90.0);
    alg->setProperty("gamma", 120.0);
    auto detCalTempPath = std::filesystem::temp_directory_path();
    detCalTempPath /= "topaz.detcal";
    alg->setPropertyValue("DetCalFilename", detCalTempPath.string());
    TS_ASSERT(alg->execute());

    // verify the results
    ITableWorkspace_sptr results = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("params_bank47");
    // TODO: Some of the fit parameters that are below are extermly sensitive to
    // rounding errors in the algorithm LoadIsawPeaks and floating point math in
    // the instrument code. Ideally the assertions should be on something else.
    TS_ASSERT_DELTA(-0.0031, results->cell<double>(0, 1), 1e-3);
    TS_ASSERT_DELTA(0.0008, results->cell<double>(1, 1), 4e-4);
    TS_ASSERT_DELTA(-0.00009, results->cell<double>(2, 1), 3e-4);
    TS_ASSERT_DELTA(0.750445, results->cell<double>(3, 1) + results->cell<double>(5, 1), 0.05);
    TS_ASSERT_DELTA(-1.39339, results->cell<double>(4, 1), 1.1);
    TS_ASSERT_DELTA(0.210786, results->cell<double>(5, 1), 1.e-2);
    TS_ASSERT_DELTA(1.0024, results->cell<double>(6, 1), 5e-3);
    TS_ASSERT_DELTA(0.9986, results->cell<double>(7, 1), 1e-2);
    TS_ASSERT_DELTA(0.2710, results->cell<double>(9, 1), 0.2);
    ITableWorkspace_sptr resultsL1 = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("params_L1");
    TS_ASSERT_DELTA(0.00529, resultsL1->cell<double>(2, 1), .01);

    remove(detCalTempPath.string().c_str());
  }
};
