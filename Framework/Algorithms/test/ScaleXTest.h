#ifndef SCALEXTEST_H_
#define SCALEXTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/ScaleX.h"
#include "MantidGeometry/Instrument.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <MantidAPI/FrameworkManager.h>

using Mantid::MantidVec;

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

    auto inputWS = WorkspaceCreationHelper::Create2DWorkspace123(10, 10);
    double factor = 2.5;
    auto result = runScaleX(inputWS, "Multiply", factor);
    testScaleFactorApplied(inputWS, result, factor, true); // multiply=true
  }

  void testAddOnWS2D() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    auto inputWS = WorkspaceCreationHelper::Create2DWorkspace123(10, 10);
    double factor = 2.5;
    auto result = runScaleX(inputWS, "Add", factor);
    testScaleFactorApplied(inputWS, result, factor, false); // multiply=false
  }

  void testMulitplyOnEvents() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ScaleX scale;
    scale.initialize();

    auto inputWS = WorkspaceCreationHelper::CreateEventWorkspace2(10, 10);
    double factor(2.5);
    auto result = runScaleX(inputWS, "Multiply", factor);
    TS_ASSERT_EQUALS("EventWorkspace", result->id());
    testScaleFactorApplied(inputWS, result, factor, true); // multiply=true
  }

  void testAddOnEvents() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ScaleX scale;
    scale.initialize();

    auto inputWS = WorkspaceCreationHelper::CreateEventWorkspace2(10, 10);
    double factor(2.5);
    auto result = runScaleX(inputWS, "Add", factor);
    TS_ASSERT_EQUALS("EventWorkspace", result->id());
    testScaleFactorApplied(inputWS, result, factor, false); // multiply=false
  }

  void
  test_X_Scaled_By_Factor_Attached_To_Leaf_Component_Or_Higher_Level_Component_On_WS2D() {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    auto inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    auto &pmap = inputWS->instrumentParameters();
    const std::string parname("factor");

    auto det1 = inputWS->getDetector(0);
    const double det1Factor(5);
    pmap.addDouble(det1->getComponentID(), parname, det1Factor);

    auto det2 = inputWS->getDetector(1);
    const double det2Factor(-10);
    pmap.addDouble(det2->getComponentID(), parname, det2Factor);

    const double instFactor(100);
    auto inst = inputWS->getInstrument();
    pmap.addDouble(inst->getComponentID(), parname, instFactor);

    auto result = runScaleX(inputWS, "Multiply", -1.0, parname);


	// Test indecies 0 for factor 5 and 
	double factor(det1Factor);
	testScaleFactorAppliedAtHistogramIndex(0, inputWS, result, factor, true);
	factor = det2Factor;
	testScaleFactorAppliedAtHistogramIndex(1, inputWS, result, factor, true);
	factor = instFactor;

    // start at index 2 because 0 and 1 are checked above
    for (size_t i = 2; i < result->getNumberHistograms(); ++i) {
		testScaleFactorAppliedAtHistogramIndex(i, inputWS, result, factor, true);
    }
  }

  void
  test_X_Scaled_By_Factor_Attached_To_Leaf_Component_Or_Higher_Level_Component_On_Events() {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    auto inputWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 3, false);
    auto &pmap = inputWS->instrumentParameters();
    const std::string parname("factor");

    auto det1 = inputWS->getDetector(0);
    const double det1Factor(5);
    pmap.addDouble(det1->getComponentID(), parname, det1Factor);

    auto det2 = inputWS->getDetector(1);
    const double det2Factor(-10);
    pmap.addDouble(det2->getComponentID(), parname, det2Factor);

    const double instFactor(100);
    auto inst = inputWS->getInstrument();
    pmap.addDouble(inst->getComponentID(), parname, instFactor);

    auto result = runScaleX(inputWS, "Multiply", -1, parname);
    auto resultEventWS =
        boost::dynamic_pointer_cast<Mantid::API::IEventWorkspace>(result);
    TS_ASSERT(resultEventWS);

    double factor(0.0);
    for (size_t i = 2; i < resultEventWS->getNumberHistograms(); ++i) {
      if (i == 0)
        factor = det1Factor;
      else if (i == 1)
        factor = det2Factor;
      else
        factor = instFactor;

      auto &inEvents = inputWS->getSpectrum(i);
      auto &outEvents = resultEventWS->getSpectrum(i);
      TS_ASSERT_EQUALS(outEvents.getNumberEvents(), inEvents.getNumberEvents());

      auto inTOFs = inEvents.getTofs();
      auto outTOFs = outEvents.getTofs();
      TS_ASSERT_EQUALS(inTOFs.size(), outTOFs.size());

      for (size_t j = 0; j < inTOFs.size(); ++j) {
        TS_ASSERT_DELTA(outTOFs[j], factor * inTOFs[j], 1e-12);
      }

      testScaleFactorAppliedAtHistogramIndex(i, inputWS, result, factor, true);
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
    testScaleFactorApplied(inputWS, result, algFactor * instFactor,
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
    testScaleFactorApplied(inputWS, result, algFactor + instFactor,
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

    TS_ASSERT_THROWS(scale.execute(), std::runtime_error);
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

    TS_ASSERT_THROWS(scale.execute(), std::invalid_argument);
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

  void testScaleFactorApplied(
      const Mantid::API::MatrixWorkspace_const_sptr &inputWS,
      const Mantid::API::MatrixWorkspace_const_sptr &outputWS, double factor,
      bool multiply) {
    const size_t xsize = outputWS->blocksize();
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < xsize; ++j) {
        double resultX =
            (multiply) ? factor * inputWS->x(i)[j] : factor + inputWS->x(i)[j];
        TS_ASSERT_DELTA(outputWS->x(i)[j], resultX, 1e-12);
        TS_ASSERT_EQUALS(outputWS->y(i)[j], inputWS->y(i)[j]);
        TS_ASSERT_EQUALS(outputWS->e(i)[j], inputWS->e(i)[j]);
      }
    }
  }

  /// 
  /**
  Only loops through the bins of the index of the parameter histogram
  The idea is to be called from within a loop that goes through the histograms
  like on line 113

  @param i :: Specifies the histogram index 
  */
  void testScaleFactorAppliedAtHistogramIndex(size_t i, const Mantid::API::MatrixWorkspace_const_sptr &inputWS,
		  const Mantid::API::MatrixWorkspace_const_sptr &outputWS, double factor,
		  bool multiply) {

	  // get bin sizes, outputWS and inputWS should be equal
	  const size_t xsize = outputWS->blocksize();
	  TS_ASSERT_EQUALS(inputWS->blocksize(), xsize);

	  // get all histograms of input and output
	  auto &outX = outputWS->x(i);
	  auto &outY = outputWS->y(i);
	  auto &outE = outputWS->e(i);

	  auto &inX = inputWS->x(i);
	  auto &inY = inputWS->y(i);
	  auto &inE = inputWS->e(i);

	  if (factor > 0) { // taken out of the tight loop
		  if (multiply) { // taken out of the tight loop
						  // this branch will perform the multiplication assert
			  for (size_t j = 0; j < xsize; ++j) {
				  TS_ASSERT_DELTA(outX[j], factor * inX[j], 1e-12);
				  TS_ASSERT_EQUALS(outY[j], inY[j]);
				  TS_ASSERT_EQUALS(outE[j], inE[j]);
			  }
		  }
		  else {

			  // this branch will perform the plus assert
			  for (size_t j = 0; j < xsize; ++j) {
				  TS_ASSERT_DELTA(outX[j], factor + inX[j], 1e-12);
				  TS_ASSERT_EQUALS(outY[j], inY[j]);
				  TS_ASSERT_EQUALS(outE[j], inE[j]);
			  }
		  }

	  }
	  else { // for negative factor

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

  void setUp() {
    inputMatrix = WorkspaceCreationHelper::Create2DWorkspaceBinned(10000, 1000);
    inputEvent =
        WorkspaceCreationHelper::CreateEventWorkspace(10000, 1000, 5000);
  }

  void tearDown() {
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
