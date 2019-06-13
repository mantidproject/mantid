// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_EventWorkspaceCollectionTEST_H_
#define MANTID_DATAHANDLING_EventWorkspaceCollectionTEST_H_

#include "MantidKernel/TimeSeriesProperty.h"

#include <cxxtest/TestSuite.h>

#include <boost/make_shared.hpp>
#include <memory>

#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/EventWorkspaceCollection.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidIndexing/IndexInfo.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {

EventWorkspaceCollection_uptr
makeEventWorkspaceCollection(unsigned int decoratorSize) {

  auto decorator = std::make_unique<EventWorkspaceCollection>();

  auto periodLog =
      std::make_unique<const TimeSeriesProperty<int>>("period_log");

  decorator->setNPeriods(decoratorSize, periodLog);

  return decorator;
}
} // namespace

class EventWorkspaceCollectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventWorkspaceCollectionTest *createSuite() {
    return new EventWorkspaceCollectionTest();
  }
  static void destroySuite(EventWorkspaceCollectionTest *suite) {
    delete suite;
  }

  void test_constructor() {
    EventWorkspaceCollection decorator;
    TSM_ASSERT_EQUALS("Always one period by default", 1, decorator.nPeriods());
  }

  void test_output_single_workspace() {
    EventWorkspaceCollection decorator;
    TSM_ASSERT_EQUALS("Always one period by default", 1, decorator.nPeriods());
    TS_ASSERT_EQUALS(decorator.combinedWorkspace(),
                     decorator.getSingleHeldWorkspace());
  }

  void test_output_multiple_workspaces() {
    EventWorkspaceCollection decorator;

    auto periodLog =
        std::make_unique<const TimeSeriesProperty<int>>("period_log");
    decorator.setNPeriods(3, periodLog);

    WorkspaceGroup_sptr outWS = boost::dynamic_pointer_cast<WorkspaceGroup>(
        decorator.combinedWorkspace());
    TSM_ASSERT("Should be a WorkspaceGroup", outWS);
    TS_ASSERT_EQUALS(3, outWS->size());
  }

  void test_set_geometryFlag() {

    EventWorkspaceCollection_uptr decorator = makeEventWorkspaceCollection(3);

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

    EventWorkspaceCollection_uptr decorator = makeEventWorkspaceCollection(3);

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
    EventWorkspaceCollection_uptr decorator = makeEventWorkspaceCollection(3);

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

    EventWorkspaceCollection_uptr decorator = makeEventWorkspaceCollection(3);

    const float width = 3;

    decorator->setWidth(width);

    WorkspaceGroup_sptr outWS = boost::dynamic_pointer_cast<WorkspaceGroup>(
        decorator->combinedWorkspace());

    for (size_t i = 0; i < outWS->size(); ++i) {
      auto memberWS =
          boost::dynamic_pointer_cast<EventWorkspace>(outWS->getItem(i));
      TSM_ASSERT_EQUALS("Child workspaces should all have the width set", width,
                        memberWS->sample().getWidth());
    }
  }

  void test_setIndexInfo() {
    EventWorkspaceCollection collection;
    auto periodLog =
        std::make_unique<const TimeSeriesProperty<int>>("period_log");
    const size_t periods = 2;
    collection.setNPeriods(periods, periodLog);
    // Set some arbitrary data to ensure that it is preserved.
    const float thickness = static_cast<float>(1.23);
    collection.setThickness(thickness);

    collection.setIndexInfo(Indexing::IndexInfo({3, 1, 2}));
    const auto ws = boost::dynamic_pointer_cast<WorkspaceGroup>(
        collection.combinedWorkspace());
    for (size_t i = 0; i < periods; ++i) {
      auto eventWS =
          boost::dynamic_pointer_cast<EventWorkspace>(ws->getItem(i));
      TS_ASSERT_EQUALS(eventWS->getSpectrum(0).getSpectrumNo(), 3);
      TS_ASSERT_EQUALS(eventWS->getSpectrum(1).getSpectrumNo(), 1);
      TS_ASSERT_EQUALS(eventWS->getSpectrum(2).getSpectrumNo(), 2);
      TS_ASSERT_EQUALS(eventWS->sample().getThickness(), thickness);
    }
  }
};

#endif /* MANTID_DATAHANDLING_EventWorkspaceCollectionTEST_H_ */
