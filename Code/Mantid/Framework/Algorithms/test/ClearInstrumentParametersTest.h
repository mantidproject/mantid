#ifndef CLEARINSTRUMENTPARAMETERSTEST_H
#define CLEARINSTRUMENTPARAMETERSTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ClearInstrumentParameters.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class ClearInstrumentParametersTest : public CxxTest::TestSuite
{
public:

  void testClearInstrumentParameters()
  {
    //Load a workspace
    prepareWorkspace();

    //Set some parameters
    setParam("nickel-holder", "testDouble", 1.23);
    setParam("nickel-holder", "testString", "hello world");

    //Clear the parameters
    clearParameters();

    //Check the parameters
    checkEmpty("nickel-holder", "testDouble");
    checkEmpty("nickel-holder", "testString");
  }

  void setParam(std::string cName, std::string pName, std::string value)
  {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap& paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    paramMap.addString(comp->getComponentID(), pName, value);
  }

  void setParam(std::string cName, std::string pName, double value)
  {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap& paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    paramMap.addDouble(comp->getComponentID(), pName, value);
  }

  void checkEmpty(std::string cName, std::string pName)
  {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap& paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    bool exists = paramMap.contains(comp.get(), pName);
    TS_ASSERT_EQUALS(exists, false);
  }

  void clearParameters()
  {
    ClearInstrumentParameters clearer;
    TS_ASSERT_THROWS_NOTHING(clearer.initialize());
    clearer.setPropertyValue("Workspace", m_ws->name());
    TS_ASSERT_THROWS_NOTHING(clearer.execute());
    TS_ASSERT(clearer.isExecuted());
  }

  void prepareWorkspace()
  {
    LoadInstrument loaderIDF2;

    TS_ASSERT_THROWS_NOTHING(loaderIDF2.initialize());

    std::string wsName = "SaveParameterFileTestIDF2";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    loaderIDF2.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING2.xml");
    loaderIDF2.setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(loaderIDF2.execute());
    TS_ASSERT( loaderIDF2.isExecuted() );

    m_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws2D);
  }

private:
  MatrixWorkspace_sptr m_ws;
};

#endif /* CLEARINSTRUMENTPARAMETERSTEST_H */
