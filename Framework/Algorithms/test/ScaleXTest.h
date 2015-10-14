#ifndef SCALEXTEST_H_
#define SCALEXTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/ScaleX.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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
    const double det2Factor(10);
    pmap.addDouble(det2->getComponentID(), parname, det2Factor);

    const double instFactor(100);
    auto inst = inputWS->getInstrument();
    pmap.addDouble(inst->getComponentID(), parname, instFactor);

    auto result = runScaleX(inputWS, "Multiply", -1.0, parname);

    const size_t xsize = result->blocksize();
    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      double factor(1.0);
      if (i == 0)
        factor = det1Factor;
      else if (i == 1)
        factor = det2Factor;
      else
        factor = instFactor;

      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_DELTA(result->readX(i)[j], factor * inputWS->readX(i)[j],
                        1e-12);
        TS_ASSERT_EQUALS(result->readY(i)[j], inputWS->readY(i)[j]);
        TS_ASSERT_EQUALS(result->readE(i)[j], inputWS->readE(i)[j]);
      }
    }
  }

  void
  test_X_Scaled_By_Factor_Attached_To_Leaf_Component_Or_Higher_Level_Component_On_Events() {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    auto inputWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 3);
    auto &pmap = inputWS->instrumentParameters();
    const std::string parname("factor");

    auto det1 = inputWS->getDetector(0);
    const double det1Factor(5);
    pmap.addDouble(det1->getComponentID(), parname, det1Factor);

    auto det2 = inputWS->getDetector(1);
    const double det2Factor(10);
    pmap.addDouble(det2->getComponentID(), parname, det2Factor);

    const double instFactor(100);
    auto inst = inputWS->getInstrument();
    pmap.addDouble(inst->getComponentID(), parname, instFactor);

    auto result = runScaleX(inputWS, "Multiply", -1, parname);
    auto resultEventWS =
        boost::dynamic_pointer_cast<Mantid::API::IEventWorkspace>(result);
    TS_ASSERT(resultEventWS);

    for (size_t i = 0; i < resultEventWS->getNumberHistograms(); ++i) {
      double factor(1.0);
      if (i == 0)
        factor = det1Factor;
      else if (i == 1)
        factor = det2Factor;
      else
        factor = instFactor;

      auto inEvents = resultEventWS->getEventListPtr(i);
      auto outEvents = resultEventWS->getEventListPtr(i);
      TS_ASSERT_EQUALS(outEvents->getNumberEvents(),
                       inEvents->getNumberEvents());

      auto inTOFs = inEvents->getTofs();
      auto outTOFs = outEvents->getTofs();
      TS_ASSERT_EQUALS(inTOFs.size(), outTOFs.size());
      for (size_t j = 0; i < inTOFs.size(); ++j) {
        TS_ASSERT_DELTA(outTOFs[j], factor * inTOFs[j], 1e-12);
      }
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
        double resultX = (multiply) ? factor * inputWS->readX(i)[j]
                                    : factor + inputWS->readX(i)[j];
        TS_ASSERT_DELTA(outputWS->readX(i)[j], resultX, 1e-12);
        TS_ASSERT_EQUALS(outputWS->readY(i)[j], inputWS->readY(i)[j]);
        TS_ASSERT_EQUALS(outputWS->readE(i)[j], inputWS->readE(i)[j]);
      }
    }
  }
};

#endif /*SCALEXTEST_H_*/
