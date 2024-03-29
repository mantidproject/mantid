// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CopyInstrumentParameters.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using Mantid::Geometry::Instrument_const_sptr;
using Mantid::Geometry::ParameterMap;
using Mantid::Kernel::V3D;

class CopyInstrumentParametersTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(copyInstParam.name(), "CopyInstrumentParameters") }

  void testInit() {
    copyInstParam.initialize();
    TS_ASSERT(copyInstParam.isInitialized())
  }

  void testExec_SameInstr() {
    // Create input workspace with parameterized instrument and put into data
    // store
    MatrixWorkspace_sptr ws1 = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);
    const std::string wsName1("CopyInstParamWs1");
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    dataStore.add(wsName1, ws1);
    /// Create output workspace with the same base instrument and put into data
    /// store
    MatrixWorkspace_sptr ws2 = WorkspaceFactory::Instance().create(ws1);
    const std::string wsName2("CopyInstParamWs2");
    dataStore.add(wsName2, ws2);

    // Set properties
    TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("InputWorkspace", wsName1));
    TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("OutputWorkspace", wsName2));
    // Get instrument of input workspace and move some detectors
    ParameterMap *pmap;
    pmap = &(ws1->instrumentParameters());
    auto &detectorInfoWs1 = ws1->mutableDetectorInfo();
    Instrument_const_sptr instrument = ws1->getInstrument();
    detectorInfoWs1.setPosition(0, V3D(6.0, 0.0, 0.7));
    detectorInfoWs1.setPosition(1, V3D(6.0, 0.1, 0.7));

    // add auxiliary instrument parameters
    pmap->addDouble(instrument.get(), "Ei", 100);
    pmap->addString(instrument.get(), "some_param", "some_value");

    // Verify that a detector moved in the input workspace has not yet been
    // moved in the output workspace
    const auto &spectrumInfoWs2 = ws2->spectrumInfo();
    V3D newPos = spectrumInfoWs2.position(0);
    TS_ASSERT_DELTA(newPos.Z(), 5.0, 0.0001);

    // Execute Algorithm
    TS_ASSERT_THROWS_NOTHING(copyInstParam.execute());
    TS_ASSERT(copyInstParam.isExecuted());
    TS_ASSERT(!copyInstParam.isInstrumentDifferent());

    // Verify that the detectors in the output workspace have been moved as in
    // the input workspace before execution
    const auto &spectrumInfoWs2New = ws2->spectrumInfo();
    int id1 = spectrumInfoWs2New.detector(0).getID();
    V3D newPos1 = spectrumInfoWs2New.position(0);
    TS_ASSERT_EQUALS(id1, 1);
    TS_ASSERT_DELTA(newPos1.X(), 6.0, 0.0001);
    TS_ASSERT_DELTA(newPos1.Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(newPos1.Z(), 0.7, 0.0001);
    int id2 = spectrumInfoWs2New.detector(1).getID();
    V3D newPos2 = spectrumInfoWs2New.position(1);
    TS_ASSERT_EQUALS(id2, 2);
    TS_ASSERT_DELTA(newPos2.X(), 6.0, 0.0001);
    TS_ASSERT_DELTA(newPos2.Y(), 0.1, 0.0001);
    TS_ASSERT_DELTA(newPos2.Z(), 0.7, 0.0001);
    auto instr2 = ws2->getInstrument();
    auto param_names = instr2->getParameterNames();
    TS_ASSERT(param_names.find("Ei") != param_names.end());
    TS_ASSERT(param_names.find("some_param") != param_names.end());

    dataStore.remove(wsName1);
    dataStore.remove(wsName2);
  }

  // parameter for different instruments
  void testDifferent_BaseInstrument_DifferentMapReplaced() {
    // Create input workspace with parameterized instrument and put into data
    // store
    MatrixWorkspace_sptr ws1 =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(4, 10, true, false, true, "Instr_modified");
    const std::string wsName1("CopyInstParamWs1");
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    dataStore.add(wsName1, ws1);

    Instrument_const_sptr instrument = ws1->getInstrument();
    ParameterMap *pmap;
    pmap = &(ws1->instrumentParameters());
    // add auxiliary instrument parameters
    pmap->addDouble(instrument.get(), "Ei", 100);
    pmap->addString(instrument.get(), "some_param", "some_value");
    auto &detectorInfoWs1 = ws1->mutableDetectorInfo();
    detectorInfoWs1.setPosition(0, V3D(6.0, 0.0, 0.7));
    detectorInfoWs1.setPosition(3, V3D(6.0, 0.1, 0.7));

    // Create output workspace with another parameterized instrument and put
    // into data store
    MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);
    const std::string wsName2("CopyInstParamWs2");
    dataStore.add(wsName2, ws2);

    pmap = &(ws2->instrumentParameters());
    auto &detectorInfoWs2 = ws2->mutableDetectorInfo();
    instrument = ws2->getInstrument();
    pmap->addDouble(instrument.get(), "T", 10);
    pmap->addString(instrument.get(), "some_param", "other_value");
    detectorInfoWs2.setPosition(1, V3D(6.0, 0.2, 0.7));

    // Set properties
    TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("InputWorkspace", wsName1));
    TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("OutputWorkspace", wsName2));

    // Execute Algorithm, should warn but proceed
    copyInstParam.setRethrows(true);
    TS_ASSERT(copyInstParam.execute());
    TS_ASSERT(copyInstParam.isExecuted());
    TS_ASSERT(copyInstParam.isInstrumentDifferent());

    auto instr2 = ws2->getInstrument();
    auto param_names = instr2->getParameterNames();
    TS_ASSERT(param_names.find("Ei") != param_names.end());
    TS_ASSERT(param_names.find("some_param") != param_names.end());
    TS_ASSERT(param_names.find("T") == param_names.end());
    std::vector<std::string> rez = instr2->getStringParameter("some_param");
    TS_ASSERT(std::string("some_value") == rez[0]);
    TS_ASSERT_EQUALS(100, (instr2->getNumberParameter("Ei"))[0]);
    // TS_ASSERT_EQUALS(10,(instr2->getNumberParameter("T"))[0]);

    const auto &spectrumInfoWs2New = ws2->spectrumInfo();

    // new detector allocation applied
    const auto &deto1 = spectrumInfoWs2New.detector(0);
    int id1 = deto1.getID();
    V3D newPos1 = deto1.getPos();
    TS_ASSERT_EQUALS(id1, 1);
    TS_ASSERT_DELTA(newPos1.X(), 6.0, 0.0001);
    TS_ASSERT_DELTA(newPos1.Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(newPos1.Z(), 0.7, 0.0001);

    // previous detector placement rejected
    const auto &deto2 = spectrumInfoWs2New.detector(1);
    int id2 = deto2.getID();
    V3D newPos2 = deto2.getPos();
    TS_ASSERT_EQUALS(id2, 2);
    TS_ASSERT_DELTA(newPos2.X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(newPos2.Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(newPos2.Z(), -9.0, 0.0001);

    dataStore.remove(wsName1);
    dataStore.remove(wsName2);
  }

private:
  CopyInstrumentParameters copyInstParam;
};

class CopyInstrumentParametersTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CopyInstrumentParametersTestPerformance *createSuite() {
    return new CopyInstrumentParametersTestPerformance();
  }
  static void destroySuite(CopyInstrumentParametersTestPerformance *suite) { delete suite; }

  CopyInstrumentParametersTestPerformance() : m_SourceWSName("SourceWS"), m_TargetWSName("TargWS") {
    size_t n_detectors = 44327;
    size_t n_Parameters = 200;
    // Create input workspace with parameterized instrument and put into data
    // store
    MatrixWorkspace_sptr ws1 = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        static_cast<int>(n_detectors + 2), 10, true, false, true, "Instr_calibrated");
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    dataStore.add(m_SourceWSName, ws1);

    Instrument_const_sptr instrument = ws1->getInstrument();
    ParameterMap *pmap;
    pmap = &(ws1->instrumentParameters());
    for (size_t i = 0; i < n_Parameters; i++) {
      // add auxiliary instrument parameters
      pmap->addDouble(instrument.get(), "Param-" + boost::lexical_cast<std::string>(i), static_cast<double>(i * 10));
    }
    // calibrate detectors;
    auto &detectorInfo = ws1->mutableDetectorInfo();
    for (size_t i = 0; i < n_detectors; i++) {
      size_t detIndex = detectorInfo.indexOf(static_cast<Mantid::detid_t>(i + 1));
      detectorInfo.setPosition(detIndex, V3D(sin(M_PI * double(i)), cos(M_PI * double(i / 500)), 7));
    }

    // Create output workspace with another parameterized instrument and put
    // into data store
    MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        static_cast<int>(n_detectors), 10, true, false, true, "Instr_base");
    dataStore.add(m_TargetWSName, ws2);

    copyInstParam.initialize();
  }

public:
  void test_copy_performance() {

    // Set properties
    TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("InputWorkspace", m_SourceWSName));
    TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("OutputWorkspace", m_TargetWSName));

    // Execute Algorithm, should warn but proceed
    copyInstParam.setRethrows(true);

    clock_t t_start = clock();
    TS_ASSERT(copyInstParam.execute());
    clock_t t_end = clock();

    double seconds = static_cast<double>(t_end - t_start) / static_cast<double>(CLOCKS_PER_SEC);
    std::cout << " Time to copy all parameters is: " << seconds << " sec\n";

    TS_ASSERT(copyInstParam.isExecuted());
    TS_ASSERT(copyInstParam.isInstrumentDifferent());

    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    MatrixWorkspace_sptr ws2 = dataStore.retrieveWS<MatrixWorkspace>(m_TargetWSName);
    auto instr2 = ws2->getInstrument();

    auto param_names = instr2->getParameterNames();

    for (auto const &name : param_names) {
      double num = boost::lexical_cast<double>(name.substr(6, name.size() - 6));
      double val = instr2->getNumberParameter(name)[0];
      TS_ASSERT_DELTA(num * 10, val, 1.e-8);
    }

    // new detector allocation applied
    size_t nDetectors = ws2->getNumberHistograms();
    const auto &spectrumInfo = ws2->spectrumInfo();
    for (size_t i = 0; i < nDetectors; i++) {
      int id = spectrumInfo.detector(i).getID();
      V3D newPos1 = spectrumInfo.position(i);
      TS_ASSERT_EQUALS(id, i + 1);
      TS_ASSERT_DELTA(newPos1.X(), sin(M_PI * double(i)), 0.0001);
      TS_ASSERT_DELTA(newPos1.Y(), cos(M_PI * double(i / 500)), 0.0001);
      TS_ASSERT_DELTA(newPos1.Z(), 7, 1.e-6);
    }

    dataStore.remove(m_SourceWSName);
    dataStore.remove(m_TargetWSName);
  }

private:
  CopyInstrumentParameters copyInstParam;
  const std::string m_SourceWSName, m_TargetWSName;
};
