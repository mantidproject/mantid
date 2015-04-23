#ifndef LOADPARAMETERFILETEST_H_
#define LOADPARAMETERFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"

#include <vector>
#include <iostream>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class LoadParameterFileTest : public CxxTest::TestSuite
{
public:

  void testExecIDF_for_unit_testing2() // IDF stands for Instrument Definition File
  {

    MatrixWorkspace_sptr output;

    // Create workspace wsName
    load_IDF2(); 
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    const ParameterMap& paramMap = output->instrumentParameters();
    std::string descr = paramMap.getDescription("nickel-holder","fjols");
    TS_ASSERT_EQUALS(descr,"test fjols description.");


    // load in additional parameters
    auto pLoaderPF = FrameworkManager::Instance().createAlgorithm("LoadParameterFile");

    TS_ASSERT_THROWS_NOTHING(pLoaderPF->initialize());
    pLoaderPF->setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING2_paramFile.xml");
    pLoaderPF->setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(pLoaderPF->execute());
    TS_ASSERT( pLoaderPF->isExecuted() );


    // Get back the saved workspace
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    boost::shared_ptr<const Instrument> i = output->getInstrument();
    boost::shared_ptr<const IDetector> ptrDet = i->getDetector(1008);
    TS_ASSERT_EQUALS( ptrDet->getID(), 1008);
    TS_ASSERT_EQUALS( ptrDet->getName(), "combined translation6");
    Parameter_sptr param = paramMap.get(&(*ptrDet), "fjols");
    TS_ASSERT_DELTA( param->value<double>(), 20.0, 0.0001);

    param = paramMap.get(&(*ptrDet), "nedtur");
    TS_ASSERT_DELTA( param->value<double>(), 77.0, 0.0001);
    param = paramMap.get(&(*ptrDet), "fjols-test-paramfile");
    TS_ASSERT_DELTA( param->value<double>(), 50.0, 0.0001);
    descr = param->getDescription();
    TS_ASSERT_EQUALS(descr,"test description. Full test description.");


    ptrDet = i->getDetector(1301);
    TS_ASSERT_EQUALS( ptrDet->getID(), 1301);
    TS_ASSERT_EQUALS( ptrDet->getName(), "pixel");
    param = paramMap.get(ptrDet.get(), "testDouble");
    TS_ASSERT_DELTA( param->value<double>(), 25.0, 0.0001);
    TS_ASSERT_EQUALS(paramMap.getString(ptrDet.get(), "testString"), "hello world");

    param = paramMap.get(ptrDet.get(), "testString");
    TS_ASSERT_EQUALS(param->getTooltip(),"its test hello word.");
    TS_ASSERT_EQUALS(param->getDescription(),"its test hello word.");
    TS_ASSERT_EQUALS(paramMap.getDescription("pixel","testString"),"its test hello word.");


    std::vector<double> dummy = paramMap.getDouble("nickel-holder", "klovn");
    TS_ASSERT_DELTA( dummy[0], 1.0, 0.0001);
    dummy = paramMap.getDouble("nickel-holder", "pos");
    TS_ASSERT_EQUALS (dummy.size(), 0);
    dummy = paramMap.getDouble("nickel-holder", "rot");
    TS_ASSERT_EQUALS (dummy.size(), 0);
    dummy = paramMap.getDouble("nickel-holder", "taabe");
    TS_ASSERT_DELTA (dummy[0], 200.0, 0.0001);
    dummy = paramMap.getDouble("nickel-holder", "mistake");
    TS_ASSERT_EQUALS (dummy.size(), 0);
    
    dummy = paramMap.getDouble("nickel-holder", "fjols-test-paramfile");
    TS_ASSERT_DELTA (dummy[0], 2000.0, 0.0001);

  AnalysisDataService::Instance().remove(wsName);

  }

  void testExec_withIDFString()  // Test use of string instead of file
  {

    // Create workspace
     load_IDF2();

    // Define parameter XML string
    std::string parameterXML = 
      "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
      "<parameter-file instrument=\"IDF_for_UNIT_TESTING2\" valid-from=\"blah...\">"
      "	<component-link name=\"nickel-holder\">"
      "  <parameter name=\"fjols-test-paramfile\"> <value val=\"2010.0\" /> </parameter>"
      " </component-link>"
      " <component-link name=\"IDF_for_UNIT_TESTING2.xml/combined translation6\" >"
      "  <parameter name=\"fjols-test-paramfile\"> <value val=\"52.0\" />"
      "  <description is = \"test description2. Full test description2.\"/>"
      "</parameter>"
      "	</component-link>"
      " <component-link id=\"1301\" >"
      "  <parameter name=\"testDouble\"> <value val=\"27.0\" /> </parameter>"
      "  <parameter name=\"testString\" type=\"string\"> <value val=\"goodbye world\" />"
      "  <description is = \"its test goodbye world.\"/>"
      "</parameter>"
      "	</component-link>"
      "</parameter-file>";

    // load in additional parameters
    auto pLoaderPF = FrameworkManager::Instance().createAlgorithm("LoadParameterFile");


    TS_ASSERT_THROWS_NOTHING(pLoaderPF->initialize());
    pLoaderPF->setPropertyValue("ParameterXML", parameterXML);
    pLoaderPF->setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(pLoaderPF->execute());
    TS_ASSERT( pLoaderPF->isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    const ParameterMap& paramMap = output->instrumentParameters();
    boost::shared_ptr<const Instrument> i = output->getInstrument();
    boost::shared_ptr<const IDetector> ptrDet = i->getDetector(1008);
    TS_ASSERT_EQUALS( ptrDet->getID(), 1008);
    TS_ASSERT_EQUALS( ptrDet->getName(), "combined translation6");
    Parameter_sptr param = paramMap.get(&(*ptrDet), "fjols");
    TS_ASSERT_DELTA( param->value<double>(), 20.0, 0.0001);
    param = paramMap.get(&(*ptrDet), "nedtur");
    TS_ASSERT_DELTA( param->value<double>(), 77.0, 0.0001);
    param = paramMap.get(&(*ptrDet), "fjols-test-paramfile");
    TS_ASSERT_DELTA( param->value<double>(), 52.0, 0.0001);
    std::string descr = param->getDescription();
    TS_ASSERT_EQUALS(descr,"test description2. Full test description2.");


    ptrDet = i->getDetector(1301);
    TS_ASSERT_EQUALS( ptrDet->getID(), 1301);
    TS_ASSERT_EQUALS( ptrDet->getName(), "pixel");
    param = paramMap.get(ptrDet.get(), "testDouble");
    TS_ASSERT_DELTA( param->value<double>(), 27.0, 0.0001);
    TS_ASSERT_EQUALS( paramMap.getString(ptrDet.get(), "testString"), "goodbye world");

    param = paramMap.get(ptrDet.get(), "testString");
    TS_ASSERT_EQUALS(param->getTooltip(),"its test goodbye world.");
    TS_ASSERT_EQUALS(param->getDescription(),"its test goodbye world.");
    TS_ASSERT_EQUALS(paramMap.getDescription("pixel","testString"),"its test goodbye world.");


    std::vector<double> dummy = paramMap.getDouble("nickel-holder", "klovn");
    TS_ASSERT_DELTA( dummy[0], 1.0, 0.0001);
    dummy = paramMap.getDouble("nickel-holder", "pos");
    TS_ASSERT_EQUALS (dummy.size(), 0);
    dummy = paramMap.getDouble("nickel-holder", "rot");
    TS_ASSERT_EQUALS (dummy.size(), 0);
    dummy = paramMap.getDouble("nickel-holder", "taabe");
    TS_ASSERT_DELTA (dummy[0], 200.0, 0.0001);
    dummy = paramMap.getDouble("nickel-holder", "mistake");
    TS_ASSERT_EQUALS (dummy.size(), 0);
    dummy = paramMap.getDouble("nickel-holder", "fjols-test-paramfile");
    TS_ASSERT_DELTA (dummy[0], 2010.0, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_failure_if_no_file_or_string()
  {

     // Create workspace
     load_IDF2();

     // Run algorithm without file or string properties set
    auto pLoaderPF = FrameworkManager::Instance().createAlgorithm("LoadParameterFile");
    TS_ASSERT_THROWS_NOTHING(pLoaderPF->initialize());
    pLoaderPF->setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(pLoaderPF->execute());
    TS_ASSERT( ! pLoaderPF->execute() )
  }

  void load_IDF2()
  {
    auto pLoadInstrument = FrameworkManager::Instance().createAlgorithm("LoadInstrument");

    TS_ASSERT_THROWS_NOTHING(pLoadInstrument->initialize());

    //create a workspace with some sample data
    wsName = "LoadParameterFileTestIDF2";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Path to test input file assumes Test directory checked out from git
    pLoadInstrument->setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING2.xml");
    //inputFile = loaderIDF2.getPropertyValue("Filename");
    pLoadInstrument->setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(pLoadInstrument->execute());
    TS_ASSERT( pLoadInstrument->isExecuted() );

  }


private:

  std::string inputFile;
  std::string wsName;

};

#endif /*LOADPARAMETERFILETEST_H_*/
