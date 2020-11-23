// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CropWorkspaceRagged.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class CropWorkspaceRaggedTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CropWorkspaceRaggedTest *createSuite() { return new CropWorkspaceRaggedTest(); }
  static void destroySuite(CropWorkspaceRaggedTest *suite) { delete suite; }

 Workspace_sptr createInputWorkspace() {
    std::string name = "toCrop";
     // Set up a small workspace for testing
     Workspace_sptr space =
          WorkspaceFactory::Instance().create("Workspace2D", m_numberOfSpectra, m_numberOfYPoints+1, m_numberOfYPoints);
      Workspace2D_sptr space2D = std::dynamic_pointer_cast<Workspace2D>(space);
      std::vector<double> ydata(m_numberOfYPoints*m_numberOfSpectra), errors(m_numberOfYPoints*m_numberOfSpectra);
      for (int j = 0; j < m_numberOfSpectra; ++j) {
        for (int k = 0; k < m_numberOfYPoints+1; ++k) {
          ydata[k+j*m_numberOfYPoints] = k+1;
          errors[k+j*m_numberOfYPoints] = sqrt(double(k+1));
          space2D->dataX(j)[k] = k;
        }
        auto beginY = std::next(ydata.begin(), j * m_numberOfYPoints);
        space2D->dataY(j) = std::vector<double>(beginY, std::next(beginY, m_numberOfYPoints));
        auto beginE = std::next(errors.begin(), j * m_numberOfYPoints);
        space2D->dataE(j) = std::vector<double>(beginE, std::next(beginE, m_numberOfYPoints));
      }
    return space2D;
  }
  void setUp() override{
    m_numberOfYPoints = 15;
    m_numberOfSpectra = 5;
    auto a = createInputWorkspace();
  }
  void test_Name() { TS_ASSERT_EQUALS(m_alg.name(), "CropWorkspaceRagged"); }

  void test_Version() { TS_ASSERT_EQUALS(m_alg.version(), 1); }

  void test_Init() {
    TS_ASSERT_THROWS_NOTHING(m_alg.initialize());
    TS_ASSERT(m_alg.isInitialized());
  }

  void test_InvalidInputs() {
	  return;
    std::string inputName = createInputWorkspace();
    if (!m_alg.isInitialized())
      m_alg.initialize();

    TS_ASSERT_THROWS(m_alg.execute(), const std::runtime_error &);
    TS_ASSERT(!m_alg.isExecuted());
    TS_ASSERT_THROWS_NOTHING(
        m_alg.setPropertyValue("InputWorkspace", inputName));
    TS_ASSERT_THROWS_NOTHING(
        m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "2"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "1"));
    TS_ASSERT_THROWS_NOTHING(m_alg.execute());
    TS_ASSERT(m_alg.isExecuted());
  }


private:
 std::string m_name;
  CropWorkspaceRagged m_alg;
  int m_numberOfSpectra;
  int m_numberOfYPoints;
};

