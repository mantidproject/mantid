#ifndef ALIGNDETECTORSTEST_H_
#define ALIGNDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/AlignDetectors.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/SpectraDetectorMap.h"

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class AlignDetectorsTest : public CxxTest::TestSuite
{
public:
  AlignDetectorsTest()
  {
    LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/Data/HRP38692.RAW");
    inputWS = "rawWS";
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setProperty("SpectrumMin",320);
    loader.setProperty("SpectrumMax",330);
    loader.execute();
	
  }

  void testName()
  {
    TS_ASSERT_EQUALS( align.name(), "AlignDetectors" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( align.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( align.category(), "DataHandling\\Detectors" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( align.initialize() )
    TS_ASSERT( align.isInitialized() )

    std::vector<Property*> props = align.getProperties();
    TS_ASSERT_EQUALS( static_cast<int>(props.size()), 3 )
  }

  void testExec()
  {
    if ( !align.isInitialized() ) align.initialize();

    TS_ASSERT_THROWS( align.execute(), std::runtime_error )

    align.setPropertyValue("InputWorkspace", inputWS);
    const std::string outputWS = "aligned";
    align.setPropertyValue("OutputWorkspace", outputWS);
    align.setPropertyValue("CalibrationFile", "../../../../Test/Data/hrpd_new_072_01.cal");

    TS_ASSERT_THROWS_NOTHING( align.execute() )
    TS_ASSERT( align.isExecuted() )

    boost::shared_ptr<MatrixWorkspace> inWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(inputWS));
    boost::shared_ptr<MatrixWorkspace> outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));

    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->unitID(), "dSpacing" )
    TS_ASSERT_EQUALS( &(outWS->spectraMap()), &(inWS->spectraMap()) )
    TS_ASSERT_EQUALS( outWS->size(), inWS->size() )
    TS_ASSERT_EQUALS( outWS->blocksize(), inWS->blocksize() )

    TS_ASSERT_DELTA( outWS->dataX(2)[50], 0.7223, 0.0001 )
    TS_ASSERT_EQUALS( outWS->dataY(2)[50], inWS->dataY(1)[50] )
    TS_ASSERT_EQUALS( outWS->dataY(2)[50], inWS->dataY(1)[50] )
	AnalysisDataService::Instance().remove(outputWS);
  }

private:
  AlignDetectors align;
  std::string inputWS;

};

#endif /*ALIGNDETECTORSTEST_H_*/
