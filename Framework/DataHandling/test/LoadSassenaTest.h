// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadSassena.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;

class LoadSassenaTest : public CxxTest::TestSuite {
public:
  static LoadSassenaTest *createSuite() { return new LoadSassenaTest(); }
  static void destroySuite(LoadSassenaTest *suite) { delete suite; }

  LoadSassenaTest() { m_inputFile = "outputSassena_1.4.1.h5"; }

  void test_init() {
    TS_ASSERT_THROWS_NOTHING(m_alg.initialize())
    TS_ASSERT(m_alg.isInitialized())
  }

  void test_confidence() {
    if (!m_alg.isInitialized())
      m_alg.initialize();
    m_alg.setPropertyValue("Filename", m_inputFile);
    Mantid::Kernel::NexusDescriptor descr(m_alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(m_alg.confidence(descr), 99);
  }

  void test_exec() {
    std::string result;
    if (!m_alg.isInitialized())
      m_alg.initialize();

    m_alg.setPropertyValue("Filename", m_inputFile);
    m_alg.setProperty("SortByQVectors", true);

    const std::string outSpace = "outGWS";
    m_alg.setPropertyValue("OutputWorkSpace", outSpace);
    TS_ASSERT_THROWS_NOTHING(result = m_alg.getPropertyValue("OutputWorkspace"))
    TS_ASSERT(result == outSpace);

    TS_ASSERT_THROWS_NOTHING(m_alg.execute());
    TS_ASSERT(m_alg.isExecuted());

    // Test the last stored value for each of the workspaces
    API::WorkspaceGroup_sptr gws = API::AnalysisDataService::Instance().retrieveWS<API::WorkspaceGroup>("outGWS");

    // Test qvectors
    DataObjects::Workspace2D_sptr ws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(gws->getItem("outGWS_qvectors"));
    TS_ASSERT_DELTA(ws->y(2)[0], 0.012, 1e-03);

    // Test fq
    ws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(gws->getItem("outGWS_fq"));
    TS_ASSERT_DELTA(ws->y(0)[4], 1070.7009, 1e-04);
    TS_ASSERT_DELTA(ws->y(1)[4], 674.67703, 1e-05);

    // Test fq0
    ws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(gws->getItem("outGWS_fq0"));
    TS_ASSERT_DELTA(ws->y(0)[4], 1094.1314, 1e-04);
    TS_ASSERT_DELTA(ws->y(1)[4], 652.75902, 1e-05);

    // Test fq2
    ws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(gws->getItem("outGWS_fq2"));
    TS_ASSERT_DELTA(ws->y(0)[4], 358926.16, 1e-02);
    TS_ASSERT_EQUALS(ws->y(1)[4], 0.0);

    // Test fq Real part
    ws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(gws->getItem("outGWS_fqt.Re"));
    TS_ASSERT_DELTA(ws->y(4)[0], 1918.2156, 1e-04);
    TS_ASSERT_DELTA(ws->y(4)[14], 1918.2156, 1e-04);

    // Test fq Imaginary part
    ws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(gws->getItem("outGWS_fqt.Im"));
    TS_ASSERT_DELTA(ws->y(4)[0], -656.82368, 1e-05);
    TS_ASSERT_DELTA(ws->y(4)[14], 656.82368, 1e-05);

  } // end of testExec

private:
  std::string m_inputFile;
  Mantid::DataHandling::LoadSassena m_alg;

}; // end of class LoadSassenaTest
