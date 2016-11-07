
#ifndef MANTID_CUSTOMINTERFACES_PROJECTSAVEPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_PROJECTSAVEPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/ProjectSavePresenter.h"
#include "ProjectSaveMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ProjectSavePresenterTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProjectSavePresenterTest *createSuite() {
    return new ProjectSavePresenterTest();
  }

  static void destroySuite(ProjectSavePresenterTest *suite) { delete suite; }

  ProjectSavePresenterTest() {}

  void testConstruct() {
    MockProjectSaveView view;
    ProjectSavePresenter presenter(&view);
  }
};

#endif
