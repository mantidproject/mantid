#ifndef SAVENXSPETEST_H_
#define SAVENXSPETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidNexus/SaveNXSPE.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidDataHandling/LoadInstrument.h"
#include <Poco/File.h>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <numeric>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::NeXus;
using Mantid::Kernel::UnitFactory;
using Mantid::Geometry::Instrument;

static const double MASK_FLAG=-1e30;   // values here need to match what is in the SaveNXSPE.h file
static const double MASK_ERROR=0.0;

static const int NHIST = 3;
static const int THEMASKED = 2;
static const int DEFAU_Y = 2;

class SaveNXSPETest : public CxxTest::TestSuite
{
public:
  static SaveNXSPETest *createSuite() { return new SaveNXSPETest(); }
  static void destroySuite(SaveNXSPETest *suite) { delete suite; }

  SaveNXSPETest()
  {// the functioning of SaveNXSPE is affected by a function call in the FrameworkManager's constructor, creating the algorithm in this way ensures that function is executed
    saver = FrameworkManager::Instance().createAlgorithm("SaveNXSPE");
  }

  void testName()
  {
    TS_ASSERT_EQUALS( saver->name(), "SaveNXSPE" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( saver->version(), 1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( saver->category(), "DataHandling" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( saver->initialize() );
    TS_ASSERT( saver->isInitialized() );

    TS_ASSERT_EQUALS( static_cast<int>(saver->getProperties().size()), 5 );
  }

  void testExec()
  {
    // Create a small test workspace
    std::string WSName = "saveNXSPETest_input";
    MatrixWorkspace_const_sptr input = makeWorkspace(WSName);

    TS_ASSERT_THROWS_NOTHING( saver->setPropertyValue("InputWorkspace", WSName) );
    std::string outputFile("testNXSPE.nxspe");
    TS_ASSERT_THROWS_NOTHING( saver->setPropertyValue("Filename",outputFile) );
    outputFile = saver->getPropertyValue("Filename");//get absolute path

    TS_ASSERT_THROWS_NOTHING( saver->setProperty("Efixed", 0.0));
    TS_ASSERT_THROWS_NOTHING( saver->setProperty("psi", 0.0));
    TS_ASSERT_THROWS_NOTHING( saver->setProperty("ki_over_kf_scaling", true));


    TS_ASSERT_THROWS_NOTHING( saver->execute() );
    TS_ASSERT( saver->isExecuted() );

    TS_ASSERT( Poco::File(outputFile).exists() );

    // TODO: Test the files contents...

    AnalysisDataService::Instance().remove(WSName);
    if( Poco::File(outputFile).exists() )
      Poco::File(outputFile).remove();
  }

  void xtestThatOutputIsValidFromWorkspaceWithNumericAxis()
  {
    // Create a small test workspace
    std::string WSName = "saveNXSPETestB_input";
    MatrixWorkspace_sptr input = makeWorkspaceWithNumericAxis(WSName);

    TS_ASSERT_THROWS_NOTHING( saver->setPropertyValue("InputWorkspace", WSName) );
    const std::string outputFile("testNXSPE_Axis.nxspe");
    TS_ASSERT_THROWS_NOTHING( saver->setPropertyValue("Filename",outputFile) );
    saver->setRethrows(true);
    saver->execute();
    TS_ASSERT( saver->isExecuted() );

    TS_ASSERT( Poco::File(outputFile).exists() );
    if( Poco::File(outputFile).exists() )
    {
      Poco::File(outputFile).remove();
    }
  }

private:

  MatrixWorkspace_sptr makeWorkspace(const std::string & input)
  {
    // all the Y values in this new workspace are set to DEFAU_Y, which currently = 2
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(NHIST,10,1.0);
    return setUpWorkspace(input, inputWS);
  }

  MatrixWorkspace_sptr makeWorkspaceWithNumericAxis(const std::string & input)
  {
    // all the Y values in this new workspace are set to DEFAU_Y, which currently = 2
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(NHIST,10,1.0);
    inputWS = setUpWorkspace(input, inputWS);
    API::Axis * axisOne = inputWS->getAxis(1);
    API::NumericAxis *newAxisOne = new NumericAxis(axisOne->length());
    for( int i = 0; i < axisOne->length(); ++i)
    {
      newAxisOne->setValue(i, axisOne->operator()((i)));
    }
    // Set the units
    inputWS->replaceAxis(1, newAxisOne);
    inputWS->getAxis(1)->unit() = UnitFactory::Instance().create("Energy");
    inputWS->setYUnit("MyCaption");
    return inputWS;
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

    AnalysisDataService::Instance().add(input,inputWS);

    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "INES_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", input);
    loader.execute();

    inputWS->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, NHIST);

    // mask the detector
    Geometry::ParameterMap* m_Pmap = &(inputWS->instrumentParameters());
    boost::shared_ptr<Instrument> instru = inputWS->getBaseInstrument();
    Geometry::Detector* toMask =
      dynamic_cast<Geometry::Detector*>( instru->getDetector(THEMASKED).get() );
    TS_ASSERT(toMask);
    m_Pmap->addBool(toMask, "masked", true);

    // required to get it passed the algorthms validator
    inputWS->isDistribution(true);

    return inputWS;
  }
private:
  IAlgorithm* saver;
};

#endif /*SAVENXSPETEST_H_*/
