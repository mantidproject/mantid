// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/FitPeaks.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::FitPeaks;

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Points;

class FitPeaksTest : public CxxTest::TestSuite {
private:
  std::string m_inputWorkspaceName{"FitPeaksTest_workspace"};

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitPeaksTest *createSuite() { return new FitPeaksTest(); }
  static void destroySuite(FitPeaksTest *suite) { delete suite; }

  void setUp() override {
    // Needs other algorithms and functions to be registered
    FrameworkManager::Instance();
  }

  void test_Init() {
    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());
  }
};
