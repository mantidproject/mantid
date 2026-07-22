// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidMDAlgorithms/MDNormDirectSC.h"

using Mantid::MDAlgorithms::MDNormDirectSC;
using namespace Mantid::API;

class MDNormDirectSCTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDNormDirectSCTest *createSuite() { return new MDNormDirectSCTest(); }
  static void destroySuite(MDNormDirectSCTest *suite) { delete suite; }

  std::vector<MatrixWorkspace_sptr> m_event_vec;
  MatrixWorkspace_sptr m_event_ws;
  int m_numAngles, m_numHist, m_numEvents, m_numEbins;
  double m_Ei;

  void test_Init() {
    MDNormDirectSC alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_properties() {
    std::string mdWsName = "__temp_InputMDWorkspaceName";
    createMDWorkspace(mdWsName);
    std::string saWsName = "__temp_InputSAWorkspaceName";
    createSolidAngleWorkspace(saWsName);

    MDNormDirectSC alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", mdWsName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SolidAngleWorkspace", saWsName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "OutWSName"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputNormalizationWorkspace", "OutNormWSName"));

    AnalysisDataService::Instance().clear();
  }

  void test_compare_uselogtimes() {
    // Tests that using a MDE with multiple ExpInfos (original workflow) and with a single
    // ExpInfo and created using UseLogTimes=True are equivalent
    m_numAngles = 5;
    m_numHist = 5;
    m_numEvents = 10000;
    m_Ei = 100.;
    createEventWorkspaces();
    createMDEs();
    MDNormDirectSC alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim0", "Q1,-10,10,100"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim1", "Q2,-10,10,100"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim2", "Q3,-10,10,100"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim3", "DeltaE," + std::to_string(-m_Ei) + "," +
                                                                     std::to_string(0.9 * m_Ei) + ",100"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "MDNorm"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputNormalizationWorkspace", "MDNorm_single"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "MDE_single"));
    alg.execute();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "MDE_multiple"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputNormalizationWorkspace", "MDNorm_multiple"));
    alg.execute();
    IMDHistoWorkspace_sptr MDH_single =
        std::dynamic_pointer_cast<IMDHistoWorkspace>(AnalysisDataService::Instance().retrieve("MDNorm_single"));
    IMDHistoWorkspace_sptr MDH_multi =
        std::dynamic_pointer_cast<IMDHistoWorkspace>(AnalysisDataService::Instance().retrieve("MDNorm_multiple"));
    size_t np_single = MDH_single->getNPoints(), np_multi = MDH_multi->getNPoints();
    TS_ASSERT_EQUALS(np_single, np_multi);
    const auto sig_single = MDH_single->getSignalArray();
    const auto sig_multi = MDH_multi->getSignalArray();
    double diffsum = 0.0;
    for (size_t i = 0; i < np_single; i++) {
      diffsum += abs(sig_single[i] - sig_multi[i]);
    }
    TS_ASSERT_DELTA(diffsum, 0.0, 1e-3);
    for (auto wsname : {"MDE_single", "MDE_multiple", "MDNorm_single", "MDNorm_multiple", "MDNorm"}) {
      AnalysisDataService::Instance().remove(wsname);
    }
  }

private:
  void createMDWorkspace(const std::string &wsName) {
    const int ndims = 2;
    std::string bins = "2,2";
    std::string extents = "0,1,0,1";
    std::vector<std::string> names(ndims);
    names[0] = "A";
    names[1] = "B";
    std::vector<std::string> units(ndims);
    units[0] = "a";
    units[1] = "b";

    Mantid::MDAlgorithms::CreateMDWorkspace alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Dimensions", ndims));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Extents", extents));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Names", names));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Units", units));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", wsName));
    alg.execute();
  }

  void createSolidAngleWorkspace(const std::string &wsName) {
    auto sa = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);
    AnalysisDataService::Instance().addOrReplace(wsName, sa);
  }

  void createEventWorkspaces() {
    Mantid::Geometry::Goniometer gonio;
    gonio.pushAxis("Rot", 0., 1., 0., 0.);
    // Creates a single workspace with m_numAngles
    int nEv = m_numEvents * m_numAngles;
    double binsize = 10000.0 / nEv; // To get range of 10000us
    std::vector<Mantid::Types::Core::DateAndTime> pchargetimes(nEv), rottimes;
    std::vector<double> pcharges(nEv, 0.1), rotvals;
    Mantid::Types::Core::DateAndTime t0 = Mantid::Types::Core::DateAndTime("2010-01-01T00:00:00");
    std::generate(pchargetimes.begin(), pchargetimes.end(), [t0, n = 0]() mutable { return t0 + double(n++); });
    for (int i = 0; i < m_numAngles; i++) {
      rotvals.push_back(i * MDNormDirectSC::GONIOBINSTEP);
      rottimes.push_back(t0 + double(i * m_numEvents));
    }
    m_event_ws = std::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::createEventWorkspace(m_numHist, nEv, nEv, 0.0, binsize, 3, 0));
    m_event_ws->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(m_numHist));
    m_event_ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    m_event_ws->mutableRun().addProperty("Ei", m_Ei, "meV", true);
    m_event_ws->mutableRun().addProperty("gd_prtn_chrg", 0.1 * nEv);
    Mantid::Kernel::TimeSeriesProperty<double> *pchargelog =
        new Mantid::Kernel::TimeSeriesProperty<double>("proton_charge");
    pchargelog->addValues(pchargetimes, pcharges);
    m_event_ws->mutableRun().addLogData(std::move(pchargelog));
    Mantid::Kernel::TimeSeriesProperty<double> *rotlog = new Mantid::Kernel::TimeSeriesProperty<double>("Rot");
    rotlog->addValues(rottimes, rotvals);
    m_event_ws->mutableRun().addLogData(std::move(rotlog));
    m_event_ws->mutableRun().setGoniometer(gonio, true);
    // Creates a set of equivalent event workspaces at fixed angles
    binsize = 10000.0 / m_numEvents; // So we get a range of 10000us
    for (int i = 0; i < m_numAngles; i++) {
      Mantid::Types::Core::DateAndTime t1 = t0 + double(i * m_numEvents);
      MatrixWorkspace_sptr ws =
          std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::createEventWorkspaceWithStartTime(
              m_numHist, m_numEvents, m_numEvents, 0.0, binsize, 3, 0, t1));
      ws->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(m_numHist));
      ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
      ws->mutableRun().addProperty("Ei", m_Ei, "meV", true);
      ws->mutableRun().addProperty("gd_prtn_chrg", 0.1 * m_numEvents);
      // pchargelog = new Mantid::Kernel::TimeSeriesProperty<double>("proton_charge");
      // pchargelog->addValues(pchargetimes, pcharges);
      // ws->mutableRun().addLogData(std::move(pchargelog));
      rotlog = new Mantid::Kernel::TimeSeriesProperty<double>("Rot");
      rotlog->addValues(std::vector<Mantid::Types::Core::DateAndTime>(1, t1),
                        std::vector<double>(1, i * MDNormDirectSC::GONIOBINSTEP));
      ws->mutableRun().addLogData(std::move(rotlog));
      ws->mutableRun().setGoniometer(gonio, true);
      m_event_vec.push_back(ws);
    }
  }

  void createMDEs() {
    Mantid::MDAlgorithms::ConvertToMD convMDAlg;
    convMDAlg.initialize();
    convMDAlg.setPropertyValue("QDimensions", "Q3D");
    convMDAlg.setPropertyValue("dEAnalysisMode", "Direct");
    std::string qm = std::to_string(2.35 * sqrt(m_Ei));
    convMDAlg.setPropertyValue("MinValues", "-" + qm + ",-" + qm + ",-" + qm + "," + std::to_string(-m_Ei));
    convMDAlg.setPropertyValue("MaxValues", qm + "," + qm + "," + qm + "," + std::to_string(m_Ei));
    convMDAlg.setProperty("UseLogTimes", true);
    AnalysisDataService::Instance().addOrReplace("EvWS", m_event_ws);
    convMDAlg.setPropertyValue("InputWorkspace", "EvWS");
    convMDAlg.setPropertyValue("OutputWorkspace", "MDE_single");
    convMDAlg.execute();
    // Now create an MDE from multiple workspaces
    convMDAlg.setProperty("OverwriteExisting", false);
    convMDAlg.setProperty("UseLogTimes", false);
    for (auto ws : m_event_vec) {
      AnalysisDataService::Instance().addOrReplace("EvWS", ws);
      convMDAlg.setPropertyValue("InputWorkspace", "EvWS");
      convMDAlg.setPropertyValue("OutputWorkspace", "MDE_multiple");
      convMDAlg.execute();
    }
    AnalysisDataService::Instance().remove("EvWS");
  }
};
