#ifndef UNWRAPTEST_H_
#define UNWRAPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Unwrap.h"
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidKernel/PropertyWithValue.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;

class UnwrapTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( unwrap.name(), "Unwrap" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( unwrap.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( unwrap.category(), "Units" )
  }

  void testInit()
  {
    unwrap.initialize();
    TS_ASSERT( unwrap.isInitialized() )

    const std::vector<Property*> props = unwrap.getProperties();
    TS_ASSERT_EQUALS( props.size(), 4 )

    TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" )
    TS_ASSERT( props[0]->isDefault() )
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[0]) )

    TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" )
    TS_ASSERT( props[1]->isDefault() )
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[1]) )

    TS_ASSERT_EQUALS( props[2]->name(), "LRef" )
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[2]) )
  }

  void testExec()
  {
    ConfigService::Instance().setString("default.facility", "ISIS");

    IAlgorithm* loader = new Mantid::DataHandling::LoadRaw2;
    loader->initialize();
    loader->setPropertyValue("Filename", "OSI11886.raw");

    std::string outputSpace = "toUnwrap";
    loader->setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING( loader->execute() )
    TS_ASSERT( loader->isExecuted() )

    unwrap.setPropertyValue("InputWorkspace", outputSpace);
    unwrap.setPropertyValue("OutputWorkspace", "unwrappedWS" );
    unwrap.setPropertyValue("LRef","36.0");

    TS_ASSERT_THROWS_NOTHING( TS_ASSERT(unwrap.execute()) )
    TS_ASSERT( unwrap.isExecuted() )

    boost::shared_ptr<MatrixWorkspace> inWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace));
    boost::shared_ptr<MatrixWorkspace> outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("unwrappedWS"));

    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->unitID(), "Wavelength" )
    TS_ASSERT_EQUALS( &(outWS->spectraMap()), &(inWS->spectraMap()) )
    TS_ASSERT_DIFFERS( outWS->size(), inWS->size() )
    TS_ASSERT_DIFFERS( outWS->blocksize(), inWS->blocksize() )
    TS_ASSERT_EQUALS( outWS->blocksize(), 712 )

    TS_ASSERT_DELTA( outWS->dataX(0)[0], 12.956, 0.0001 )
    TS_ASSERT_DELTA( outWS->dataX(0)[350], 15.1168, 0.0001 )
    TS_ASSERT_DELTA( outWS->dataX(0)[712], 17.3516, 0.0001 )

    // Test the frame overlapping part
    Unwrap unwrap2;
    TS_ASSERT_THROWS_NOTHING( unwrap2.initialize() )
    unwrap2.setPropertyValue("InputWorkspace", outputSpace);
    unwrap2.setPropertyValue("OutputWorkspace", "unwrappedWS2" );
    unwrap2.setPropertyValue("LRef","40.0");

    TS_ASSERT_THROWS_NOTHING( unwrap2.execute() )
    TS_ASSERT( unwrap2.isExecuted() )
  }

private:
  Unwrap unwrap;
};

#endif /*UNWRAPTEST_H_*/
