// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CLEARINSTRUMENTPARAMETERSTEST_H
#define CLEARINSTRUMENTPARAMETERSTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/ClearInstrumentParameters.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/OptionalBool.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class ClearInstrumentParametersTest : public CxxTest::TestSuite {
public:
  void testClearInstrumentParameters() {
    // Load a workspace
    prepareWorkspace();

    // Set some parameters
    setParam("nickel-holder", "testDouble", 1.23);
    setParam("nickel-holder", "testString", "hello world");
    const auto oldPos = m_ws->detectorInfo().position(0);
    m_ws->mutableDetectorInfo().setPosition(0, oldPos + V3D(1.1, 2.2, 3.3));
    TS_ASSERT_DIFFERS(oldPos, m_ws->detectorInfo().position(0));

    // Clear the parameters
    clearParameters();

    // Check the parameters
    checkEmpty("nickel-holder", "testDouble");
    checkEmpty("nickel-holder", "testString");
    TS_ASSERT_EQUALS(oldPos, m_ws->detectorInfo().position(0));
  }

  void setParam(std::string cName, std::string pName, std::string value) {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap &paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    paramMap.addString(comp->getComponentID(), pName, value);
  }

  void setParam(std::string cName, std::string pName, double value) {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap &paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    paramMap.addDouble(comp->getComponentID(), pName, value);
  }

  void checkEmpty(std::string cName, std::string pName) {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap &paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    bool exists = paramMap.contains(comp.get(), pName);
    TS_ASSERT_EQUALS(exists, false);
  }

  void clearParameters() {
    ClearInstrumentParameters clearer;
    TS_ASSERT_THROWS_NOTHING(clearer.initialize());
    clearer.setPropertyValue("Workspace", m_ws->getName());
    TS_ASSERT_THROWS_NOTHING(clearer.execute());
    TS_ASSERT(clearer.isExecuted());
  }

  void prepareWorkspace() {
    LoadInstrument loaderIDF2;

    TS_ASSERT_THROWS_NOTHING(loaderIDF2.initialize());

    std::string wsName = "SaveParameterFileTestIDF2";
    Workspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    loaderIDF2.setPropertyValue("Filename",
                                "unit_testing/IDF_for_UNIT_TESTING2.xml");
    loaderIDF2.setPropertyValue("Workspace", wsName);
    loaderIDF2.setProperty("RewriteSpectraMap",
                           Mantid::Kernel::OptionalBool(true));
    TS_ASSERT_THROWS_NOTHING(loaderIDF2.execute());
    TS_ASSERT(loaderIDF2.isExecuted());

    m_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws2D);
  }

private:
  MatrixWorkspace_sptr m_ws;
};

#endif /* CLEARINSTRUMENTPARAMETERSTEST_H */
