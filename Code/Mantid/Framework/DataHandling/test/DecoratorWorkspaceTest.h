#ifndef MANTID_DATAHANDLING_DECORATORWORKSPACETEST_H_
#define MANTID_DATAHANDLING_DECORATORWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/TimeSeriesProperty.h"

#include <memory>
#include <boost/make_shared.hpp>

#include "MantidDataHandling/DecoratorWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {

DecoratorWorkspace_uptr makeDecoratorWorkspace(unsigned int decoratorSize) {

  DecoratorWorkspace_uptr decorator(new DecoratorWorkspace);

  std::unique_ptr<const TimeSeriesProperty<int>> periodLog(
      new const TimeSeriesProperty<int>("period_log"));

  decorator->setNPeriods(decoratorSize, periodLog);

  return decorator;
}
}

class DecoratorWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DecoratorWorkspaceTest *createSuite() {
    return new DecoratorWorkspaceTest();
  }
  static void destroySuite(DecoratorWorkspaceTest *suite) { delete suite; }

  void test_constructor() {
    DecoratorWorkspace decorator;
    TSM_ASSERT_EQUALS("Always one period by default", 1, decorator.nPeriods());
  }

  void test_output_single_workspace() {
    DecoratorWorkspace decorator;
    TSM_ASSERT_EQUALS("Always one period by default", 1, decorator.nPeriods());
    TS_ASSERT_EQUALS(decorator.combinedWorkspace(),
                     decorator.getSingleHeldWorkspace());
  }

  void test_output_multiple_workspaces() {
    DecoratorWorkspace decorator;

    std::unique_ptr<const TimeSeriesProperty<int>> periodLog(
        new const TimeSeriesProperty<int>("period_log"));
    decorator.setNPeriods(3, periodLog);

    WorkspaceGroup_sptr outWS = boost::dynamic_pointer_cast<WorkspaceGroup>(
        decorator.combinedWorkspace());
    TSM_ASSERT("Should be a WorkspaceGroup", outWS);
    TS_ASSERT_EQUALS(3, outWS->size());
  }

  void test_set_geometryFlag() {

    DecoratorWorkspace_uptr decorator = makeDecoratorWorkspace(3);

    const int geometryFlag = 3;

    decorator->setGeometryFlag(geometryFlag);

    WorkspaceGroup_sptr outWS = boost::dynamic_pointer_cast<WorkspaceGroup>(
        decorator->combinedWorkspace());

    for (size_t i = 0; i < outWS->size(); ++i) {
      auto memberWS =
          boost::dynamic_pointer_cast<EventWorkspace>(outWS->getItem(i));
      TSM_ASSERT_EQUALS(
          "Child workspaces should all have the geometry flag set",
          geometryFlag, memberWS->sample().getGeometryFlag());
    }
  }

  void test_set_thickness() {

    DecoratorWorkspace_uptr decorator = makeDecoratorWorkspace(3);

    const float thickness = 3;

    decorator->setThickness(thickness);

    WorkspaceGroup_sptr outWS = boost::dynamic_pointer_cast<WorkspaceGroup>(
        decorator->combinedWorkspace());

    for (size_t i = 0; i < outWS->size(); ++i) {
      auto memberWS =
          boost::dynamic_pointer_cast<EventWorkspace>(outWS->getItem(i));
      TSM_ASSERT_EQUALS("Child workspaces should all have the thickness set",
                        thickness, memberWS->sample().getThickness());
    }
  }

  void test_set_height() {
    DecoratorWorkspace_uptr decorator = makeDecoratorWorkspace(3);

    const float height = 3;

    decorator->setHeight(height);

    WorkspaceGroup_sptr outWS = boost::dynamic_pointer_cast<WorkspaceGroup>(
        decorator->combinedWorkspace());

    for (size_t i = 0; i < outWS->size(); ++i) {
      auto memberWS =
          boost::dynamic_pointer_cast<EventWorkspace>(outWS->getItem(i));
      TSM_ASSERT_EQUALS("Child workspaces should all have the height set",
                        height, memberWS->sample().getHeight());
    }
  }

  void test_set_width() {

    DecoratorWorkspace_uptr decorator = makeDecoratorWorkspace(3);

    const float width = 3;

    decorator->setWidth(width);

    WorkspaceGroup_sptr outWS = boost::dynamic_pointer_cast<WorkspaceGroup>(
        decorator->combinedWorkspace());

    for (size_t i = 0; i < outWS->size(); ++i) {
      auto memberWS =
          boost::dynamic_pointer_cast<EventWorkspace>(outWS->getItem(i));
      TSM_ASSERT_EQUALS("Child workspaces should all have the width set",
                        width, memberWS->sample().getWidth());
    }
  }
};

#endif /* MANTID_DATAHANDLING_DECORATORWORKSPACETEST_H_ */
