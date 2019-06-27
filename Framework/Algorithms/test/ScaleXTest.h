// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SCALEXTEST_H_
#define SCALEXTEST_H_

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ScaleX.h"
#include "MantidGeometry/Instrument.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/FrameworkManager.h"

using Mantid::HistogramData::Histogram;

class ScaleXTest : public CxxTest::TestSuite {
public:
  void testName() {
    Mantid::Algorithms::ScaleX scale;
    TS_ASSERT_EQUALS(scale.name(), "ScaleX");
  }

  void testVersion() {
    Mantid::Algorithms::ScaleX scale;
    TS_ASSERT_EQUALS(scale.version(), 1);
  }

  void testInit() {
    Mantid::Algorithms::ScaleX scale;
    TS_ASSERT_THROWS_NOTHING(scale.initialize());
    TS_ASSERT(scale.isInitialized());
  }

  void testMultiplyOnWS2D() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspace123(10, 10);
    double factor = 2.5;
    auto result = runScaleX(inputWS, "Multiply", factor);
    checkScaleFactorApplied(inputWS, result, factor, true); // multiply=true
  }

  void testAddOnWS2D() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspace123(10, 10);
    double factor = 2.5;
    auto result = runScaleX(inputWS, "Add", factor);
    checkScaleFactorApplied(inputWS, result, factor, false); // multiply=false
  }

  void testMulitplyOnEvents() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ScaleX scale;
    scale.initialize();

    auto inputWS = WorkspaceCreationHelper::createEventWorkspace2(10, 10);
    double factor(2.5);
    auto result = runScaleX(inputWS, "Multiply", factor);
    TS_ASSERT_EQUALS("EventWorkspace", result->id());
    checkScaleFactorApplied(inputWS, result, factor, true); // multiply=true
  }

  void testAddOnEvents() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ScaleX scale;
    scale.initialize();

    auto inputWS = WorkspaceCreationHelper::createEventWorkspace2(10, 10);
    double factor(2.5);
    auto result = runScaleX(inputWS, "Add", factor);
    TS_ASSERT_EQUALS("EventWorkspace", result->id());
    checkScaleFactorApplied(inputWS, result, factor, false); // multiply=false
  }

  void
  test_X_Scaled_By_Factor_Attached_To_Leaf_Component_Or_Higher_Level_Component_On_WS2D() {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    auto inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    auto &pmap = inputWS->instrumentParameters();
    const std::string parname("factor");

    const auto &spectrumInfo = inputWS->spectrumInfo();
    const auto &det1 = spectrumInfo.detector(0);
    const double det1Factor(5);
    pmap.addDouble(det1.getComponentID(), parname, det1Factor);

    const auto &det2 = spectrumInfo.detector(1);
    const double det2Factor(-10);
    pmap.addDouble(det2.getComponentID(), parname, det2Factor);

    const double instFactor(100);
    auto inst = inputWS->getInstrument();
    pmap.addDouble(inst->getComponentID(), parname, instFactor);

    auto result = runScaleX(inputWS, "Multiply", -1.0, parname);

    const size_t xsize = result->blocksize();
    TS_ASSERT_EQUALS(inputWS->blocksize(), xsize);

    // Test index 0 for factor 5
    double factor(det1Factor);
    checkScaleFactorAppliedAtHistIndex(inputWS->histogram(0),
                                       result->histogram(0), xsize, factor);

    // Test index 1 for factor -10
    factor = det2Factor;
    checkScaleFactorAppliedAtHistIndex(inputWS->histogram(1),
                                       result->histogram(1), xsize, factor);

    // Test the rest for factor 100
    // start at index 2 because 0 and 1 are checked above
    factor = instFactor;
    for (size_t i = 2; i < result->getNumberHistograms(); ++i) {
      checkScaleFactorAppliedAtHistIndex(inputWS->histogram(i),
                                         result->histogram(i), xsize, factor);
    }
  }

  void
  test_X_Scaled_By_Factor_Attached_To_Leaf_Component_Or_Higher_Level_Component_On_Events() {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    bool retainEventInfo = true;
    auto inputWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(
            2, 3, retainEventInfo);
    auto &pmap = inputWS->instrumentParameters();
    const std::string parname("factor");

    const auto &spectrumInfo = inputWS->spectrumInfo();
    const auto &det1 = spectrumInfo.detector(0);
    const double det1Factor(5);
    pmap.addDouble(det1.getComponentID(), parname, det1Factor);

    const auto &det2 = spectrumInfo.detector(1);
    const double det2Factor(-10);
    pmap.addDouble(det2.getComponentID(), parname, det2Factor);

    const double instFactor(100);
    auto inst = inputWS->getInstrument();
    pmap.addDouble(inst->getComponentID(), parname, instFactor);

    auto result = runScaleX(inputWS, "Multiply", -1, parname);
    auto resultEventWS =
        boost::dynamic_pointer_cast<Mantid::API::IEventWorkspace>(result);
    TS_ASSERT(resultEventWS);

    const size_t xsize = result->blocksize();
    TS_ASSERT_EQUALS(inputWS->blocksize(), xsize);

    // Test index 0 for factor 5
    double factor(det1Factor);
    checkTimeOfFlightEvents(inputWS->getSpectrum(0),
                            resultEventWS->getSpectrum(0), factor);
    checkScaleFactorAppliedAtHistIndex(inputWS->histogram(0),
                                       result->histogram(0), xsize, factor);

    // Test index 1 for factor -10
    factor = det2Factor;
    checkTimeOfFlightEvents(inputWS->getSpectrum(1),
                            resultEventWS->getSpectrum(1), factor);
    checkScaleFactorAppliedAtHistIndex(inputWS->histogram(1),
                                       result->histogram(1), xsize, factor);

    // Test the rest for factor 100
    // start at index 2 because 0 and 1 are checked above
    factor = instFactor;
    for (size_t i = 2; i < resultEventWS->getNumberHistograms(); ++i) {
      checkTimeOfFlightEvents(inputWS->getSpectrum(i),
                              resultEventWS->getSpectrum(i), factor);
      checkScaleFactorAppliedAtHistIndex(inputWS->histogram(i),
                                         result->histogram(i), xsize, factor);
    }
  }

  void
  testMultiplyOperationWithCombineMulitpliesTheInstrumentAndFactorArguments() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    auto inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    auto &pmap = inputWS->instrumentParameters();
    const std::string parname("factor");
    const double instFactor(10);
    auto inst = inputWS->getInstrument();
    pmap.addDouble(inst->getComponentID(), parname, instFactor);

    double algFactor(2.0);
    bool combine(true);
    auto result = runScaleX(inputWS, "Multiply", algFactor, parname, combine);
    checkScaleFactorApplied(inputWS, result, algFactor * instFactor,
                            true); // multiply=true
  }

  void testAddOperationWithCombineAddsTheInstrumentAndFactorArguments() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    auto inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    auto &pmap = inputWS->instrumentParameters();
    const std::string parname("factor");
    const double instFactor(10);
    auto inst = inputWS->getInstrument();
    pmap.addDouble(inst->getComponentID(), parname, instFactor);

    double algFactor(2.0);
    bool combine(true);
    auto result = runScaleX(inputWS, "Add", algFactor, parname, combine);
    checkScaleFactorApplied(inputWS, result, algFactor + instFactor,
                            false); // multiply=true
  }

  //------------------------------- Failure cases
  //--------------------------------------
  void testInputByInstrumentParameterThrowsForMissingParameter() {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    Mantid::Algorithms::ScaleX scale;
    scale.initialize();
    scale.setRethrows(true);

    auto inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    AnalysisDataService::Instance().add("tomultiply", inputWS);

    TS_ASSERT_THROWS_NOTHING(
        scale.setPropertyValue("InputWorkspace", "tomultiply"));
    TS_ASSERT_THROWS_NOTHING(
        scale.setPropertyValue("OutputWorkspace", "multiplied"));
    TS_ASSERT_THROWS_NOTHING(
        scale.setPropertyValue("InstrumentParameter", "factor"));

    TS_ASSERT_THROWS(scale.execute(), const std::runtime_error &);
    TS_ASSERT(!scale.isExecuted());

    AnalysisDataService::Instance().remove("tomultiply");
  }

  void testCombineInputFailsIfInstrumentParameterNotSupplied() {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    Mantid::Algorithms::ScaleX scale;
    scale.initialize();
    scale.setRethrows(true);

    auto inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    AnalysisDataService::Instance().add("tomultiply", inputWS);

    TS_ASSERT_THROWS_NOTHING(
        scale.setPropertyValue("InputWorkspace", "tomultiply"));
    TS_ASSERT_THROWS_NOTHING(
        scale.setPropertyValue("OutputWorkspace", "multiplied"));
    TS_ASSERT_THROWS_NOTHING(scale.setProperty("Combine", true));

    TS_ASSERT_THROWS(scale.execute(), const std::invalid_argument &);
    TS_ASSERT(!scale.isExecuted());

    AnalysisDataService::Instance().remove("tomultiply");
  }

private:
  Mantid::API::MatrixWorkspace_sptr
  runScaleX(const Mantid::API::MatrixWorkspace_sptr &inputWS,
            const std::string &op, const double factor = -1.0,
            const std::string &instPar = "", const bool combine = false) {
    Mantid::Algorithms::ScaleX scale;
    scale.initialize();
    scale.setChild(true);

    TS_ASSERT_THROWS_NOTHING(scale.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        scale.setPropertyValue("OutputWorkspace", "__unused"));
    TS_ASSERT_THROWS_NOTHING(scale.setPropertyValue("Operation", op));
    if (factor > 0.0)
      TS_ASSERT_THROWS_NOTHING(scale.setProperty("Factor", factor));
    if (combine || !instPar.empty())
      TS_ASSERT_THROWS_NOTHING(
          scale.setPropertyValue("InstrumentParameter", instPar))
    TS_ASSERT_THROWS_NOTHING(scale.setProperty("Combine", combine));

    TS_ASSERT_THROWS_NOTHING(scale.execute());
    TS_ASSERT(scale.isExecuted());

    return scale.getProperty("OutputWorkspace");
  }

  void checkTimeOfFlightEvents(const Mantid::API::IEventList &inEvents,
                               const Mantid::API::IEventList &outEvents,
                               const double factor) {

    TS_ASSERT_EQUALS(outEvents.getNumberEvents(), inEvents.getNumberEvents());

    auto inTOFs = inEvents.getTofs();
    auto outTOFs = outEvents.getTofs();
    TS_ASSERT_EQUALS(inTOFs.size(), outTOFs.size());

    for (size_t j = 0; j < inTOFs.size(); ++j) {
      TS_ASSERT_DELTA(outTOFs[j], factor * inTOFs[j], 1e-12);
    }
  }

  void checkScaleFactorApplied(
      const Mantid::API::MatrixWorkspace_const_sptr &inputWS,
      const Mantid::API::MatrixWorkspace_const_sptr &outputWS, double factor,
      bool multiply) {

    const size_t xsize = outputWS->blocksize();
    TS_ASSERT_EQUALS(inputWS->blocksize(), xsize);

    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      auto &outX = outputWS->x(i);
      auto &outY = outputWS->y(i);
      auto &outE = outputWS->e(i);

      auto &inX = inputWS->x(i);
      auto &inY = inputWS->y(i);
      auto &inE = inputWS->e(i);

      if (multiply) { // taken out of the tight loop, this way only 1 check
        // this branch will perform the multiplication assert
        for (size_t j = 0; j < xsize; ++j) {
          TS_ASSERT_DELTA(outX[j], factor * inX[j], 1e-12);
          TS_ASSERT_EQUALS(outY[j], inY[j]);
          TS_ASSERT_EQUALS(outE[j], inE[j]);
        }
      } else {

        // this branch will perform the plus assert
        for (size_t j = 0; j < xsize; ++j) {
          TS_ASSERT_DELTA(outX[j], factor + inX[j], 1e-12);
          TS_ASSERT_EQUALS(outY[j], inY[j]);
          TS_ASSERT_EQUALS(outE[j], inE[j]);
        }
      }
    }
  }

  void checkScaleFactorAppliedAtHistIndex(const Histogram &input,
                                          const Histogram &output,
                                          const size_t xsize,
                                          const double factor) {

    // get all histograms of input and output
    auto &outX = output.x();
    auto &outY = output.y();
    auto &outE = output.e();

    auto &inX = input.x();
    auto &inY = input.y();
    auto &inE = input.e();

    if (factor > 0) { // taken out of the tight loop
      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_DELTA(outX[j], factor * inX[j], 1e-12);
        TS_ASSERT_EQUALS(outY[j], inY[j]);
        TS_ASSERT_EQUALS(outE[j], inE[j]);
      }
    } else { // for negative factor
      for (size_t j = 0; j < xsize; ++j) {
        // ScaleX reverses the histogram if the factor is negative
        // X vector has length xsize+1
        TS_ASSERT_DELTA(outX[j], factor * inX[xsize - j], 1e-12);
        // Y and E have length xsize
        TS_ASSERT_EQUALS(outY[j], inY[xsize - 1 - j]);
        TS_ASSERT_EQUALS(outE[j], inE[xsize - 1 - j]);
      }
    }
  }
};

class ScaleXTestPerformance : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScaleXTestPerformance *createSuite() {
    return new ScaleXTestPerformance();
  }

  static void destroySuite(ScaleXTestPerformance *suite) { delete suite; }

  void setUp() override {
    inputMatrix = WorkspaceCreationHelper::create2DWorkspaceBinned(10000, 1000);
    inputEvent =
        WorkspaceCreationHelper::createEventWorkspace(10000, 1000, 5000);
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("output");
    Mantid::API::AnalysisDataService::Instance().remove("output2");
  }

  void testPerformanceMatrixWS() {
    Mantid::Algorithms::ScaleX scale;
    scale.initialize();
    scale.setProperty("InputWorkspace", inputMatrix);
    scale.setPropertyValue("OutputWorkspace", "output");
    scale.execute();
  }

  void testPerformanceEventWS() {
    Mantid::Algorithms::ScaleX scale;
    scale.initialize();
    scale.setProperty("InputWorkspace", inputMatrix);
    scale.setPropertyValue("OutputWorkspace", "output2");
    scale.execute();
  }

private:
  Mantid::API::MatrixWorkspace_sptr inputMatrix;
  Mantid::DataObjects::EventWorkspace_sptr inputEvent;
};

#endif /*SCALEXTEST_H_*/
