// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceNearestNeighbourInfo.h"
#include "MantidAPI/WorkspaceNearestNeighbours.h"
#include "MantidAlgorithms/SmoothNeighbours.h"
#include "MantidKernel/ANN/ANN.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Algorithms;

class SmoothNeighboursTest : public CxxTest::TestSuite {

public:
  void doTestWithNumberOfNeighbours(const std::string &WeightedSum = "Flat") {
    MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(100, 10);

    SmoothNeighbours alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "testMW");
    alg.setProperty("PreserveEvents", false);
    alg.setProperty("WeightedSum", WeightedSum);
    alg.setProperty("NumberOfNeighbours", 8);
    alg.setProperty("IgnoreMaskedDetectors", true);
    alg.setProperty("Radius", 1.2);
    alg.setProperty("RadiusUnits", "NumberOfPixels");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testMW"));

    // Some basic checks
    TSM_ASSERT_EQUALS("Wrong number of histograms", inWS->getNumberHistograms(), outWS->getNumberHistograms());
    TSM_ASSERT_EQUALS("Wrong number of bins", inWS->x(0).size(), outWS->x(0).size());

    // Compare the workspaces
    for (size_t wi = 0; wi < inWS->getNumberHistograms(); wi++) {
      for (size_t j = 0; j < inWS->x(0).size(); j++) {
        TS_ASSERT_DELTA(inWS->x(wi)[j], outWS->x(wi)[j], 1e-5);
      }
      // Y is the same
      for (size_t j = 0; j < inWS->y(0).size(); j++) {
        TS_ASSERT_DELTA(inWS->y(wi)[j], outWS->y(wi)[j], 1e-5);
      }
      // Error has decreased due to adding step (improved statistics)
      // Therefore the output WS has lower errors than input:
      for (size_t j = 0; j < inWS->e(0).size(); j++) {
        TS_ASSERT_LESS_THAN(outWS->e(wi)[j], inWS->e(wi)[j]);
      }
      auto inDetIDs = inWS->getSpectrum(wi).getDetectorIDs();
      auto outDetIDs = outWS->getSpectrum(wi).getDetectorIDs();
      TS_ASSERT_LESS_THAN(inDetIDs.size(), outDetIDs.size());
      if (!inDetIDs.empty())
        TS_ASSERT_EQUALS(outDetIDs.count(*inDetIDs.begin()), 1);
    }

    AnalysisDataService::Instance().remove("testMW");
  }

  void do_test_non_uniform(EventType type, double *expectedY, const std::string &WeightedSum = "Parabolic",
                           bool PreserveEvents = true, double Radius = 0.001, bool ConvertTo2D = false,
                           int numberOfNeighbours = 8) {
    EventWorkspace_sptr in_ws = WorkspaceCreationHelper::createEventWorkspaceWithNonUniformInstrument(1, false);

    if (type == WEIGHTED) {
      in_ws *= 2.0;
      in_ws *= 0.5;
    }
    if (type == WEIGHTED_NOTIME) {
      for (size_t i = 0; i < in_ws->getNumberHistograms(); i++) {
        EventList &el = in_ws->getSpectrum(i);
        el.compressEvents(0.0, &el);
      }
    }

    // Multiply by 2 the workspace at index 4
    EventList &el = in_ws->getSpectrum(4);
    el += el;

    // Register the workspace in the data service
    AnalysisDataService::Instance().addOrReplace("SmoothNeighboursTest_input", in_ws);

    if (ConvertTo2D) {
      FrameworkManager::Instance().exec("ConvertToMatrixWorkspace", 4, "InputWorkspace", "SmoothNeighboursTest_input",
                                        "OutputWorkspace", "SmoothNeighboursTest_input");
    }

    SmoothNeighbours alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setRethrows(true);
    alg.setPropertyValue("InputWorkspace", "SmoothNeighboursTest_input");
    alg.setProperty("OutputWorkspace", "testEW");
    alg.setProperty("PreserveEvents", PreserveEvents);
    alg.setProperty("WeightedSum", WeightedSum);
    alg.setProperty("Radius", Radius);
    alg.setProperty("NumberOfNeighbours", numberOfNeighbours);
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    if (PreserveEvents) {
      EventWorkspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("testEW"));
      TS_ASSERT(ws);
      if (!ws)
        return;
      TS_ASSERT_LESS_THAN(in_ws->getNumberEvents(), ws->getNumberEvents());
    }

    // Result workspace
    MatrixWorkspace_sptr ws;

    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testEW"));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Compare to expected values
    TS_ASSERT_DELTA(ws->y(0)[0], expectedY[0], 1e-4);
    TS_ASSERT_DELTA(ws->y(1)[0], expectedY[1], 1e-4);
    TS_ASSERT_DELTA(ws->y(2)[0], expectedY[2], 1e-4);
    TS_ASSERT_DELTA(ws->y(3)[0], expectedY[3], 1e-4);
    TS_ASSERT_DELTA(ws->y(4)[0], expectedY[4], 1e-4);
    TS_ASSERT_DELTA(ws->y(5)[0], expectedY[5], 1e-4);
    TS_ASSERT_DELTA(ws->y(6)[0], expectedY[6], 1e-4);
    TS_ASSERT_DELTA(ws->y(7)[0], expectedY[7], 1e-4);
    TS_ASSERT_DELTA(ws->y(8)[0], expectedY[8], 1e-4);

    AnalysisDataService::Instance().remove("testEW");
  }

  void do_test_rectangular(EventType type, double *expectedY, const std::string &WeightedSum = "Parabolic",
                           bool PreserveEvents = true, bool ConvertTo2D = false) {
    // Pixels will be spaced 0.008 apart.
    EventWorkspace_sptr in_ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 20, false);

    if (type == WEIGHTED) {
      in_ws *= 2.0;
      in_ws *= 0.5;
    }
    if (type == WEIGHTED_NOTIME) {
      for (size_t i = 0; i < in_ws->getNumberHistograms(); i++) {
        EventList &el = in_ws->getSpectrum(i);
        el.compressEvents(0.0, &el);
      }
    }

    // Multiply by 2 the workspace at index 4
    EventList &el = in_ws->getSpectrum(4);
    el += el;

    size_t nevents0 = in_ws->getNumberEvents();

    // Register the workspace in the data service
    AnalysisDataService::Instance().addOrReplace("SmoothNeighboursTest_input", in_ws);

    if (ConvertTo2D) {
      FrameworkManager::Instance().exec("ConvertToMatrixWorkspace", 4, "InputWorkspace", "SmoothNeighboursTest_input",
                                        "OutputWorkspace", "SmoothNeighboursTest_input");
    }

    SmoothNeighbours alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setRethrows(true);
    alg.setPropertyValue("InputWorkspace", "SmoothNeighboursTest_input");
    alg.setProperty("OutputWorkspace", "testEW");
    alg.setProperty("PreserveEvents", PreserveEvents);
    alg.setProperty("WeightedSum", WeightedSum);
    alg.setProperty("AdjX", 1);
    alg.setProperty("AdjY", 1);
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    if (PreserveEvents) {
      EventWorkspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("testEW"));
      TS_ASSERT(ws);
      if (!ws)
        return;
      size_t nevents = ws->getNumberEvents();
      TS_ASSERT_LESS_THAN(nevents0, nevents);
    }

    // Check the values
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testEW");
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Compare to expected values
    TS_ASSERT_DELTA(ws->y(0)[0], expectedY[0], 1e-4);
    TS_ASSERT_DELTA(ws->y(1)[0], expectedY[1], 1e-4);
    TS_ASSERT_DELTA(ws->y(2)[0], expectedY[2], 1e-4);
    TS_ASSERT_DELTA(ws->y(3)[0], expectedY[3], 1e-4);
    TS_ASSERT_DELTA(ws->y(4)[0], expectedY[4], 1e-4);
    TS_ASSERT_DELTA(ws->y(5)[0], expectedY[5], 1e-4);
    TS_ASSERT_DELTA(ws->y(6)[0], expectedY[6], 1e-4);
    TS_ASSERT_DELTA(ws->y(7)[0], expectedY[7], 1e-4);
    TS_ASSERT_DELTA(ws->y(8)[0], expectedY[8], 1e-4);

    AnalysisDataService::Instance().remove("testEW");
  }

  /*
   * Start test Radius Filter.
   */
  void testRadiusThrowsIfNegativeCutoff() { TS_ASSERT_THROWS(RadiusFilter(-1), const std::invalid_argument &); }

  void testRadiusFiltering() {
    SpectraDistanceMap input;
    input[0] = V3D(1, 0, 0);
    input[1] = V3D(2, 0, 0);
    input[3] = V3D(3, 0, 0);

    RadiusFilter filter(2);
    SpectraDistanceMap product = filter.apply(input);

    TSM_ASSERT_EQUALS("Should have kept all but one of the inputs", 2, product.size());
    TS_ASSERT_EQUALS(1, input[0][0]);
    TS_ASSERT_EQUALS(2, input[1][0]);
  }

  /*
   * End test radius filter
   */

  void testWithUnsignedNumberOfNeighbours() {
    SmoothNeighbours alg;
    alg.initialize();
    TSM_ASSERT_THROWS("Cannot have number of neighbours < 1", alg.setProperty("NumberOfNeighbours", -1),
                      const std::invalid_argument &);
  }

  void testWithNonIntegerNumberOfNeighbours() {
    SmoothNeighbours alg;
    alg.initialize();
    TSM_ASSERT_THROWS("Cannot have non-integer number of neighbours", alg.setProperty("NumberOfNeighbours", 1.1),
                      const std::invalid_argument &);
  }

  void testWithValidNumberOfNeighbours() {
    SmoothNeighbours alg;
    alg.initialize();
    TSM_ASSERT_THROWS_NOTHING("A single neighbour is valid", alg.setProperty("NumberOfNeighbours", 1));
    int value = alg.getProperty("NumberOfNeighbours");
    TS_ASSERT_EQUALS(1, value);
  }

  void testWithNumberOfNeighboursAndFlatWeighting() { doTestWithNumberOfNeighbours("Flat"); }

  void testWithNumberOfNeighboursAndLinearWeighting() { doTestWithNumberOfNeighbours("Linear"); }

  void testWithNumberOfNeighboursAndParabolictWeighting() { doTestWithNumberOfNeighbours("Parabolic"); }

  void testWithNumberOfNeighboursAndGaussianWeighting() { doTestWithNumberOfNeighbours("Gaussian"); }

  void test_event_WEIGHTED() {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test_rectangular(WEIGHTED, expectedY);
  }

  void test_event_WEIGHTED_NOTIME() {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test_rectangular(WEIGHTED_NOTIME, expectedY);
  }

  void test_event_dont_PreserveEvents() {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test_rectangular(TOF, expectedY, "Parabolic");
  }

  void test_event() {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test_rectangular(TOF, expectedY);
  }

  void test_event_no_WeightedSum() {
    double expectedY[9] = {2, 2, 2, 2.3333, 2.3333, 2.3333, 2, 2, 2};
    do_test_rectangular(TOF, expectedY, "Flat", true /*PreserveEvents*/);
  }

  void test_event_Radius_no_WeightedSum() {
    double expectedY[9] = {2.5, 2.3333, 2.5, 2.3333, 2.2222, 2.3333, 2.5, 2.3333, 2.5};
    do_test_non_uniform(TOF, expectedY, "Flat", true /*PreserveEvents*/, 0.009 /* Radius */);
  }

  void test_event_Radius_WeightedSum() {
    double expectedY[9] = {2.2038, 2.3218, 2.2038, 2.3218, 2.5501, 2.3218, 2.2038, 2.3218, 2.2038};
    do_test_non_uniform(TOF, expectedY, "Linear", true /*PreserveEvents*/, 0.009 /* Radius */);
  }

  void test_workspace2D() {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test_rectangular(TOF, expectedY, "Parabolic", false /*PreserveEvents*/, true /*Convert2D*/);
  }

  void test_workspace2D_no_WeightedSum() {
    double expectedY[9] = {2, 2, 2, 2.3333, 2.3333, 2.3333, 2, 2, 2};
    do_test_rectangular(TOF, expectedY, "Flat", false /*PreserveEvents*/, true /*Convert2D*/);
  }

  void test_workspace2D_Radius_no_WeightedSum() {
    double expectedY[9] = {2.5, 2.3333, 2.5, 2.3333, 2.2222, 2.3333, 2.5, 2.3333, 2.5};
    do_test_non_uniform(TOF, expectedY, "Flat", false /*PreserveEvents*/, 0.009 /* Radius */, true /*Convert2D*/);
  }

  void test_workspace2D_Radius_WeightedSum() {
    double expectedY[9] = {2.2038, 2.3218, 2.2038, 2.3218, 2.5501, 2.3218, 2.2038, 2.3218, 2.2038};
    do_test_non_uniform(TOF, expectedY, "Linear", false /*PreserveEvents*/, 0.009 /* Radius */, true /*Convert2D*/);
  }

  void test_properties_in_no_group() {
    SmoothNeighbours alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // No Group
    Property *propWeightedSum = alg.getProperty("WeightedSum");
    Property *propIgnoreMaskedDetectors = alg.getProperty("IgnoreMaskedDetectors");
    Property *propSigma = alg.getProperty("Sigma");

    TS_ASSERT_EQUALS("", propWeightedSum->getGroup());
    TS_ASSERT_EQUALS("", propIgnoreMaskedDetectors->getGroup());
    TS_ASSERT_EQUALS("", propSigma->getGroup());
  }

  void test_properties_in_nonuniform_group() {
    SmoothNeighbours alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // NonUniform detector arrangement group
    Property *propNumberOfNeighbours = alg.getProperty("NumberOfNeighbours");
    Property *propRadius = alg.getProperty("Radius");
    Property *propRadiusUnits = alg.getProperty("RadiusUnits");
    Property *propSumNumberOfNeighbours = alg.getProperty("SumNumberOfNeighbours");

    TS_ASSERT_EQUALS("NonUniform Detectors", propNumberOfNeighbours->getGroup());
    TS_ASSERT_EQUALS("NonUniform Detectors", propRadius->getGroup());
    TS_ASSERT_EQUALS("NonUniform Detectors", propRadiusUnits->getGroup());
    TS_ASSERT_EQUALS("NonUniform Detectors", propSumNumberOfNeighbours->getGroup());
  }

  void test_properties_in_rectangular_group() {
    SmoothNeighbours alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // Uniform detector arrangement group
    Property *propAdjX = alg.getProperty("AdjX");
    Property *propAdjY = alg.getProperty("AdjY");
    Property *propSumPixelsX = alg.getProperty("SumPixelsX");
    Property *propSumPixelsY = alg.getProperty("SumPixelsY");
    Property *propZeroEdgePixels = alg.getProperty("ZeroEdgePixels");

    TS_ASSERT_EQUALS("Rectangular Detectors", propAdjX->getGroup());
    TS_ASSERT_EQUALS("Rectangular Detectors", propAdjY->getGroup());
    TS_ASSERT_EQUALS("Rectangular Detectors", propSumPixelsX->getGroup());
    TS_ASSERT_EQUALS("Rectangular Detectors", propSumPixelsY->getGroup());
    TS_ASSERT_EQUALS("Rectangular Detectors", propZeroEdgePixels->getGroup());
  }

  void test_WISH() {
    auto algo = AlgorithmManager::Instance().create("LoadRaw");
    algo->setPropertyValue("Filename", "WISH00019612.raw");
    algo->setPropertyValue("SpectrumMin", "1");
    algo->setPropertyValue("SpectrumMax", "19461");
    algo->setPropertyValue("LoadMonitors", "Separate");
    algo->setPropertyValue("OutputWorkspace", "WISH00019612");
    algo->execute();

    /*algo = AlgorithmManager::Instance().create("CropWorkspace");
    algo->setPropertyValue("InputWorkspace", "WISH00019612");
    algo->setPropertyValue("XMin", "6000");
    algo->setPropertyValue("XMax", "99000");
    algo->setPropertyValue("OutputWorkspace", "WISH00019612");
    algo->execute();

    algo = AlgorithmManager::Instance().create("CropWorkspace");
    algo->setPropertyValue("InputWorkspace", "WISH00019612_monitors");
    algo->setPropertyValue("XMin", "6000");
    algo->setPropertyValue("XMax", "99000");
    algo->setPropertyValue("OutputWorkspace", "WISH00019612_monitors");
    algo->execute();

    algo = AlgorithmManager::Instance().create("NormaliseByCurrent");
    algo->setPropertyValue("InputWorkspace", "WISH00019612");
    algo->setPropertyValue("OutputWorkspace", "WISH00019612");
    algo->execute();

    algo = AlgorithmManager::Instance().create("NormaliseByCurrent");
    algo->setPropertyValue("InputWorkspace", "WISH00019612_monitors");
    algo->setPropertyValue("OutputWorkspace", "WISH00019612_monitors");
    algo->execute();

    algo = AlgorithmManager::Instance().create("ConvertUnits");
    algo->setPropertyValue("InputWorkspace", "WISH00019612");
    algo->setPropertyValue("Target", "Wavelength");
    algo->setPropertyValue("OutputWorkspace", "WISH00019612");
    algo->execute();

    algo = AlgorithmManager::Instance().create("ConvertUnits");
    algo->setPropertyValue("InputWorkspace", "WISH00019612_monitors");
    algo->setPropertyValue("Target", "Wavelength");
    algo->setPropertyValue("OutputWorkspace", "WISH00019612_monitors");
    algo->execute();

    algo = AlgorithmManager::Instance().create("NormaliseToMonitor");
    algo->setPropertyValue("InputWorkspace", "WISH00019612");
    algo->setPropertyValue("MonitorWorkspaceIndex", "3");
    algo->setPropertyValue("MonitorWorkspace", "WISH00019612_monitors");
    algo->setPropertyValue("OutputWorkspace", "WISH00019612");
    algo->execute();

    algo = AlgorithmManager::Instance().create("ReplaceSpecialValues");
    algo->setPropertyValue("InputWorkspace", "WISH00019612");
    algo->setPropertyValue("NaNValue", "0");
    algo->setPropertyValue("InfinityValue", "0");
    algo->setPropertyValue("OutputWorkspace", "WISH00019612");
    algo->execute();

    algo = AlgorithmManager::Instance().create("CropWorkspace");
    algo->setPropertyValue("InputWorkspace", "WISH00019612");
    algo->setPropertyValue("XMin", "0.8");
    algo->setPropertyValue("XMax", "9.3");
    algo->setPropertyValue("OutputWorkspace", "WISH00019612");
    algo->execute();*/

    MatrixWorkspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("WISH00019612"));

    /*specnum_t inSpec = workspace->getSpectrum(19397).getSpectrumNo();

    API::WorkspaceNearestNeighbourInfo neighbourInfo(*workspace, true, 8);
    // Step one - Get the number of specified neighbours
    SpectraDistanceMap insideGrid = neighbourInfo.getNeighboursExact(inSpec);
    std::vector<specnum_t> expectedSpecNums{19146, 19147, 19274, 19275, 19276, 19277, 19402, 19404};
    int i = 0;
    for (std::map<specnum_t, Kernel::V3D>::iterator it = insideGrid.begin(); it != insideGrid.end(); ++it) {
      TS_ASSERT_EQUALS(it->first, expectedSpecNums[i]);
      i++;
      }*/

    std::vector<specnum_t> spectrumNumbers;
    const auto nhist = workspace->getNumberHistograms();
    spectrumNumbers.reserve(nhist);
    for (size_t i = 0; i < nhist; ++i)
      spectrumNumbers.emplace_back(workspace->getSpectrum(i).getSpectrumNo());

    auto m_nearestNeighbours =
        std::make_unique<WorkspaceNearestNeighbours>(8, workspace->spectrumInfo(), std::move(spectrumNumbers), true);
    /*SpectraDistanceMap insideGrid2 = m_nearestNeighbours->neighbours(inSpec);
    i = 0;
    for (std::map<specnum_t, Kernel::V3D>::iterator it = insideGrid2.begin(); it != insideGrid2.end(); ++it) {
      TS_ASSERT_EQUALS(it->first, expectedSpecNums[i]);
      i++;
    }

    // check WorkspaceNearestNeighbours::build()
    TS_ASSERT_EQUALS(m_nearestNeighbours->m_specToVertex.size(), 19456);
    TS_ASSERT_EQUALS(m_nearestNeighbours->m_specToVertex[19403], 19397);
    auto vertex = m_nearestNeighbours->m_specToVertex.find(19403);
    std::pair<WorkspaceNearestNeighbours::Graph::adjacency_iterator,
              WorkspaceNearestNeighbours::Graph::adjacency_iterator>
        adjacent = boost::adjacent_vertices(vertex->second, m_nearestNeighbours->m_graph);
    WorkspaceNearestNeighbours::Graph::adjacency_iterator adjIt;
    std::vector<int> specNs;
    for (adjIt = adjacent.first; adjIt != adjacent.second; adjIt++) {
      WorkspaceNearestNeighbours::Vertex nearest = (*adjIt);
      auto nrSpec = specnum_t(m_nearestNeighbours->m_vertexID[nearest]);
      specNs.push_back(nrSpec);
    }
    TS_ASSERT_EQUALS(specNs[0], 19275);
    TS_ASSERT_EQUALS(specNs[1], 19404);
    TS_ASSERT_EQUALS(specNs[2], 19402);
    TS_ASSERT_EQUALS(specNs[3], 19276);
    TS_ASSERT_EQUALS(specNs[4], 19274);
    TS_ASSERT_EQUALS(specNs[5], 19147);
    TS_ASSERT_EQUALS(specNs[6], 19277);
    TS_ASSERT_EQUALS(specNs[7], 19146);*/

    const auto indices = m_nearestNeighbours->getSpectraDetectors();
    TS_ASSERT_EQUALS(indices.size(), 19456);

    Mantid::API::SpectrumInfo specInfo = workspace->spectrumInfo();
    Mantid::Geometry::BoundingBox bbox;
    // Base the scaling on the first detector, should be adequate but we can look
    // at this
    const auto &firstDet = specInfo.detector(indices.front());
    firstDet.getBoundingBox(bbox);
    auto m_scale = V3D(bbox.width());
    TS_ASSERT_EQUALS(m_scale[0], 0.011467840514357563);
    TS_ASSERT_EQUALS(m_scale[1], 0.0099093322347372226);
    TS_ASSERT_EQUALS(m_scale[2], 0.011487383212974045);

    ANNpointArray dataPoints = annAllocPts(static_cast<int>(nhist), 3);
    std::vector<specnum_t> spectrumNumbers2;
    spectrumNumbers2.reserve(nhist);
    for (size_t i = 0; i < nhist; ++i)
      spectrumNumbers2.emplace_back(workspace->getSpectrum(i).getSpectrumNo());
    int pointNo = 0;
    for (const auto i : indices) {
      // const specnum_t spectrum = spectrumNumbers2[i];
      V3D pos = specInfo.position(i) / m_scale;
      dataPoints[pointNo][0] = pos.X();
      dataPoints[pointNo][1] = pos.Y();
      dataPoints[pointNo][2] = pos.Z();
      ++pointNo;
    }
    auto annTree = std::make_unique<ANNkd_tree>(dataPoints, static_cast<int>(nhist), 3);

    // Run the nearest neighbour search on each detector, reusing the arrays
    // Set size initially to avoid array index error when testing in debug mode
    std::vector<ANNidx> nnIndexList(12);
    std::vector<ANNdist> nnDistList(12);
    ANNpoint scaledPos = dataPoints[19397];
    annTree->annkSearch(scaledPos,          // Point to search nearest neighbours of
                        12,                 // Number of neighbours to find
                        nnIndexList.data(), // Index list of results
                        nnDistList.data(),  // List of distances to each of these
                        0.0);               // Error bound (?) is this the radius to search in?

    TS_ASSERT_EQUALS(nnIndexList[0], 19269);
    TS_ASSERT_EQUALS(nnIndexList[1], 19396);
    TS_ASSERT_EQUALS(nnIndexList[2], 19398);
    TS_ASSERT_EQUALS(nnIndexList[3], 19270);
    TS_ASSERT_EQUALS(nnIndexList[4], 19268);
    TS_ASSERT_EQUALS(nnIndexList[5], 19141);
    TS_ASSERT_EQUALS(nnIndexList[6], 19271);
    TS_ASSERT_EQUALS(nnIndexList[7], 19140);
    TS_ASSERT_EQUALS(nnIndexList[8], 19142);

    TS_ASSERT_DELTA(nnDistList[0], 0.59042046182435803,
                    std::numeric_limits<double>::epsilon() * (nnDistList[0] + 0.59042046182435803));
    TS_ASSERT_DELTA(nnDistList[1], 0.67358344667063674,
                    std::numeric_limits<double>::epsilon() * (nnDistList[1] + 0.6735834466706367));
    TS_ASSERT_DELTA(nnDistList[2], 0.67358344667063597,
                    std::numeric_limits<double>::epsilon() * (nnDistList[2] + 0.673583446670635974));
    TS_ASSERT_DELTA(nnDistList[3], 0.76706550105551297,
                    std::numeric_limits<double>::epsilon() * (nnDistList[3] + 0.76706550105551297));
    TS_ASSERT_DELTA(nnDistList[4], 1.7609423159344773,
                    std::numeric_limits<double>::epsilon() * (nnDistList[4] + 1.7609423159344773));
    TS_ASSERT_DELTA(nnDistList[5], 1.9952666377146773,
                    std::numeric_limits<double>::epsilon() * (nnDistList[5] + 1.9952666377146773));
    TS_ASSERT_DELTA(nnDistList[6], 2.2908774336279398,
                    std::numeric_limits<double>::epsilon() * (nnDistList[6] + 2.2908774336279398));
    // 8th and 9th neighbour distances only different in 15th dp
    TS_ASSERT_DELTA(nnDistList[7], 2.6688500843853133,
                    std::numeric_limits<double>::epsilon() * (nnDistList[7] + 2.6688500843853133));
    TS_ASSERT_DELTA(nnDistList[8], 2.6688500843853147,
                    std::numeric_limits<double>::epsilon() * (nnDistList[8] + 2.6688500843853147));

    Mantid::Kernel::Logger g_log("SmoothNeighboursTest");
    for (int i = 0; i < 12; i++)
      g_log.error() << "nnIndexList[" << i << "]=" << nnIndexList[i] << std::setprecision(17) << ", nnDistList[" << i
                    << "]=" << nnDistList[i] << std::endl;

    g_log.error() << std::setprecision(17) << "epsilon=" << std::numeric_limits<double>::epsilon() << std::endl;
    g_log.error() << std::setprecision(17) << std::abs(nnDistList[8] - nnDistList[7]) << std::endl;
    // TS_ASSERT(std::abs(nnDistList[8]-nnDistList[7])<std::numeric_limits<double>::epsilon()*(nnDistList[7]+nnDistList[8]));
  }
}

;

class SmoothNeighboursTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SmoothNeighboursTestPerformance *createSuite() { return new SmoothNeighboursTestPerformance(); }
  static void destroySuite(SmoothNeighboursTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }
  SmoothNeighboursTestPerformance() { FrameworkManager::Instance(); }

  void setUp() override { inWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(500, 10); }

  void testNeighboursAndFlatWeighting() {
    // Test with number of neighbours and flat weighting

    SmoothNeighbours alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "testMW");
    alg.setProperty("PreserveEvents", false);
    alg.setProperty("WeightedSum", "Flat");
    alg.setProperty("NumberOfNeighbours", 200);
    alg.setProperty("IgnoreMaskedDetectors", true);
    alg.setProperty("Radius", 10.0);
    alg.setProperty("RadiusUnits", "NumberOfPixels");
    alg.execute();
  }

private:
  MatrixWorkspace_sptr inWS;
};
