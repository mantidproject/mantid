// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidSINQ/LoadFlexiNexus.h"
#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid;

class LoadFlexiNexusTest : public CxxTest::TestSuite {
public:
  void testName() {
    LoadFlexiNexus loader;
    TS_ASSERT_EQUALS(loader.name(), "LoadFlexiNexus");
  }

  void testInit() {
    LoadFlexiNexus loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  void testExec3D() {
    LoadFlexiNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "amor2013n000366.hdf");
    loader.setPropertyValue("Dictionary", "mantidamor.dic");
    std::string outputSpace = "LoadFlexiNexusTest_out";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    // test data
    IMDHistoWorkspace_sptr data = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(outputSpace);
    long nBin = static_cast<long>(data->getNPoints());
    long sum = 0;
    auto sdata = data->getSignalArray();
    for (long i = 0; i < nBin; i++) {
      sum += (long)sdata[i];
    }
    TS_ASSERT_EQUALS(sum, 18816);

    // test dimensions
    std::shared_ptr<const IMDDimension> dimi = data->getDimension(0);
    TS_ASSERT_EQUALS(dimi->getNBins(), 360);
    TS_ASSERT_DELTA(dimi->getMinimum(), 32471.4, .1);
    TS_ASSERT_DELTA(dimi->getMaximum(), 194590.43, .1);

    dimi = data->getDimension(1);
    TS_ASSERT_EQUALS(dimi->getNBins(), 256);
    TS_ASSERT_DELTA(dimi->getMinimum(), -95, .1);
    TS_ASSERT_DELTA(dimi->getMaximum(), 94.25, .1);

    dimi = data->getDimension(2);
    TS_ASSERT_EQUALS(dimi->getNBins(), 128);
    TS_ASSERT_DELTA(dimi->getMinimum(), -86, .1);
    TS_ASSERT_DELTA(dimi->getMaximum(), 84.65, .1);

    // test some meta data
    std::string title = data->getTitle();
    size_t found = title.find("Selene");
    TS_ASSERT_DIFFERS(found, std::string::npos);

    ExperimentInfo_sptr info;
    info = data->getExperimentInfo(0);
    const Run r = info->run();
    Mantid::Kernel::Property *p = r.getProperty("chopper_detector_distance");
    std::string cd = p->value();
    found = cd.find("6423");
    TS_ASSERT_DIFFERS(found, std::string::npos);

    AnalysisDataService::Instance().clear();
  }

  void testExec1D() {
    LoadFlexiNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "amor2013n000366.hdf");
    loader.setPropertyValue("Dictionary", "mantidamors1.dic");
    std::string outputSpace = "LoadFlexiNexusTest_out";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    // test data
    MatrixWorkspace_sptr data = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
    TS_ASSERT_EQUALS(data->getNumberHistograms(), 1);
    const auto &X = data->x(0);
    const auto &Y = data->y(0);
    double dSum = std::accumulate(Y.cbegin(), Y.cend(), 0.);
    TS_ASSERT_EQUALS(dSum, 198812);

    // test X
    TS_ASSERT_EQUALS(X.size(), 360);
    TS_ASSERT_DELTA(X.front(), 32471.4, .1);
    TS_ASSERT_DELTA(X.back(), 194590.43, .1);

    // test some meta data
    std::string title = data->getTitle();
    size_t found = title.find("Selene");
    TS_ASSERT_DIFFERS(found, std::string::npos);

    const Run r = data->run();
    Mantid::Kernel::Property *p = r.getProperty("chopper_detector_distance");
    std::string cd = p->value();
    found = cd.find("6423");
    TS_ASSERT_DIFFERS(found, std::string::npos);

    AnalysisDataService::Instance().clear();
  }
};
