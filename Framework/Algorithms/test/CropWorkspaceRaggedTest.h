// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/ConvertToPointData.h"
#include "MantidAlgorithms/CropWorkspaceRagged.h"
#include "MantidDataObjects/Workspace2D.h"
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

  void createInputWorkspace() {
    std::string name = "toCrop";
    // Set up a small workspace for testing
    Workspace_sptr space =
        WorkspaceFactory::Instance().create("Workspace2D", m_numberOfSpectra, m_numberOfYPoints + 1, m_numberOfYPoints);
    m_ws = std::dynamic_pointer_cast<Workspace2D>(space);
    std::vector<double> ydata, errors;
    for (int j = 0; j < m_numberOfSpectra; ++j) {
      ydata.clear();
      errors.clear();
      for (int k = 0; k < m_numberOfYPoints; ++k) {
        ydata.push_back(double(k + 1));
        errors.push_back(sqrt(double(k + 1)));
        m_ws->dataX(j)[k] = k;
      }
      m_ws->dataY(j) = ydata;
      m_ws->dataE(j) = errors;
    }
  }

  void setUp() override {
    m_numberOfYPoints = 15;
    m_numberOfSpectra = 5;
    createInputWorkspace();
  }
  void test_Name() { TS_ASSERT_EQUALS(m_alg.name(), "CropWorkspaceRagged"); }

  void test_Version() { TS_ASSERT_EQUALS(m_alg.version(), 1); }

  void test_Init() {
    return;
    TS_ASSERT_THROWS_NOTHING(m_alg.initialize());
    TS_ASSERT(m_alg.isInitialized());
  }

  void test_NoInputs() {
    if (!m_alg.isInitialized())
      m_alg.initialize();

    TS_ASSERT_THROWS(m_alg.execute(), const std::runtime_error &);
  }
  void test_XMinLarger() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "5."));

    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "10."));
    TS_ASSERT_THROWS(m_alg.execute(), const std::runtime_error &);
  }
  void test_XMinListBug() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "10"));

    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "1.,2.,3.,20.,5."));
    TS_ASSERT_THROWS(m_alg.execute(), const std::runtime_error &);
  }
  void test_XMaxListBug() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "1."));

    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "10.,20.,30.,0.4,50."));
    TS_ASSERT_THROWS(m_alg.execute(), const std::runtime_error &);
  }
  void test_ListsBug() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "1.,2.,3.,20.,5."));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "10.,20.,30.,0.4,50."));
    TS_ASSERT_THROWS(m_alg.execute(), const std::runtime_error &);
  }
  void test_TooFewXMins() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "5."));

    TS_ASSERT_THROWS(m_alg.setPropertyValue("XMin", ""), const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "1,2"));
    TS_ASSERT_THROWS(m_alg.execute(), const std::runtime_error &);
  }

  void test_TooFewXMaxs() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "1."));

    TS_ASSERT_THROWS(m_alg.setPropertyValue("XMax", ""), const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "11,12"));
    TS_ASSERT_THROWS(m_alg.execute(), const std::runtime_error &);
  }

  void test_TooManyMins() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "11."));

    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "1,2,3,4,5,6"));
    TS_ASSERT_THROWS(m_alg.execute(), const std::runtime_error &);
  }

  void test_TooManyXMaxs() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "1."));

    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "11,12,13,14,15,16"));
    TS_ASSERT_THROWS(m_alg.execute(), const std::runtime_error &);
  }

  void test_SingleValueCrop() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "2."));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "11"));
    TS_ASSERT_THROWS_NOTHING(m_alg.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("nothing");
    for (int spec = 0; spec < m_numberOfSpectra; spec++) {
      TS_ASSERT_DELTA(out->readX(spec)[0], 2.0, 1e-6);
      TS_ASSERT_DELTA(out->readY(spec)[0], 3.0, 1e-6);
      TS_ASSERT_DELTA(out->readE(spec)[0], 1.732051, 1e-6); // sqrt(3)

      TS_ASSERT_DELTA(out->readX(spec).back(), 11.0, 1e-6);
      TS_ASSERT_DELTA(out->readY(spec).back(), 11.0, 1e-6);
      TS_ASSERT_DELTA(out->readE(spec).back(), 3.316625, 1e-6); // sqrt(11)
    }
  }

  void test_MinListCrop() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    std::vector<double> xMin{2., 5., 6., 7., 1.};

    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "2., 5., 6., 7., 1."));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "11"));
    TS_ASSERT_THROWS_NOTHING(m_alg.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("nothing");
    for (int spec = 0; spec < m_numberOfSpectra; spec++) {
      TS_ASSERT_DELTA(out->readX(spec)[0], xMin[spec], 1e-6);
      TS_ASSERT_DELTA(out->readY(spec)[0], xMin[spec] + 1., 1e-6);
      TS_ASSERT_DELTA(out->readE(spec)[0], sqrt(xMin[spec] + 1.), 1e-6);

      TS_ASSERT_DELTA(out->readX(spec).back(), 11.0, 1e-6);
      TS_ASSERT_DELTA(out->readY(spec).back(), 11.0, 1e-6);
      TS_ASSERT_DELTA(out->readE(spec).back(), 3.316625, 1e-6); // sqrt(11)
    }
  }
  void test_MaxListCrop() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    std::vector<double> xMax{12., 13., 11., 8., 9.};

    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "2."));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "12, 13, 11, 8, 9"));
    TS_ASSERT_THROWS_NOTHING(m_alg.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("nothing");
    for (int spec = 0; spec < m_numberOfSpectra; spec++) {
      TS_ASSERT_DELTA(out->readX(spec)[0], 2.0, 1e-6);
      TS_ASSERT_DELTA(out->readY(spec)[0], 3.0, 1e-6);
      TS_ASSERT_DELTA(out->readE(spec)[0], sqrt(3.), 1e-6);

      TS_ASSERT_DELTA(out->readX(spec).back(), xMax[spec], 1e-6);
      TS_ASSERT_DELTA(out->readY(spec).back(), xMax[spec], 1e-6);
      TS_ASSERT_DELTA(out->readE(spec).back(), sqrt(xMax[spec]), 1e-6);
    }
  }

  void test_preservesHist() {
    TS_ASSERT(m_ws->isHistogramData());
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "2."));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "11"));
    TS_ASSERT_THROWS_NOTHING(m_alg.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("nothing");
    TS_ASSERT(out->isHistogramData());
  }

  void test_preservesPoints() {
    ConvertToPointData alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", m_ws);
    alg.setProperty("OutputWorkspace", "points");
    alg.execute();
    MatrixWorkspace_sptr points = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("points");

    TS_ASSERT_EQUALS(points->isHistogramData(), false);
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", points));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "2."));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "11"));
    TS_ASSERT_THROWS_NOTHING(m_alg.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("nothing");
    TS_ASSERT_EQUALS(out->isHistogramData(), false);
  }

  void test_xMaxMoreThanData() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "2."));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "111"));
    TS_ASSERT_THROWS_NOTHING(m_alg.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("nothing");

    for (int spec = 0; spec < m_numberOfSpectra; spec++) {
      TS_ASSERT_DELTA(out->readX(spec)[0], 2.0, 1e-6);
      TS_ASSERT_DELTA(out->readY(spec)[0], 3.0, 1e-6);
      TS_ASSERT_DELTA(out->readE(spec)[0], 1.732051, 1e-6); // sqrt(3)

      TS_ASSERT_DELTA(out->readX(spec).back(), m_ws->readX(spec).back(), 1e-6);
      TS_ASSERT_DELTA(out->readY(spec).back(), m_ws->readY(spec).back(), 1e-6);
      TS_ASSERT_DELTA(out->readE(spec).back(), m_ws->readE(spec).back(), 1e-6);
    }
  }

  void test_xMinLessThanData() {
    TS_ASSERT_THROWS_NOTHING(m_alg.setProperty("InputWorkspace", m_ws));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMin", "-2."));
    TS_ASSERT_THROWS_NOTHING(m_alg.setPropertyValue("XMax", "11"));
    TS_ASSERT_THROWS_NOTHING(m_alg.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("nothing");
    for (int spec = 0; spec < m_numberOfSpectra; spec++) {
      TS_ASSERT_DELTA(out->readX(spec)[0], m_ws->readX(spec)[0], 1e-6);
      TS_ASSERT_DELTA(out->readY(spec)[0], m_ws->readY(spec)[0], 1e-6);
      TS_ASSERT_DELTA(out->readE(spec)[0], m_ws->readE(spec)[0], 1e-6);

      TS_ASSERT_DELTA(out->readX(spec).back(), 11.0, 1e-6);
      TS_ASSERT_DELTA(out->readY(spec).back(), 11.0, 1e-6);
      TS_ASSERT_DELTA(out->readE(spec).back(), 3.316625, 1e-6); // sqrt(11)
    }
  }

private:
  Workspace2D_sptr m_ws;
  CropWorkspaceRagged m_alg;
  int m_numberOfSpectra;
  int m_numberOfYPoints;
};
