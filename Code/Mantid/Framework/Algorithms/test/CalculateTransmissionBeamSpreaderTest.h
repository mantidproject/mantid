#ifndef CALCULATETRANSMISSIONBEAMSPREADERTEST_H_
#define CALCULATETRANSMISSIONBEAMSPREADERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateTransmissionBeamSpreader.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidCurveFitting/Linear.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;

class CalculateTransmissionBeamSpreaderTest : public CxxTest::TestSuite
{
public:
  CalculateTransmissionBeamSpreaderTest() : trans() {}
  
  void testName()
  {
    TS_ASSERT_EQUALS( trans.name(), "CalculateTransmissionBeamSpreader" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( trans.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( trans.category(), "SANS" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( trans.initialize() )
    TS_ASSERT( trans.isInitialized() )	  
  }

  void testSingleBin()
  {
    // By default UDET=2 for the monitor, which is spectrum 1
    // Sample spreader fake. Created with Y=2, scale it to Y=10
    const std::string sample_spreader("sample_spreader_ws");
    Workspace2D_sptr sample_spreaderWS = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(sample_spreader);
    sample_spreaderWS *= 5.0;
    sample_spreaderWS->dataY(1)[0] = 1.0;
    Mantid::API::AnalysisDataService::Instance().addOrReplace(sample_spreader, sample_spreaderWS);

    // Sample scattering fake. Scale it to Y=8
    const std::string sample_scatt("sample_scatt_ws");
    Workspace2D_sptr sample_scattWS = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(sample_scatt);
    sample_scattWS *= 4.0;
    sample_scattWS->dataY(1)[0] = 1.0;
    Mantid::API::AnalysisDataService::Instance().addOrReplace(sample_scatt, sample_scattWS);

    // Empty spreader fake. Scale it to Y=6
    const std::string empty_spreader("empty_spreader_ws");
    Workspace2D_sptr empty_spreaderWS = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(empty_spreader);
    empty_spreaderWS *= 3.0;
    empty_spreaderWS->dataY(1)[0] = 1.0;
    Mantid::API::AnalysisDataService::Instance().addOrReplace(empty_spreader, empty_spreaderWS);

    // Empty scattering fake.
    const std::string empty_scatt("empty_scatt_ws");
    Workspace2D_sptr empty_scattWS = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(empty_scatt);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(empty_scatt, empty_scattWS);
    empty_scattWS->dataY(1)[0] = 1.0;

    TS_ASSERT_EQUALS( empty_scattWS->dataY(0).size(), 1 )
    TS_ASSERT_EQUALS( sample_spreaderWS->dataY(3)[0], 10);
    TS_ASSERT_EQUALS( sample_scattWS->dataY(3)[0], 8);
    TS_ASSERT_EQUALS( empty_spreaderWS->dataY(3)[0], 6);
    TS_ASSERT_EQUALS( empty_scattWS->dataY(3)[0], 2);

    Mantid::Algorithms::CalculateTransmissionBeamSpreader trans;
    if ( !trans.isInitialized() ) trans.initialize();

    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("SampleSpreaderRunWorkspace", sample_spreader) )
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("DirectSpreaderRunWorkspace", empty_spreader) )
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("SampleScatterRunWorkspace", sample_scatt) )
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("DirectScatterRunWorkspace", empty_scatt) )
    std::string outputWS("outputWS2");
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("OutputWorkspace",outputWS) )

    trans.execute();
    TS_ASSERT( trans.isExecuted() )

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS)) )
    if (!output) return;
    TS_ASSERT_DELTA( output->readY(0)[0], 0.5, 0.010 )

    Mantid::API::AnalysisDataService::Instance().remove(sample_spreader);
    Mantid::API::AnalysisDataService::Instance().remove(empty_spreader);
    Mantid::API::AnalysisDataService::Instance().remove(sample_scatt);
    Mantid::API::AnalysisDataService::Instance().remove(empty_scatt);
  }

private:
  Mantid::Algorithms::CalculateTransmissionBeamSpreader trans;
};

#endif /*CALCULATETRANSMISSIONBEAMSPREADERTEST_H_*/
