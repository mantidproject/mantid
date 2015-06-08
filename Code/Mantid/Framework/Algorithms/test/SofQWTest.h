#ifndef SOFQWTEST_H_
#define SOFQWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SofQW.h"
#include "MantidDataHandling/LoadNexusProcessed.h"

#include <boost/make_shared.hpp>

using namespace Mantid::API;

class SofQWTest : public CxxTest::TestSuite
{
public:

  template <typename SQWType>
  static Mantid::API::MatrixWorkspace_sptr runSQW(const std::string & method = "") {
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setChild(true);
    loader.setProperty("Filename","IRS26173_ipg.nxs");
    loader.setPropertyValue("OutputWorkspace","__unused");
    loader.execute();

    Mantid::API::Workspace_sptr loadedWS = loader.getProperty("OutputWorkspace");
    auto inWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(loadedWS);
    WorkspaceHelpers::makeDistribution(inWS);

    SQWType sqw;
    sqw.initialize();
    // Cannot be marked as child or history is not recorded
    TS_ASSERT_THROWS_NOTHING( sqw.setProperty("InputWorkspace", inWS) );
    std::ostringstream wsname;
    wsname << "_tmp_" << loadedWS;
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("OutputWorkspace", wsname.str()) );
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("QAxisBinning","0.5,0.25,2") );
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("EMode","Indirect") );
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("EFixed","1.84") );
    if(!method.empty()) sqw.setPropertyValue("Method", method);
    TS_ASSERT_THROWS_NOTHING( sqw.execute() );
    TS_ASSERT( sqw.isExecuted() );

    auto & dataStore = Mantid::API::AnalysisDataService::Instance();
    auto result = dataStore.retrieveWS<Mantid::API::MatrixWorkspace>(wsname.str());
    dataStore.remove(wsname.str());
    return result;
  }

  void testName()
  {
    Mantid::Algorithms::SofQW sqw;
    TS_ASSERT_EQUALS( sqw.name(), "SofQW" );
  }

  void testVersion()
  {
    Mantid::Algorithms::SofQW sqw;
    TS_ASSERT_EQUALS( sqw.version(), 1 );
  }

  void testCategory()
  {
    Mantid::Algorithms::SofQW sqw;
    TS_ASSERT_EQUALS( sqw.category(), "Inelastic" );
  }

  void testInit()
  {
    Mantid::Algorithms::SofQW sqw;
    TS_ASSERT_THROWS_NOTHING( sqw.initialize() );
    TS_ASSERT( sqw.isInitialized() );
  }

  void testExecWithDefaultMethodUsesSofQWCentre()
  {
    auto result = SofQWTest::runSQW<Mantid::Algorithms::SofQW>();

    TS_ASSERT(isAlgorithmInHistory(*result, "SofQWCentre"));

    TS_ASSERT_EQUALS( result->getAxis(0)->length(), 1904 );
    TS_ASSERT_EQUALS( result->getAxis(0)->unit()->unitID(), "DeltaE" );
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(0), -0.5590, 0.0001 );
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(999), -0.0971, 0.0001 );
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(1900), 0.5728, 0.0001 );

    TS_ASSERT_EQUALS( result->getAxis(1)->length(), 7 );
    TS_ASSERT_EQUALS( result->getAxis(1)->unit()->unitID(), "MomentumTransfer" );
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(0), 0.5 );
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(3), 1.25 );
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(6), 2.0 );

    const double delta(1e-08);
    TS_ASSERT_DELTA( result->readY(0)[1160], 54.85624399, delta);
    TS_ASSERT_DELTA( result->readE(0)[1160], 0.34252858, delta);
    TS_ASSERT_DELTA( result->readY(1)[1145], 22.72491806, delta);
    TS_ASSERT_DELTA( result->readE(1)[1145], 0.19867742, delta);
    TS_ASSERT_DELTA( result->readY(2)[1200], 6.76047436, delta);
    TS_ASSERT_DELTA( result->readE(2)[1200], 0.10863549, delta);
    TS_ASSERT_DELTA( result->readY(3)[99], 0.16439574, delta);
    TS_ASSERT_DELTA( result->readE(3)[99], 0.03414360, delta);
    TS_ASSERT_DELTA( result->readY(4)[1654], 0.069311442, delta);
    TS_ASSERT_DELTA( result->readE(4)[1654], 0.007573484, delta);
    TS_ASSERT_DELTA( result->readY(5)[1025], 0.226287179, delta);
    TS_ASSERT_DELTA( result->readE(5)[1025], 0.02148236, delta);
  }

  void testExecUsingDifferentMethodChoosesDifferentAlgorithm()
  {
    auto result = SofQWTest::runSQW<Mantid::Algorithms::SofQW>("Polygon");

    TS_ASSERT(isAlgorithmInHistory(*result, "SofQWPolygon"));
    // results are checked in the dedicated algorithm test
  }

private:

  bool isAlgorithmInHistory(const Mantid::API::MatrixWorkspace & result, const std::string & name) {
    // Loaded nexus file has 13 other entries
    const auto & wsHistory = result.getHistory();
    const auto & lastAlg = wsHistory.getAlgorithmHistory(wsHistory.size() - 1);
    const auto child = lastAlg->getChildAlgorithmHistory(0);
    return (child->name() == name);
  }

};

#endif /*SOFQWTEST_H_*/
