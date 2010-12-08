#ifndef GETE_ITEST_H_
#define GETE_ITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/GetEi.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "Poco/Path.h"
#include <vector>
#include <string>


using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;

class GetEiTest : public CxxTest::TestSuite
{
public:
  void xtestOnMARI()
  {
    GetEi grouper;

    TS_ASSERT_EQUALS( grouper.name(), "GetEi" )
    TS_ASSERT_EQUALS( grouper.version(), 1 )
    TS_ASSERT_EQUALS( grouper.category(), "CorrectionFunctions" )
    TS_ASSERT_THROWS_NOTHING( grouper.initialize() )
    TS_ASSERT( grouper.isInitialized() )

    // now test the algorithm on a workspace
    // we don't need to load much of it, just the monitor spectra
    loadRawFile(m_MARI1, "2, 3");

    grouper.setPropertyValue("InputWorkspace", m_WS);
    grouper.setPropertyValue("Monitor1Spec", "2");
    grouper.setPropertyValue("Monitor2Spec", "3");
    grouper.setPropertyValue("EnergyEstimate", "14");
    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );
    double answer = grouper.getProperty("IncidentEnergy");
    TS_ASSERT_DELTA( answer, 12.9444, 1e-4 )//HOMER got 12.973 meV for the IncidentEnergy of MAR11001
    
    // test some more MARI runs
    loadRawFile(m_MARI2, "2, 3");

    grouper.setPropertyValue("InputWorkspace", m_WS);
    grouper.setPropertyValue("Monitor1Spec", "2");
    grouper.setPropertyValue("Monitor2Spec", "3");
    grouper.setPropertyValue("EnergyEstimate", "7");
    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );
    answer = grouper.getProperty("IncidentEnergy");
    TS_ASSERT_DELTA( answer, 6.8222, 1e-4 )//HOMER errorously got 6.518 meV for the IncidentEnergy of MAR15306
    
    loadRawFile(m_MARI3, "2, 3");
    grouper.setPropertyValue("InputWorkspace", m_WS);
    grouper.setPropertyValue("Monitor1Spec", "2");
    grouper.setPropertyValue("Monitor2Spec", "3");
    grouper.setPropertyValue("EnergyEstimate", "680");
    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );
    answer = grouper.getProperty("IncidentEnergy");
    TS_ASSERT_DELTA( answer, 717.9787, 1e-4 )//HOMER got 718.716 meV for the IncidentEnergy of MAR15317
  }

  // this test takes 10 seconds to run on Steve's computer
  void xtestOnMERLIN()
  {
    loadRawFile(m_MERLIN, "69634, 69638");

    GetEi grouper;
    grouper.initialize();

    //MAPS one off test, takes to long to do it every time. To activate uncomment the code below and two sections below that
  // when the MAP workspace that is load in the constructor
/*    grouper.setPropertyValue("InputWorkspace", m_WS);
    grouper.setPropertyValue("Monitor1Spec", "41474");
    grouper.setPropertyValue("Monitor2Spec", "41475");
    grouper.setPropertyValue("EnergyEstimate", "400");*/

    grouper.setPropertyValue("InputWorkspace", m_WS);
    grouper.setPropertyValue("Monitor1Spec", "69634");
    grouper.setPropertyValue("Monitor2Spec", "69638");
    grouper.setPropertyValue("EnergyEstimate", "15");

    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    double finalAnswer = grouper.getProperty("IncidentEnergy");
    TS_ASSERT_DELTA( finalAnswer, 15.1140, 1e-4 )
    AnalysisDataService::Instance().remove(m_WS);
  }

  void xloadRawFile(std::string filename, std::string list)
  {
    LoadRaw3 loader;
    loader.initialize();

    loader.setPropertyValue("Filename", filename);
    loader.setPropertyValue("OutputWorkspace", m_WS);
    loader.setPropertyValue("SpectrumList", list);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
  }

  GetEiTest() : m_WS("GetEi_input_workspace")
  {
    m_MARI1 = Poco::Path(Poco::Path::current()).resolve("../../../../Test/AutoTestData/MAR11001.raw").toString();
    m_MARI2 = Poco::Path(Poco::Path::current()).resolve("../../../../Test/AutoTestData/MAR15306.raw").toString();
    m_MARI3 = Poco::Path(Poco::Path::current()).resolve("../../../../Test/AutoTestData/MAR15317.raw").toString();
    m_MAPS = Poco::Path(Poco::Path::current()).resolve("../../../../Test/AutoTestData/MAP10266.raw").toString();
    m_MERLIN = Poco::Path(Poco::Path::current()).resolve("../../../../Test/AutoTestData/MER02257.raw").toString();
  }

  private:
    const std::string m_WS;
    std::string m_MARI1, m_MARI2, m_MARI3;
    std::string m_MAPS, m_MERLIN;
};

#endif /*GETE_ITEST_H_*/
