#ifndef CUSTOM_INTERFACES_WORKSPACE_RAW_FILE_MEMENTO_TEST_H_
#define CUSTOM_INTERFACES_WORKSPACE_RAW_FILE_MEMENTO_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtCustomInterfaces/RawFileMemento.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace MantidQt::CustomInterfaces;

//using namespace Mantid::DataObjects;
using namespace Mantid::API;

class RawFileMementoTest : public CxxTest::TestSuite
{
private:

  static std::string getSuitableFileNamePath()
  {
    return Mantid::API::FileFinder::Instance().getFullPath("LOQ49886.nxs");
  }

public:

  void testConstructorThrowsWithWrongExtension()
  {
    std::string badFile = "LOQ49886.nxss"; //Fictional extension
    TSM_ASSERT_THROWS("Unknown extension, should throw.", new RawFileMemento(badFile), std::invalid_argument);
  }

  void testFileExists()
  {
    RawFileMemento memento(getSuitableFileNamePath());
    TSM_ASSERT("File should be present", memento.checkStillThere()); 
  }

  void testConstructThrowsWhenFileDoesntExist()
  {
    TSM_ASSERT_THROWS("Unknown file, should throw.", new RawFileMemento("MadeUp.nxs"), std::runtime_error);
  }

  void testFetchItSucceedsWhenFileExists()
  {
    RawFileMemento memento(getSuitableFileNamePath());
    TSM_ASSERT("File should be present", memento.checkStillThere()); 
    MatrixWorkspace_sptr result = boost::dynamic_pointer_cast<MatrixWorkspace>(memento.fetchIt(MinimalData));
    TSM_ASSERT("Should have fetched the workspace", result);
  }

  void testFetchItWithMinimalData()
  {
    RawFileMemento memento(getSuitableFileNamePath());
    MatrixWorkspace_sptr result = boost::dynamic_pointer_cast<MatrixWorkspace>(memento.fetchIt(MinimalData));
    TS_ASSERT_EQUALS(1, result->getNumberHistograms());
  }

  void testFetchItWithEverything()
  {
    RawFileMemento memento(getSuitableFileNamePath());
    MatrixWorkspace_sptr result = boost::dynamic_pointer_cast<MatrixWorkspace>(memento.fetchIt(Everything));
    TS_ASSERT(result->getNumberHistograms() > 1);
  }

  void testNoExistingUB()
  {
    RawFileMemento memento(getSuitableFileNamePath());
    TS_ASSERT_EQUALS(WorkspaceMemento::NoOrientedLattice, memento.generateStatus());
  }

  void testApplyActions()
  {
    using Mantid::Geometry::Goniometer;
    using Mantid::Kernel::V3D;

    RawFileMemento memento(getSuitableFileNamePath());
    memento.setUB(0,0,2,0,4,0,-8,0,0);
    memento.setLogValue("A", "12", "Number");
    memento.setLogValue("angle1", "1.234", "Number Series");
    memento.setLogValue("angle2", "2", "Number Series");
    memento.setGoniometer("angle1, 1.0,2.0,3.0,1","angle2, 1.1,2.1,3.1,-1","","","","");

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(memento.applyActions());
    TS_ASSERT(ws->getNumberHistograms() > 1);
    std::vector<double> ub = ws->sample().getOrientedLattice().getUB().get_vector();

    TS_ASSERT_EQUALS("12", ws->run().getLogData("A")->value());
    TS_ASSERT_THROWS_NOTHING(ws->run().getLogData("angle1")->value());
    TS_ASSERT_THROWS_NOTHING(ws->run().getLogData("angle2")->value());
    
    Goniometer & gon = ws->mutableRun().getGoniometer();
    TS_ASSERT_EQUALS( gon.getNumberAxes(), 2);

    TS_ASSERT_EQUALS( gon.getAxis(0).name, "angle1");
    TS_ASSERT_EQUALS( gon.getAxis(0).rotationaxis, V3D(1.0,2.0,3.0));
    TS_ASSERT_EQUALS( gon.getAxis(0).sense, 1);

    TS_ASSERT_EQUALS( gon.getAxis(1).name, "angle2");
    TS_ASSERT_EQUALS( gon.getAxis(1).rotationaxis, V3D(1.1,2.1,3.1));
    TS_ASSERT_EQUALS( gon.getAxis(1).sense, -1);

    TS_ASSERT_EQUALS(0, ub[0]);
    TS_ASSERT_EQUALS(0, ub[1]);
    TS_ASSERT_EQUALS(2, ub[2]);
    TS_ASSERT_EQUALS(0, ub[3]);
    TS_ASSERT_EQUALS(4, ub[4]);
    TS_ASSERT_EQUALS(0, ub[5]);
    TS_ASSERT_EQUALS(-8, ub[6]);
    TS_ASSERT_EQUALS(0, ub[7]);
    TS_ASSERT_EQUALS(0, ub[8]);
  }

};

#endif