#ifndef MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_
#define MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ImplicitFunctionBuilder.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidAPI/ImplicitFunctionParser.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidMDAlgorithms/BinMD.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidMDAlgorithms/FakeMDEventData.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <boost/math/special_functions/fpclassify.hpp>

#include <cxxtest/TestSuite.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;
using Mantid::coord_t;

class BinMDTest : public CxxTest::TestSuite
{

private:

  //Helper class. Mock Implicit function.
  class MockImplicitFunction : public Mantid::Geometry::MDImplicitFunction
  {
  public:
    using MDImplicitFunction::isPointContained; // Avoids Intel compiler warning.
    virtual bool isPointContained(const Mantid::coord_t *)
    {
      return false;
    }
    virtual std::string getName() const
    {
      return "MockImplicitFunction";
    }
    MOCK_CONST_METHOD0(toXMLString, std::string());
    ~MockImplicitFunction()   {;}
  };

  //Helper class. Builds mock implicit functions.
  class MockImplicitFunctionBuilder : public Mantid::API::ImplicitFunctionBuilder
  {
  public:
    Mantid::Geometry::MDImplicitFunction* create() const
    {
      return new MockImplicitFunction;
    }
  };

  //Helper class. Parses mock Implicit Functions.
  class MockImplicitFunctionParser : public Mantid::API::ImplicitFunctionParser
  {
  public:
    MockImplicitFunctionParser() : Mantid::API::ImplicitFunctionParser(NULL){}
    Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* /*functionElement*/)
    {
      return new MockImplicitFunctionBuilder;
    }
    void setSuccessorParser(Mantid::API::ImplicitFunctionParser* /*successor*/){}
    void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* /*parser*/){}
  };
  // helper ws creator
  Mantid::API::Workspace_sptr createSimple3DWorkspace()
  {
    using namespace Mantid::API;
    AnalysisDataService::Instance().remove("3D_Workspace");
    CreateMDWorkspace create;

    create.initialize();
    create.setProperty("Dimensions", 3);
    create.setPropertyValue("Extents","0,10,0,10,0,10");
    create.setPropertyValue("Names","x,y,z");
    create.setPropertyValue("Units","m,m,m");
    create.setPropertyValue("SplitInto","10");
    create.setPropertyValue("OutputWorkspace", "3D_Workspace");
    create.execute();
    return AnalysisDataService::Instance().retrieve("3D_Workspace");
  }


public:

  void testSetup()
  {
    using namespace Mantid::Kernel;
    Mantid::API::ImplicitFunctionFactory::Instance().subscribe<testing::NiceMock<MockImplicitFunction> >("MockImplicitFunction"); 
    Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<MockImplicitFunctionParser >("MockImplicitFunctionParser"); 
  }

  void test_Init()
  {
    BinMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  

  /** Test the algo
  * @param nameX : name of the axis
  * @param expected_signal :: how many events in each resulting bin
  * @param expected_numBins :: how many points/bins in the output
  */
  void do_test_exec(std::string functionXML,
      std::string name1, std::string name2, std::string name3, std::string name4,
      double expected_signal,
      size_t expected_numBins,
      bool IterateEvents=true,
      size_t numEventsPerBox=1,
      VMD expectBasisX = VMD(1,0,0), VMD expectBasisY = VMD(0,1,0), VMD expectBasisZ = VMD(0,0,1))
  {
    BinMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    IMDEventWorkspace_sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, numEventsPerBox);
    Mantid::Kernel::SpecialCoordinateSystem appliedCoord = Mantid::Kernel::QSample;
    in_ws->setCoordinateSystem(appliedCoord);
    AnalysisDataService::Instance().addOrReplace("BinMDTest_ws", in_ws);
    
    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000*numEventsPerBox);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "BinMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim0", name1));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim1", name2));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim2", name3));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim3", name4));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ImplicitFunctionXML",functionXML));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IterateEvents", IterateEvents));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinMDTest_ws"));

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )

    TS_ASSERT( alg.isExecuted() );

    MDHistoWorkspace_sptr out ;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<MDHistoWorkspace>(
        AnalysisDataService::Instance().retrieve("BinMDTest_ws")); )
    TS_ASSERT(out);
    if(!out) return;

    TS_ASSERT_EQUALS(appliedCoord, out->getSpecialCoordinateSystem());
    // Took 6x6x6 bins in the middle of the box
    TS_ASSERT_EQUALS(out->getNPoints(), expected_numBins);
    // Every box has a single event summed into it, so 1.0 weight
    for (size_t i=0; i < out->getNPoints(); i++)
    {
      if (functionXML == "")
      {
        // Nothing rejected
        TS_ASSERT_DELTA(out->getSignalAt(i), expected_signal, 1e-5);
        TS_ASSERT_DELTA(out->getNumEventsAt(i), expected_signal, 1e-5);
        TS_ASSERT_DELTA(out->getErrorAt(i), sqrt(expected_signal), 1e-5);
      }
      else
      {
        // All NAN cause of implicit function
        TS_ASSERT( boost::math::isnan( out->getSignalAt(i) ) ); //The implicit function should have ensured that no bins were present.
      }
    }
    // check basis vectors
    TS_ASSERT_EQUALS( out->getBasisVector(0), expectBasisX);
    if (out->getNumDims() > 1) { TS_ASSERT_EQUALS( out->getBasisVector(1), expectBasisY); }
    if (out->getNumDims() > 2) { TS_ASSERT_EQUALS( out->getBasisVector(2), expectBasisZ); }
    const CoordTransform * ctFrom = out->getTransformFromOriginal();
    TS_ASSERT(ctFrom);
    // Experiment Infos were copied
    TS_ASSERT_EQUALS( out->getNumExperimentInfo(), in_ws->getNumExperimentInfo());

    AnalysisDataService::Instance().remove("BinMDTest_ws");
  }

  void test_exec_3D()
  { do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, true /*IterateEvents*/ );
  }


  void test_exec_3D_scrambled_order()
  { do_test_exec("", "Axis1,2.0,8.0, 6", "Axis0,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, true /*IterateEvents*/, 1,
      VMD(0,1,0), VMD(1,0,0), VMD(0,0,1));
  }

  void test_exec_3D_unevenSizes()
  { 
    do_test_exec("", "Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 2.0 /*signal*/, 6*6*3 /*# of bins*/, true /*IterateEvents*/ );
  }


  void test_exec_2D()
  { // Integrate over the 3rd dimension
    do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "", "", 1.0*10.0 /*signal*/, 6*6 /*# of bins*/, true /*IterateEvents*/ );
  }

  void test_exec_2D_largeBins()
  {
    do_test_exec("", "Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "", "", 4.0*10.0 /*signal*/, 3*3 /*# of bins*/, true /*IterateEvents*/ );
  }


  void test_exec_2D_scrambledAndUnevent()
  { do_test_exec("", "Axis0,2.0,8.0, 3", "Axis2,2.0,8.0, 6", "", "", 2.0*10.0 /*signal*/, 3*6 /*# of bins*/, true /*IterateEvents*/, 1,
      VMD(1,0,0), VMD(0,0,1));
  }

  void test_exec_1D()
  { do_test_exec("", "Axis2,2.0,8.0, 6", "", "", "", 1.0*100.0 /*signal*/, 6 /*# of bins*/, true /*IterateEvents*/, 1,
      VMD(0,0,1)  );
  }

  void test_exec_1D_boxCompletelyContained()
  { do_test_exec("", "Axis2,2.0,8.0, 1", "", "", "", 20*6.0*100.0 /*signal*/, 1 /*# of bins*/, true /*IterateEvents*/, 20 /*numEventsPerBox*/,
      VMD(0,0,1) );
  }

  bool etta(int x,int base)
  {
    int ii = x-base/2;
    if(ii<0)return false;
    return !(ii%base);

  }

  void testExecLagreRegularSignal()
  {

    CreateMDWorkspace creator;

    FakeMDEventData FakeDat;
    TS_ASSERT_THROWS_NOTHING( FakeDat.initialize() )
    TS_ASSERT( FakeDat.isInitialized() )

    //MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(3, 0.0, 10.0, 0);
    auto a_ws  = createSimple3DWorkspace();
    MDEventWorkspace3Lean::sptr in_ws = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(a_ws);
    TS_ASSERT(in_ws);
    if(!in_ws)return;

    AnalysisDataService::Instance().addOrReplace("FakeMDEventDataTest_ws", in_ws);


    TS_ASSERT_THROWS_NOTHING(FakeDat.setPropertyValue("InputWorkspace", "FakeMDEventDataTest_ws") );
    TS_ASSERT_THROWS_NOTHING(FakeDat.setPropertyValue("PeakParams", ""));
    TS_ASSERT_THROWS_NOTHING(FakeDat.setPropertyValue("UniformParams", "-1000,0.50001,1,0.50001,1,0.50001,1"));

    TS_ASSERT_THROWS_NOTHING( FakeDat.execute(); )
    TS_ASSERT( FakeDat.isExecuted() );

    // Now there are at 1000 points.
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000);
    TS_ASSERT_DELTA( in_ws->getBox()->getSignal(), 1000.0, 1.e-6);
    TS_ASSERT_DELTA( in_ws->getBox()->getErrorSquared(), 1000.0, 1.e-6);



    BinMD BinAlg;
    TS_ASSERT_THROWS_NOTHING( BinAlg.initialize() )
    TS_ASSERT( BinAlg.isInitialized() )

    TS_ASSERT_THROWS_NOTHING(BinAlg.setPropertyValue("InputWorkspace", "FakeMDEventDataTest_ws") );
    TS_ASSERT_THROWS_NOTHING(BinAlg.setPropertyValue("AlignedDim0", "x,0,10,40"));
    TS_ASSERT_THROWS_NOTHING(BinAlg.setPropertyValue("AlignedDim1", "y,0,10,5"));
    TS_ASSERT_THROWS_NOTHING(BinAlg.setPropertyValue("AlignedDim2", "z,0,10,20"));

    TS_ASSERT_THROWS_NOTHING(BinAlg.setPropertyValue("OutputWorkspace", "BinMDTest_ws"));

    TS_ASSERT_THROWS_NOTHING( BinAlg.execute(); )

    TS_ASSERT( BinAlg.isExecuted() );

    MDHistoWorkspace_sptr out ;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<MDHistoWorkspace>(
        AnalysisDataService::Instance().retrieve("BinMDTest_ws")); )
    TSM_ASSERT("can not retrieve binned workspace from analysis data service",out);
    if(!out) return;

    TS_ASSERT_EQUALS( out->getNEvents(), 1000);    

    double expected_signal(2.);
    std::vector<size_t> nBins(3),indexes(3);
    nBins[0]=40;nBins[1]=5;nBins[2]=20;

    for (size_t i=0; i < out->getNPoints(); i++)
    {
      Utils::getIndicesFromLinearIndex(i,nBins,indexes);
      if(etta(int(indexes[0]),4)&&etta(int(indexes[2]),2))
      {
        TS_ASSERT_DELTA(out->getSignalAt(i), expected_signal, 1e-5);
        TS_ASSERT_DELTA(out->getNumEventsAt(i), expected_signal, 1e-5);
        TS_ASSERT_DELTA(out->getErrorAt(i), sqrt(expected_signal), 1e-5);
      }
      else
      {
        TS_ASSERT_DELTA(out->getSignalAt(i), 0, 1e-5);
        TS_ASSERT_DELTA(out->getNumEventsAt(i), 0, 1e-5);
        TS_ASSERT_DELTA(out->getErrorAt(i), 0, 1e-5);

      }
    }
    
    AnalysisDataService::Instance().remove("FakeMDEventDataTest_ws");
    AnalysisDataService::Instance().remove("BinMDTest_ws");
  }




  void test_exec_with_impfunction()
  {
    //This describes the local implicit function that will always reject bins. so output workspace should have zero.
    std::string functionXML = std::string("<Function>")+
        "<Type>MockImplicitFunction</Type>"+
        "<ParameterList>"+
        "</ParameterList>"+
        "</Function>";
    do_test_exec(functionXML,  "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, false /*IterateEvents*/ );
  }
  void test_exec_with_impfunction_IterateEvents()
  { //This describes the local implicit function that will always reject bins. so output workspace should have zero.
    std::string functionXML = std::string("<Function>")+
        "<Type>MockImplicitFunction</Type>"+
        "<ParameterList>"+
        "</ParameterList>"+
        "</Function>";
    do_test_exec(functionXML,  "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, true /*IterateEvents*/ );
  }










  /** Test the algorithm, with a coordinate transformation.
   *
  * @param binsX : # of bins in the output
  * @param expected_signal :: how many events in each resulting bin
  * @param expected_numBins :: how many points/bins in the output
  * @param FlipYBasis :: flip the Y basis vector
  */
  void do_test_transform(int binsX, int binsY, int binsZ,
      double expected_signal,
      size_t expected_numBins,
      bool IterateEvents,
      bool ForceOrthogonal,
      bool FlipYBasis = false)
  {
    BinMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    // Make a workspace with events along a regular grid that is rotated and offset along x,y
    MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 20.0, 0);
    in_ws->splitBox();
    double theta = 0.1;
    VMD origin(-2.0, -3.0, -4.0);
    for (coord_t ox=0.5; ox<10; ox++)
      for (coord_t oy=0.5; oy<10; oy++)
        for (coord_t oz=0.5; oz<10; oz++)
        {
          double x = ox*cos(theta) - oy*sin(theta) + origin[0];
          double y = oy*cos(theta) + ox*sin(theta) + origin[1];
          double z = oz + origin[2];
          double center[3] = {x,y,z};
          MDLeanEvent<3> ev(1.0, 1.0, center);
//          std::cout << x << "," << y << "," << z << std::endl;
          in_ws->addEvent(ev);
        }
    in_ws->refreshCache();


    // Build the transformation (from eventWS to binned workspace)
    CoordTransformAffine ct(3,3);

    // Build the basis vectors, a 0.1 rad rotation along +Z
    double angle = 0.1;
    VMD baseX(cos(angle), sin(angle), 0.0);
    VMD baseY(-sin(angle), cos(angle), 0.0);
    if (FlipYBasis)
    {
      baseY = baseY * -1.;
      // Adjust origin to be at the upper left corner of the square
      origin = origin + VMD(-sin(angle), cos(angle), 0) * 10.0;
    }
    VMD baseZ(0.0, 0.0, 1.0);
    // Make a bad (i.e. non-orthogonal) input, to get it fixed.
    if (ForceOrthogonal)
    {
      baseY = VMD(0.0, 1.0, 0);
      baseZ = VMD(0.5, 0.5, 0.5);
    }

    AnalysisDataService::Instance().addOrReplace("BinMDTest_ws", in_ws);
    if (false)
    {
      // Save to NXS file for testing
      FrameworkManager::Instance().exec("SaveMD", 4,
          "InputWorkspace", "BinMDTest_ws",
          "Filename", "BinMDTest_ws_rotated.nxs");
    }

    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "BinMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AxisAligned", false));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVector0", "OutX,m," + baseX.toString(",")  ));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVector1", "OutY,m," + baseY.toString(",")  ));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVector2", "OutZ,m," + baseZ.toString(",")  ));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVector3", ""));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Translation", origin.toString(",") ));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ForceOrthogonal", ForceOrthogonal ));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ImplicitFunctionXML",""));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IterateEvents", IterateEvents));

    std::vector<int> OutputBins;    OutputBins.push_back(binsX);    OutputBins.push_back(binsY);    OutputBins.push_back(binsZ);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputBins", OutputBins));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputExtents", "0,10, 0,10, 0,10"));

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinMDTest_ws"));

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )

    TS_ASSERT( alg.isExecuted() );

    MDHistoWorkspace_sptr out ;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<MDHistoWorkspace>(
        AnalysisDataService::Instance().retrieve("BinMDTest_ws")); )
    TS_ASSERT(out);
    if(!out) return;

    // Took 6x6x6 bins in the middle of the box
    TS_ASSERT_EQUALS(out->getNPoints(), expected_numBins);
    // Every box has a single event summed into it, so 1.0 weight
    for (size_t i=0; i < out->getNPoints(); i++)
    {
      // Nothing rejected
      TS_ASSERT_DELTA(out->getSignalAt(i), expected_signal, 1e-5);
      TS_ASSERT_DELTA(out->getErrorAt(i), std::sqrt(expected_signal), 1e-5);
    }

    // check basis vectors
    TS_ASSERT_EQUALS( out->getBasisVector(0), baseX);
    if (!ForceOrthogonal)
    { TS_ASSERT_EQUALS( out->getBasisVector(1), baseY);
      TS_ASSERT_EQUALS( out->getBasisVector(2), baseZ); }

    const CoordTransform * ctFrom = out->getTransformFromOriginal();
    TS_ASSERT(ctFrom);
    const CoordTransform * ctTo = out->getTransformToOriginal();
    TS_ASSERT(ctTo);
    if (!ctTo) return;

    // Round-trip transform
    coord_t originalPoint[3] = {1.0, 2.0, 3.0};
    coord_t * transformedPoint = new coord_t[3];
    coord_t * originalBack = new coord_t[3];
    ctFrom->apply(originalPoint, transformedPoint);
    ctTo->apply(transformedPoint, originalBack);
    for (size_t d=0; d<3; d++)
    { TS_ASSERT_DELTA( originalPoint[d], originalBack[d], 1e-5); }

    AnalysisDataService::Instance().remove("BinMDTest_ws");
  }


  void test_exec_with_transform()
  {
    do_test_transform(10, 10, 10,
        1.0 /*signal*/, 1000 /*# of bins*/, true /*IterateEvents*/,
        false /* Dont force orthogonal */);
  }

  void test_exec_with_transform_unevenSizes()
  {
    do_test_transform(5, 10, 2,
        10*1.0 /*signal*/, 100 /*# of bins*/, true /*IterateEvents*/,
        false /* Dont force orthogonal */ );
  }

  void test_exec_with_transform_ForceOrthogonal()
  {
    do_test_transform(5, 10, 2,
        10*1.0 /*signal*/, 100 /*# of bins*/, true /*IterateEvents*/,
        true /* Do force orthogonal */ );
  }

  /** Change the handedness of the basis vectors by flipping the Y vector */
  void test_exec_with_transform_flipping_Y_basis()
  {
    do_test_transform(10, 10, 10,
        1.0 /*signal*/, 1000 /*# of bins*/, true /*IterateEvents*/,
        false /* Dont force orthogonal */,
        true /* Flip sign of Y basis vector*/);
  }



  //---------------------------------------------------------------------------------------------
  /** Check that two MDHistos have the same values .
   *
   * @param binned1Name :: original, binned direct from MDEW
   * @param binned2Name :: binned from a MDHisto
   * @param origWS :: both should have this as its originalWorkspace
   * @return binned2 shared pointer
   */
  MDHistoWorkspace_sptr do_compare_histo(std::string binned1Name, std::string binned2Name, std::string origWS)
  {
    MDHistoWorkspace_sptr binned1 = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>(binned1Name);
    MDHistoWorkspace_sptr binned2 = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>(binned2Name);
    TS_ASSERT_EQUALS( binned1->getOriginalWorkspace()->getName(), origWS);
    TS_ASSERT_EQUALS( binned2->getOriginalWorkspace()->getName(), origWS);
    TS_ASSERT(binned2);
    size_t numErrors = 0;
    for (size_t i=0; i < binned1->getNPoints(); i++)
    {
      double diff = std::abs(binned1->getSignalAt(i) - binned2->getSignalAt(i));
      if (diff > 1e-6) numErrors++;
      TS_ASSERT_DELTA( binned1->getSignalAt(i), binned2->getSignalAt(i), 1e-5);
    }
    TS_ASSERT_EQUALS( numErrors, 0);
    return binned2;
  }

  /** Common setup for double-binning tests */
  void do_prepare_comparison()
  {
    AnalysisDataService::Instance().remove("mdew");
    AnalysisDataService::Instance().remove("binned0");
    AnalysisDataService::Instance().remove("binned1");
    AnalysisDataService::Instance().remove("binned2");

    // ---- Start with empty MDEW ----
    FrameworkManager::Instance().exec("CreateMDWorkspace", 16,
        "Dimensions", "2",
        "Extents", "-10,10,-10,10",
        "Names", "x,y",
        "Units", "m,m",
        "SplitInto", "4",
        "SplitThreshold", "100",
        "MaxRecursionDepth", "20",
        "OutputWorkspace", "mdew");

    // Give fake uniform data
    FrameworkManager::Instance().exec("FakeMDEventData", 6,
        "InputWorkspace", "mdew",
        "UniformParams", "1000",
        "RandomSeed", "1234");
  }


  //---------------------------------------------------------------------------------------------
  /** Bin a MDHistoWorkspace that was itself binned from a MDEW, axis-aligned */
  void test_exec_Aligned_then_nonAligned()
  {
    do_prepare_comparison();
    // Bin aligned to original. Coordinates remain the same
    FrameworkManager::Instance().exec("BinMD", 10,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned0",
        "AxisAligned", "1",
        "AlignedDim0", "x, -10, 10, 10",
        "AlignedDim1", "y, -10, 10, 10");

    // Bin, non-axis-aligned, with translation
    FrameworkManager::Instance().exec("BinMD", 18,
        "InputWorkspace", "binned0",
        "OutputWorkspace", "binned1",
        "AxisAligned", "0",
        "BasisVector0", "rx,m, 1.0,0.0",
        "BasisVector1", "ry,m, 0.0,1.0",
        "ForceOrthogonal", "1",
        "Translation", "-10, -10",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");

    MDHistoWorkspace_sptr binned1 = do_compare_histo("binned0", "binned1", "mdew");

    // Intermediate workspace (the MDHisto)
    TS_ASSERT_EQUALS( binned1->numOriginalWorkspaces(), 2);
    TS_ASSERT_EQUALS( binned1->getOriginalWorkspace(1)->name(), "binned0");
    // Transforms to/from the INTERMEDIATE workspace exist
    CoordTransform * toIntermediate = binned1->getTransformToOriginal(1);
    CoordTransform * fromIntermediate = binned1->getTransformFromOriginal(1);
    TS_ASSERT( toIntermediate);
    TS_ASSERT( fromIntermediate );

    // Transform from the BINNED to the INTERMEDIATE and check.
    VMD binnedPos(0.1, 0.2);
    VMD intermediatePos = toIntermediate->applyVMD(binnedPos);
    TS_ASSERT_DELTA( intermediatePos[0], -9.9, 1e-5);
    TS_ASSERT_DELTA( intermediatePos[1], -9.8, 1e-5);

  }


  //---------------------------------------------------------------------------------------------
  /** Bin a MDHistoWorkspace that was itself binned from a MDEW, axis-aligned, but swapping axes around */
  void test_exec_AlignedSwappingAxes_then_nonAligned()
  {
    do_prepare_comparison();
    // Bin aligned to original.
    FrameworkManager::Instance().exec("BinMD", 10,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned0",
        "AxisAligned", "1",
        "AlignedDim0", "y, -10, 10, 10",
        "AlignedDim1", "x, -10, 10, 10");

    // binned0.x is mdew.y
    // binned0.y is mdew.x
    // Bin, non-axis-aligned, with translation
    FrameworkManager::Instance().exec("BinMD", 18,
        "InputWorkspace", "binned0",
        "OutputWorkspace", "binned1",
        "AxisAligned", "0",
        "BasisVector0", "rx,m, 1.0,0.0",
        "BasisVector1", "ry,m, 0.0,1.0",
        "ForceOrthogonal", "1",
        "Translation", "-10, -5",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");

    // Get the final binned workspace
    MDHistoWorkspace_sptr binned1 = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>("binned1");

    // Intermediate workspace (the MDHisto) is binned0
    TS_ASSERT_EQUALS( binned1->numOriginalWorkspaces(), 2);
    TS_ASSERT_EQUALS( binned1->getOriginalWorkspace(1)->name(), "binned0");
    // Transforms to/from the INTERMEDIATE workspace exist
    CoordTransform * toIntermediate = binned1->getTransformToOriginal(1);
    CoordTransform * fromIntermediate = binned1->getTransformFromOriginal(1);
    TS_ASSERT( toIntermediate);
    TS_ASSERT( fromIntermediate );

    // Transforms to/from the REALLY ORIGINAL workspace exist
    CoordTransform * toOriginal = binned1->getTransformToOriginal(0);
    CoordTransform * fromOriginal = binned1->getTransformFromOriginal(0);
    TS_ASSERT( toOriginal);
    TS_ASSERT( fromOriginal );

    // Transform from the BINNED coordinates.
    // in binned1 : (0.1,   0.2)
    // in binned0 : (-9.9, -4.8)
    // in mdew :    (-4.8, -9.9)
    VMD binned1Pos(0.1, 0.2);
    VMD intermediatePos = toIntermediate->applyVMD(binned1Pos);
    VMD originalPos = toOriginal->applyVMD(binned1Pos);
    TS_ASSERT_DELTA( intermediatePos[0], -9.9, 1e-5);
    TS_ASSERT_DELTA( intermediatePos[1], -4.8, 1e-5);
    TS_ASSERT_DELTA( originalPos[0], -4.8, 1e-5);
    TS_ASSERT_DELTA( originalPos[1], -9.9, 1e-5);

    // Now check the reverse transforms
    VMD originalToBinned = fromOriginal->applyVMD( VMD(-4.8, -9.9) );
    TS_ASSERT_DELTA( originalToBinned[0], 0.1, 1e-5);
    TS_ASSERT_DELTA( originalToBinned[1], 0.2, 1e-5);

    VMD intermediateToBinned = fromIntermediate->applyVMD( VMD(-9.9, -4.8) );
    TS_ASSERT_DELTA( intermediateToBinned[0], 0.1, 1e-5);
    TS_ASSERT_DELTA( intermediateToBinned[1], 0.2, 1e-5);
  }


  //---------------------------------------------------------------------------------------------
  /** Bin a MDHistoWorkspace that was itself binned from a MDEW, axis-aligned, but swapping axes around */
  void test_exec_AlignedSwappingAxes_then_nonAligned_3D()
  {
    AnalysisDataService::Instance().remove("mdew3d");

    FrameworkManager::Instance().exec("CreateMDWorkspace", 16,
        "Dimensions", "3",
        "Extents", "-10,10,-10,10,-10,10",
        "Names", "A,B,C",
        "Units", "m,m,m",
        "SplitInto", "4",
        "SplitThreshold", "100",
        "MaxRecursionDepth", "20",
        "OutputWorkspace", "mdew3d");

    FrameworkManager::Instance().exec("BinMD", 12,
        "InputWorkspace", "mdew3d",
        "OutputWorkspace", "binned0",
        "AxisAligned", "1",
        "AlignedDim0", "B, -10, 10, 10",
        "AlignedDim1", "C, -10, 10, 10",
        "AlignedDim2", "A, -10, 10, 10"
        );

    FrameworkManager::Instance().exec("BinMD", 20,
        "InputWorkspace", "binned0",
        "OutputWorkspace", "binned1",
        "AxisAligned", "0",
        "BasisVector0", "rx,m, 1.0,0.0,0.0",
        "BasisVector1", "ry,m, 0.0,1.0,0.0",
        "BasisVector2", "rz,m, 0.0,0.0,1.0",
        "ForceOrthogonal", "1",
        "Translation", "-10, -5, -3",
        "OutputExtents", "0,20, 0,20, 0,20",
        "OutputBins", "10,10,10");

    // Get the final binned workspace
    MDHistoWorkspace_sptr binned1 = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>("binned1");

    // Intermediate workspace (the MDHisto) is binned0
    TS_ASSERT_EQUALS( binned1->numOriginalWorkspaces(), 2);
    TS_ASSERT_EQUALS( binned1->getOriginalWorkspace(1)->name(), "binned0");
    // Transforms to/from the INTERMEDIATE workspace exist
    CoordTransform * toIntermediate = binned1->getTransformToOriginal(1);
    CoordTransform * fromIntermediate = binned1->getTransformFromOriginal(1);
    TS_ASSERT( toIntermediate);
    TS_ASSERT( fromIntermediate );

    // Transforms to/from the REALLY ORIGINAL workspace exist
    CoordTransform * toOriginal = binned1->getTransformToOriginal(0);
    CoordTransform * fromOriginal = binned1->getTransformFromOriginal(0);
    TS_ASSERT( toOriginal);
    TS_ASSERT( fromOriginal );

    // Transform from the BINNED coordinates.
    // in binned1 : (0.1,   0.2, 0.3)
    // in binned0 : (-9.9, -4.8, -2.7)
    // in mdew3d :  (-2.7, -9.9, -4.8)
    VMD binned1Pos(0.1, 0.2, 0.3);
    VMD intermediatePos = toIntermediate->applyVMD(binned1Pos);
    VMD originalPos = toOriginal->applyVMD(binned1Pos);
    TS_ASSERT_DELTA( intermediatePos[0], -9.9, 1e-5);
    TS_ASSERT_DELTA( intermediatePos[1], -4.8, 1e-5);
    TS_ASSERT_DELTA( intermediatePos[2], -2.7, 1e-5);
    TS_ASSERT_DELTA( originalPos[0], -2.7, 1e-5);
    TS_ASSERT_DELTA( originalPos[1], -9.9, 1e-5);
    TS_ASSERT_DELTA( originalPos[2], -4.8, 1e-5);

//
//    // Now check the reverse transforms
//    VMD originalToBinned = fromOriginal->applyVMD( VMD(-4.8, -9.9) );
//    TS_ASSERT_DELTA( originalToBinned[0], 0.1, 1e-5);
//    TS_ASSERT_DELTA( originalToBinned[1], 0.2, 1e-5);
//
//    VMD intermediateToBinned = fromIntermediate->applyVMD( VMD(-9.9, -4.8) );
//    TS_ASSERT_DELTA( intermediateToBinned[0], 0.1, 1e-5);
//    TS_ASSERT_DELTA( intermediateToBinned[1], 0.2, 1e-5);
  }

  //---------------------------------------------------------------------------------------------
  /** Bin a MDHistoWorkspace that was itself binned from a MDEW, not axis-aligned */
  void test_exec_nonAligned_then_nonAligned_rotation()
  {
    do_prepare_comparison();

    // Bin NOT aligned to original, with translation
    FrameworkManager::Instance().exec("BinMD", 18,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned0",
        "AxisAligned", "0",
        "BasisVector0", "rx,m, 1.0, 0.0",
        "BasisVector1", "ry,m, 0.0, 1.0",
        "ForceOrthogonal", "1",
        "Translation", "-10, -10",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");

    // Bin with some rotation (10 degrees)
    FrameworkManager::Instance().exec("BinMD", 18,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned1",
        "AxisAligned", "0",
        "BasisVector0", "rx,m, 0.98, 0.17",
        "BasisVector1", "ry,m, -.17, 0.98",
        "ForceOrthogonal", "1",
        "Translation", "-10, -10",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");
    // Bin the binned output with the opposite rotation
    FrameworkManager::Instance().exec("BinMD", 18,
        "InputWorkspace", "binned1",
        "OutputWorkspace", "binned2",
        "AxisAligned", "0",
        "BasisVector0", "rrx,m, 0.98, -.17",
        "BasisVector1", "rry,m, 0.17, 0.98",
        "ForceOrthogonal", "1",
        "Translation", "0, 0",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");
    // Check they are the same
    MDHistoWorkspace_sptr binned2 = do_compare_histo("binned0", "binned2", "mdew");

    // Intermediate workspace (the MDHisto)
    TS_ASSERT_EQUALS( binned2->numOriginalWorkspaces(), 2);
    TS_ASSERT_EQUALS( binned2->getOriginalWorkspace(1)->name(), "binned1");
    // Transforms to/from the INTERMEDIATE workspace exist
    TS_ASSERT( binned2->getTransformToOriginal(1) );
    TS_ASSERT( binned2->getTransformFromOriginal(1) );

  }


  //---------------------------------------------------------------------------------------------
  /** Bin a MDHistoWorkspace that was itself binned from a MDEW, not axis-aligned, with translation */
  void test_exec_nonAligned_then_nonAligned_translation()
  {
    do_prepare_comparison();

    // Bin aligned to original
    FrameworkManager::Instance().exec("BinMD", 18,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned0",
        "AxisAligned", "0",
        "BasisVector0", "rx,m, 1.0, 0.0",
        "BasisVector1", "ry,m, 0.0, 1.0",
        "ForceOrthogonal", "1",
        "Translation", "-10, -10",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");

    // Bin with a translation. -10,-10 in MDEW becomes 0,0 in binned1
    FrameworkManager::Instance().exec("BinMD", 18,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned1",
        "AxisAligned", "0",
        "BasisVector0", "rx,m, 1.0, 0.0",
        "BasisVector1", "ry,m, 0.0, 1.0",
        "ForceOrthogonal", "1",
        "Translation", "-10, -10",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");

    // Bin the binned output with the opposite translation
    FrameworkManager::Instance().exec("BinMD", 18,
        "InputWorkspace", "binned1",
        "OutputWorkspace", "binned2",
        "AxisAligned", "0",
        "BasisVector0", "rrx,m, 1.0, 0.0",
        "BasisVector1", "rry,m, 0.0, 1.0",
        "ForceOrthogonal", "1",
        "Translation", "0, 0",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");

    // Check they are the same
    do_compare_histo("binned0", "binned2", "mdew");
  }

  //---------------------------------------------------------------------------------------------
  /** Can't do Axis-Aligned on a MDHisto because the transformation gets too annoying. */
  void test_exec_Aligned_onMDHisto_fails()
  {
    do_prepare_comparison();

    // Bin NOT aligned to original, translated. Coordinates change.
    FrameworkManager::Instance().exec("BinMD", 18,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned0",
        "AxisAligned", "0",
        "BasisVector0", "rx,m, 1.0, 0.0",
        "BasisVector1", "ry,m, 0.0, 1.0",
        "ForceOrthogonal", "1",
        "Translation", "-10, -10",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");

    // Bin aligned to binned0. This is not allowed!
    IAlgorithm_sptr alg = FrameworkManager::Instance().exec("BinMD", 10,
        "InputWorkspace", "binned0",
        "OutputWorkspace", "binned1",
        "AxisAligned", "1",
        "AlignedDim0", "rx, 0, 20, 10",
        "AlignedDim1", "ry, 0, 20, 10");
    TS_ASSERT( !alg->isExecuted() );
  }


  //---------------------------------------------------------------------------------------------
  /** A = a MDEventWorkspace
   *  B = binned from A with translation and scaling
   *  C = binned from B with translation and scaling
   *  */
  void test_exec_nonAligned_then_nonAligned_translation_scaling()
  {
    do_prepare_comparison();

    // Make the reference bin, which is all space (-10 to 10) with 10 bins
    FrameworkManager::Instance().exec("BinMD", 20,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "reference",
        "AxisAligned", "0",
        "BasisVector0", "tx,m, 1.0, 0.0",
        "BasisVector1", "ty,m, 0.0, 1.0",
        "NormalizeBasisVectors", "0",
        "ForceOrthogonal", "0",
        "Translation", "-10, -10",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");

    // Bin to workspace B. Have translation and scaling
    FrameworkManager::Instance().exec("BinMD", 20,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "B",
        "AxisAligned", "0",
        "BasisVector0", "tx, m, 2.0, 0.0",
        "BasisVector1", "ty, m, 0.0, 2.0",
        "NormalizeBasisVectors", "0",
        "ForceOrthogonal", "0",
        "Translation", "-2, -2",
        "OutputExtents", "-4,6, -4,6", /* The extents are in the scaled OUTPUT dimensions */
        "OutputBins", "10,10");

    // Check that B turns out to be the same as "Reference"
    do_compare_histo("reference", "B", "mdew");

    // Bin the binned output with more translation and scaling,
    // but it still ends up binning A from (-10 to 10) with 10 bins.
    FrameworkManager::Instance().exec("BinMD", 20,
        "InputWorkspace", "B",
        "OutputWorkspace", "C",
        "AxisAligned", "0",
        "BasisVector0", "ttx,m, 2.0, 0.0", /* size 2 in B = size 4 in A */
        "BasisVector1", "tty,m, 0.0, 2.0",
        "NormalizeBasisVectors", "0",
        "ForceOrthogonal", "0",
        "Translation", "-1, -1", /* coords in B = (-4,-4) in A */
        "OutputExtents", "-1.5, 3.5, -1.5, 3.5", /* size of 5 in C = size of 10 in B = size of 20 in A */
        "OutputBins", "10,10");

    // Finally, C maps back onto A (mdew) binned as reference
    do_compare_histo("reference", "C", "mdew");

    IMDWorkspace_sptr C = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("C");
    TS_ASSERT(C);

    VMD out;
    // Check the mapping of coordinates from C to B
    CoordTransform * transf_C_to_B = C->getTransformToOriginal(1);
    out = transf_C_to_B->applyVMD(VMD(-1.5, -1.5));
    TS_ASSERT_EQUALS( out, VMD(-4, -4) );

    // And this is the mapping to the A workspace
    CoordTransform * transf_C_to_A = C->getTransformToOriginal(0);
    out = transf_C_to_A->applyVMD(VMD(-1.5, -1.5));
    TS_ASSERT_EQUALS( out, VMD(-10, -10) );
  }


  //---------------------------------------------------------------------------------------------
  /** Modify a MDHistoWorkspace with a binary operation.
   *  */
  void test_FailsIfYouModify_a_MDHistoWorkspace()
  {
    FrameworkManager::Instance().exec("BinMD", 18,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned0",
        "AxisAligned", "0",
        "BasisVector0", "rx,m, 1.0, 0.0",
        "BasisVector1", "ry,m, 0.0, 1.0",
        "ForceOrthogonal", "1",
        "Translation", "-10, -10",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");

    FrameworkManager::Instance().exec("PlusMD", 6,
        "LHSWorkspace", "binned0",
        "RHSWorkspace", "binned0",
        "OutputWorkspace", "binned0");

    IAlgorithm_sptr alg = FrameworkManager::Instance().exec("BinMD", 18,
        "InputWorkspace", "binned0",
        "OutputWorkspace", "binned1",
        "AxisAligned", "0",
        "BasisVector0", "rx,m, 1.0, 0.0",
        "BasisVector1", "ry,m, 0.0, 1.0",
        "ForceOrthogonal", "1",
        "Translation", "-10, -10",
        "OutputExtents", "0,20, 0,20",
        "OutputBins", "10,10");
    TSM_ASSERT( "Algorithm threw an error, as expected", !alg->isExecuted())
  }
};


class BinMDTestPerformance : public CxxTest::TestSuite
{
public:
  MDEventWorkspace3Lean::sptr in_ws;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinMDTestPerformance *createSuite() { return new BinMDTestPerformance(); }
  static void destroySuite( BinMDTestPerformance *suite ) { delete suite; }

  BinMDTestPerformance()
  {
    in_ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
    in_ws->getBoxController()->setSplitThreshold(2000);
    in_ws->splitAllIfNeeded(NULL);
    AnalysisDataService::Instance().addOrReplace("BinMDTest_ws", in_ws);
    FrameworkManager::Instance().exec("FakeMDEventData", 4,
        "InputWorkspace", "BinMDTest_ws",
        "UniformParams", "1000000");
    // 1 million random points
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000*1000);
    TS_ASSERT_EQUALS( in_ws->getBoxController()->getMaxId(), 1001 );
  }

  ~BinMDTestPerformance()
  {
    AnalysisDataService::Instance().remove("BinMDTest_ws");
  }

  void do_test(std::string binParams, bool IterateEvents)
  {
    BinMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "BinMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim0", "Axis0," + binParams));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim1", "Axis1," + binParams));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim2", "Axis2," + binParams));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim3", ""));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IterateEvents", IterateEvents));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinMDTest_ws_histo"));
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() );
  }


  void test_3D_60cube_IterateEvents()
  {
    for (size_t i=0; i<1; i++)
      do_test("2.0,8.0, 60", true);
  }

  void test_3D_tinyRegion_60cube_IterateEvents()
  {
    for (size_t i=0; i<1; i++)
      do_test("5.3,5.4, 60", true);
  }

  void test_3D_1cube_IterateEvents()
  {
    for (size_t i=0; i<1; i++)
      do_test("2.0,8.0, 1", true);
  }
private: 

};


#endif /* MANTID_MDALGORITHMS_BINTOMDHISTOWORKSPACETEST_H_ */

