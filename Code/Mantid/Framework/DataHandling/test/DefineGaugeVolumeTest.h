#ifndef DEFINEGAUGEVOLUMETEST_H_
#define DEFINEGAUGEVOLUMETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/DefineGaugeVolume.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;

class DefineGaugeVolumeTest : public CxxTest::TestSuite
{
private:
  DefineGaugeVolume gauge;
  const std::string sphere;
  const std::string cylinder;
  

public:
  DefineGaugeVolumeTest() :
    sphere("<sphere id=\"some-sphere\"><centre x=\"0.0\"  y=\"0.0\" z=\"0.0\" /><radius val=\"1.0\" /></sphere>"),
    cylinder("<infinite-cylinder id=\"shape\"><centre x=\"0.0\" y=\"0.0\" z=\"0.0\" /><axis x=\"0.0\" y=\"0.0\" z=\"1\" /><radius val=\"0.1\" /></infinite-cylinder>")
  {}

  void testTheBasics()
  {
    TS_ASSERT_EQUALS(gauge.name(), "DefineGaugeVolume");
    TS_ASSERT_EQUALS(gauge.version(), 1);
    TS_ASSERT_EQUALS(gauge.category(), "Engineering");
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(gauge.initialize());
    TS_ASSERT(gauge.isInitialized());
  }

  void testInvalidShape()
  {
    // Create an 'empty' workspace
    AnalysisDataService::Instance().add("EmptyWorkspace", WorkspaceFactory::Instance().create(
        "WorkspaceSingleValue", 1, 1, 1));

    gauge.setPropertyValue("Workspace", "EmptyWorkspace");
    gauge.setPropertyValue("ShapeXML", sphere.substr(0, 50));

    TS_ASSERT_THROWS_NOTHING(gauge.execute());
    TS_ASSERT(!gauge.isExecuted());
  }

  void testExecute()
  {
    gauge.setPropertyValue("Workspace", "EmptyWorkspace");
    gauge.setPropertyValue("ShapeXML", sphere);

    TS_ASSERT_THROWS_NOTHING(gauge.execute());
    TS_ASSERT(gauge.isExecuted());

    MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("EmptyWorkspace"));

    TS_ASSERT(ws->run().hasProperty("GaugeVolume"));
    TS_ASSERT_EQUALS(ws->run().getProperty("GaugeVolume")->value(), sphere);

    // Now run it again to check that it correctly overwrites the property
    gauge.setPropertyValue("ShapeXML", cylinder);
    TS_ASSERT(gauge.execute());
    TS_ASSERT_EQUALS(ws->run().getProperty("GaugeVolume")->value(), cylinder);

    AnalysisDataService::Instance().remove("EmptyWorkspace");
  }

};

#endif
