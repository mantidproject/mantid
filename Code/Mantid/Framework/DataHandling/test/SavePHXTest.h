#ifndef SAVEPHXTEST_H_
#define SAVEPHXTEST_H_

#include <cxxtest/TestSuite.h>


#include "MantidDataHandling/SavePHX.h"
// to generate test workspaces
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidDataHandling/LoadInstrument.h"



using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using Mantid::Geometry::Instrument;

static const int NHIST = 3;
static const int THEMASKED = 2;

class SavePHXTest: public CxxTest::TestSuite
{
private:
  Mantid::DataHandling::SavePHX phxSaver;
  std::string TestOutputFile;
  std::string WSName;
public:
  static SavePHXTest *createSuite() { return new SavePHXTest(); }
  static void destroySuite(SavePHXTest *suite) { delete suite; }

  void testAlgorithmName()
  {
    TS_ASSERT_EQUALS(phxSaver.name(), "SavePHX");
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(phxSaver.initialize());
    TS_ASSERT(phxSaver.isInitialized());
  }
  void testExec()
  {
   // Create a small test workspace
    WSName = "savePHXTest_input";
    API::MatrixWorkspace_const_sptr input = makeWorkspace(WSName);

    TS_ASSERT_THROWS_NOTHING( phxSaver.setPropertyValue("InputWorkspace", WSName) );
    TestOutputFile = std::string("testPHX.phx");
    TS_ASSERT_THROWS_NOTHING( phxSaver.setPropertyValue("Filename",TestOutputFile) );
    TestOutputFile = phxSaver.getPropertyValue("Filename");//get absolute path
     
    TS_ASSERT_THROWS_NOTHING(phxSaver.execute());
    TS_ASSERT(phxSaver.isExecuted());
  }

  void testResutlts(){
      std::vector<std::string> pattern(5),count(5);
      std::string result;
      pattern[0]=" 3";
      pattern[1]=" 1.000	 0 		170.565 	0.000 	0.792 	5.725 		1";
      pattern[2]=" 1.000	 0 		169.565 	0.000 	0.790 	5.725 		2";
      pattern[3]=" 1.000	 0 		168.565 	0.000 	0.787 	5.725 		3";
      count[0]=" 0 ";count[1]=" 1 ";count[2]=" 2 ";count[3]=" 3 ";

      std::ifstream testFile;
      testFile.open(TestOutputFile.c_str());
      TSM_ASSERT(" Can not open test file produced by algorithm phxSaver",testFile.is_open());
      int ic(0);
      while(ic<5){
          std::getline(testFile,result);
          if(testFile.eof())break;

          TSM_ASSERT_EQUALS("wrong string N "+count[ic]+" obtained from file",pattern[ic],result);
          ic++;
      }
      TSM_ASSERT_EQUALS(" Expecting 4 rows ascii file, but got different number of rows",4,ic);
      testFile.close();
  
  }
private:
    ~SavePHXTest(){
        // delete test ws from ds after the test ends
        AnalysisDataService::Instance().remove(WSName);
        // delete test output file from the hdd;
        unlink(TestOutputFile.c_str());
    }

    MatrixWorkspace_sptr makeWorkspace(const std::string & input) {
    // all the Y values in this new workspace are set to DEFAU_Y, which currently = 2
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(NHIST,10,1.0);
    return setUpWorkspace(input, inputWS);
    }
 
    MatrixWorkspace_sptr setUpWorkspace(const std::string & input, MatrixWorkspace_sptr inputWS)
    {
        inputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("DeltaE");

    // the following is largely about associating detectors with the workspace
     int forSpecDetMap[NHIST];
     for (int j = 0; j < NHIST; ++j)
     {
      // Just set the spectrum number to match the index
          inputWS->getAxis(1)->spectraNo(j) = j+1;
          forSpecDetMap[j] = j+1;
     }
     // we do not need to deal with analysisi data service here in test to avoid holding the workspace there after the test 
     AnalysisDataService::Instance().add(input,inputWS);

    // Load the instrument data
      Mantid::DataHandling::LoadInstrument loader;
      loader.initialize();
      // Path to test input file assumes Test directory checked out from SVN
      std::string inputFile = "INES_Definition.xml";
      loader.setPropertyValue("Filename", inputFile);
      loader.setPropertyValue("Workspace", input);
      loader.execute();

      inputWS->replaceSpectraMap(new SpectraDetectorMap(forSpecDetMap, forSpecDetMap, NHIST));

      // mask the detector
      Geometry::ParameterMap* m_Pmap = &(inputWS->instrumentParameters());
      boost::shared_ptr<Instrument> instru = inputWS->getBaseInstrument();
      const Geometry::Detector* toMask =
      dynamic_cast<const Geometry::Detector*>( instru->getDetector(THEMASKED).get() );
      TS_ASSERT(toMask);
      m_Pmap->addBool(toMask, "masked", true);

    // required to get it passed the algorthms validator
      inputWS->isDistribution(true);

      return inputWS;
  }
 

};

#endif /*SAVEPHXTEST_H_*/
