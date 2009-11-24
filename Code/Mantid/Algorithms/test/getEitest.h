#ifndef GETE_ITEST_H_
#define GETE_ITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/GetEi.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "Poco/Path.h"
#include <algorithm>
#include <vector>
#include <iostream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;

class GetEiTest : public CxxTest::TestSuite
{
public:
  void testOnMARI()
  {
    GetEi grouper;

    TS_ASSERT_EQUALS( grouper.name(), "GetEi" )
    TS_ASSERT_EQUALS( grouper.version(), 1 )
    TS_ASSERT_EQUALS( grouper.category(), "CorrectionFunctions" )
    TS_ASSERT_THROWS_NOTHING( grouper.initialize() )
    TS_ASSERT( grouper.isInitialized() )

    // now test the algorithm on a workspace
    // we don't need to load much of it, just the monitor spectra
    loadRawFile(m_MARI, "2, 3");

    grouper.setPropertyValue("InputWorkspace", m_WS);
    grouper.setPropertyValue("Monitor1Spec", "2");
    grouper.setPropertyValue("Monitor2Spec", "3");
    grouper.setPropertyValue("EnergyEstimate", "14");

    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    double finalAnswer = grouper.getProperty("IncidentEnergy");
    TS_ASSERT_DELTA( finalAnswer, 12.9448, 1e-4 )//HOMER got 12.973 meV for the IncidentEnergy of MAR11001
  }

  // this test takes 10 seconds to run on Steve's computer
  void testOnMERLIN()
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
    TS_ASSERT_DELTA( finalAnswer, 15.1006, 1e-4 )
  }

  void loadRawFile(std::string filename, std::string list)
  {
    LoadRaw3 loader;
    loader.initialize();

    loader.setPropertyValue("Filename", filename);
    loader.setPropertyValue("OutputWorkspace", m_WS);
    loader.setPropertyValue("SpectrumList", list);
    loader.setPropertyValue("OutputWorkspace", m_WS);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
  }

  GetEiTest() : m_WS("GetEi_input_workspace")
  {
    m_MARI = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/MAR11001.RAW").toString();
    m_MAPS = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/MAP10266.RAW").toString();
    m_MERLIN = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/MER02257.RAW").toString();
  }
  
  ~GetEiTest()
  {
    AnalysisDataService::Instance().remove(m_WS);
  }

  private:
    const std::string m_WS;
    std::string m_MARI, m_MAPS, m_MERLIN;
};

#endif /*GETE_ITEST_H_*/
