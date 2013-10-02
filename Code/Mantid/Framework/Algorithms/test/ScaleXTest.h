#ifndef SCALEXTEST_H_
#define SCALEXTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/ScaleX.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::MantidVec;

class ScaleXTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    Mantid::Algorithms::ScaleX scale;
    TS_ASSERT_EQUALS( scale.name(), "ScaleX" );
  }

  void testVersion()
  {
    Mantid::Algorithms::ScaleX scale;
    TS_ASSERT_EQUALS( scale.version(), 1 );
  }

  void testInit()
  {
    Mantid::Algorithms::ScaleX scale;
    TS_ASSERT_THROWS_NOTHING( scale.initialize() );
    TS_ASSERT( scale.isInitialized() );
  }

  void testMultiplyOnWS2D()
  {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    auto inputWS = WorkspaceCreationHelper::Create2DWorkspace123(10,10);
    double factor = 2.5;
    auto result = runScaleX(inputWS, "Multiply", factor);

    MatrixWorkspace::const_iterator inIt(*inputWS);
    for (MatrixWorkspace::const_iterator it(*result); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), 2.5*inIt->X() );
      TS_ASSERT_EQUALS( it->Y(), inIt->Y() );
      TS_ASSERT_EQUALS( it->E(), inIt->E() );
    }
  }

  void testAddOnWS2D()
  {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    auto inputWS = WorkspaceCreationHelper::Create2DWorkspace123(10,10);
    double factor = 2.5;
    auto result = runScaleX(inputWS, "Add", factor);

    MatrixWorkspace::const_iterator inIt(*inputWS);
    for (MatrixWorkspace::const_iterator it(*result); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), 2.5 + inIt->X() );
      TS_ASSERT_EQUALS( it->Y(), inIt->Y() );
      TS_ASSERT_EQUALS( it->E(), inIt->E() );
    }
  }

  void testMulitplyOnEvents()
  {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ScaleX scale;
    scale.initialize();

    auto inputWS = WorkspaceCreationHelper::CreateEventWorkspace2(10,10);
    double factor(2.5);
    auto result = runScaleX(inputWS, "Multiply", factor);

    TS_ASSERT_EQUALS("EventWorkspace", result->id());

    MatrixWorkspace::const_iterator inIt(*inputWS);
    for (MatrixWorkspace::const_iterator it(*result); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), 2.5*inIt->X() );
      TS_ASSERT_EQUALS( it->Y(), inIt->Y() );
      TS_ASSERT_EQUALS( it->E(), inIt->E() );
    }
  }

  void testAddOnEvents()
  {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ScaleX scale;
    scale.initialize();

    auto inputWS = WorkspaceCreationHelper::CreateEventWorkspace2(10,10);
    double factor(2.5);
    auto result = runScaleX(inputWS, "Add", factor);

    TS_ASSERT_EQUALS("EventWorkspace", result->id());

    MatrixWorkspace::const_iterator inIt(*inputWS);
    for (MatrixWorkspace::const_iterator it(*result); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), 2.5 + inIt->X() );
      TS_ASSERT_EQUALS( it->Y(), inIt->Y() );
      TS_ASSERT_EQUALS( it->E(), inIt->E() );
    }
  }


  void test_X_Scaled_By_Factor_Attached_To_Leaf_Component_Or_Higher_Level_Component_On_WS2D()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10,10);
    auto & pmap = inputWS->instrumentParameters();
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

    const size_t xsize = result->blocksize();
    for(size_t i = 0; i < result->getNumberHistograms(); ++i)
    {
      double factor(1.0);
      if(i == 0) factor = det1Factor;
      else if(i == 1) factor = det2Factor;
      else factor = instFactor;

      for(size_t j = 0; j < xsize; ++j)
      {
        TS_ASSERT_DELTA(result->readX(i)[j], factor*inputWS->readX(i)[j], 1e-12);
        TS_ASSERT_EQUALS(result->readY(i)[j], inputWS->readY(i)[j]);
        TS_ASSERT_EQUALS(result->readE(i)[j], inputWS->readE(i)[j]);
      }
    }

  }

  void test_X_Scaled_By_Factor_Attached_To_Leaf_Component_Or_Higher_Level_Component_On_Events()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    auto inputWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2,3);
    auto & pmap = inputWS->instrumentParameters();
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
    auto resultEventWS = boost::dynamic_pointer_cast<Mantid::API::IEventWorkspace>(result);
    TS_ASSERT(resultEventWS);

    for(size_t i = 0; i < resultEventWS->getNumberHistograms(); ++i)
    {
      double factor(1.0);
      if(i == 0) factor = det1Factor;
      else if(i == 1) factor = det2Factor;
      else factor = instFactor;

      auto inEvents = resultEventWS->getEventListPtr(i);
      auto outEvents = resultEventWS->getEventListPtr(i);
      TS_ASSERT_EQUALS(outEvents->getNumberEvents(), inEvents->getNumberEvents());

      auto inTOFs = inEvents->getTofs();
      auto outTOFs = outEvents->getTofs();
      TS_ASSERT_EQUALS(inTOFs.size(),outTOFs.size());
      for(size_t j = 0; i < inTOFs.size(); ++j)
      {
        TS_ASSERT_DELTA(outTOFs[j], factor*inTOFs[j], 1e-12);
      }
    }

  }


  //------------------------------- Failure cases --------------------------------------
  void testInputByInstrumentParameterThrowsForMissingParameter()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    Mantid::Algorithms::ScaleX scale;
    scale.initialize();
    scale.setRethrows(true);

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10,10);
    AnalysisDataService::Instance().add("tomultiply",inputWS);

    TS_ASSERT_THROWS_NOTHING( scale.setPropertyValue("InputWorkspace","tomultiply") );
    TS_ASSERT_THROWS_NOTHING( scale.setPropertyValue("OutputWorkspace","multiplied") );
    TS_ASSERT_THROWS_NOTHING( scale.setPropertyValue("InstrumentParameter","factor") );

    TS_ASSERT_THROWS(scale.execute(), std::runtime_error);
    TS_ASSERT( !scale.isExecuted() );

    AnalysisDataService::Instance().remove("tomultiply");
  }

  private:

    Mantid::API::MatrixWorkspace_sptr runScaleX(const Mantid::API::MatrixWorkspace_sptr & inputWS,
                                                const std::string & op, const double factor,
                                                const std::string & instPar = "")
    {
      Mantid::Algorithms::ScaleX scale;
      scale.initialize();
      scale.setChild(true);

      TS_ASSERT_THROWS_NOTHING( scale.setProperty("InputWorkspace",inputWS));
      TS_ASSERT_THROWS_NOTHING( scale.setPropertyValue("OutputWorkspace","__unused") );
      TS_ASSERT_THROWS_NOTHING( scale.setPropertyValue("Operation",op) );
      if(instPar.empty())
      {
        TS_ASSERT_THROWS_NOTHING( scale.setProperty("Factor",factor) );
      }
      else
      {
        TS_ASSERT_THROWS_NOTHING( scale.setProperty("InstrumentParameter", instPar) );
      }

      TS_ASSERT_THROWS_NOTHING( scale.execute() );
      TS_ASSERT( scale.isExecuted() );

      return scale.getProperty("OutputWorkspace");
    }

};

#endif /*SCALEXTEST_H_*/
