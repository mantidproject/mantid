#ifndef CUSTOM_INTERFACES_WORKSPACE_EVENT_NEXUS_FILE_MEMENTO_TEST_H_
#define CUSTOM_INTERFACES_WORKSPACE_EVENT_NEXUS_FILE_MEMENTO_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtCustomInterfaces/EventNexusFileMemento.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace MantidQt::CustomInterfaces;

using namespace Mantid::API;

class EventNexusFileMementoTest : public CxxTest::TestSuite
{
private:

  static std::string getSuitableFileNamePath()
  {
    return Mantid::API::FileFinder::Instance().getFullPath("CNCS_7860_event.nxs");
  }

public:

  void testConstructorThrowsWithWrongExtension()
  {
    std::string badFile = "CNCS_7860_event.rrr"; //Fictional extension
    TSM_ASSERT_THROWS("Unknown extension, should throw.", new EventNexusFileMemento(badFile), std::invalid_argument);
  }

  void testFileExists()
  {
    EventNexusFileMemento memento(getSuitableFileNamePath());
    TSM_ASSERT("File should be present", memento.checkStillThere()); 
  }

  void testConstructThrowsWhenFileDoesntExist()
  {
    TSM_ASSERT_THROWS("Unknown file, should throw.", new EventNexusFileMemento("MadeUp.nxs"), std::runtime_error);
  }

  void testFetchItSucceedsWhenFileExists()
  {
    EventNexusFileMemento memento(getSuitableFileNamePath());
    TSM_ASSERT("File should be present", memento.checkStillThere()); 
    IEventWorkspace_sptr result = boost::dynamic_pointer_cast<IEventWorkspace>(memento.fetchIt(MinimalData));
    TSM_ASSERT("Should have fetched the workspace", result);
  }

  void testFetchMinimalData()
  {
    EventNexusFileMemento memento(getSuitableFileNamePath());
    IEventWorkspace_sptr result = boost::dynamic_pointer_cast<IEventWorkspace>(memento.fetchIt(MinimalData));
    TS_ASSERT_EQUALS(0, result->getNumberEvents());
  }

  void testFetchEverything()
  {
    EventNexusFileMemento memento(getSuitableFileNamePath());
    IEventWorkspace_sptr result = boost::dynamic_pointer_cast<IEventWorkspace>(memento.fetchIt(Everything));
    TS_ASSERT(result->getNumberEvents() > 1);
  }

  void testNoExistingUB()
  {
    EventNexusFileMemento memento(getSuitableFileNamePath());
    TS_ASSERT_EQUALS(WorkspaceMemento::NoOrientedLattice, memento.generateStatus());
  }

  void testApplyActions()
  {
    EventNexusFileMemento memento(getSuitableFileNamePath());
    memento.setUB(0,0,2,0,4,0,-8,0,0);
    IEventWorkspace_sptr ws = boost::dynamic_pointer_cast<IEventWorkspace>(memento.applyActions());
    TS_ASSERT(ws->getNumberEvents() > 1);
    std::vector<double> ub = ws->sample().getOrientedLattice().getUB().get_vector();
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