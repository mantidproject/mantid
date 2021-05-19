// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IPreview.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/PreviewManager.h"

using Mantid::API::IPreview;
using Mantid::API::PreviewManager;
using Mantid::API::PreviewManagerImpl;
using Mantid::API::Workspace_sptr;

namespace {
class BasicPreview : public IPreview {
public:
  PreviewType type() const override { return IPreview::PreviewType::SVIEW; }
  std::string name() const override { return "BasicPreview"; }
  std::string facility() const override { return "TestFacility"; }
  std::string technique() const override { return "SANS"; }
  std::string acquisition() const override { return "Mono"; }
  std::string geometry() const override { return "1D"; }

private:
  Workspace_sptr preview(Workspace_sptr ws) const override { return ws->clone(); }
};

DECLARE_PREVIEW(BasicPreview)
} // namespace

class PreviewManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PreviewManagerTest *createSuite() { return new PreviewManagerTest(); }
  static void destroySuite(PreviewManagerTest *suite) { delete suite; }

  void test_get_preview_by_facility() {
    auto previews = PreviewManager::Instance().getPreviews("TestFacility");
    TS_ASSERT_EQUALS(previews.size(), 1);
    TS_ASSERT_EQUALS(previews[0], "BasicPreview");
  }
  void test_get_preview_by_nonexistent_facility() {
    auto previews = PreviewManager::Instance().getPreviews("Test2Facility");
    TS_ASSERT(previews.empty());
  }
  void test_get_preview_by_facility_and_technique() {
    auto previews = PreviewManager::Instance().getPreviews("TestFacility", "SANS");
    TS_ASSERT_EQUALS(previews.size(), 1);
    TS_ASSERT_EQUALS(previews[0], "BasicPreview");
  }
  void test_get_preview_by_facility_and_non_existent_technique() {
    auto previews = PreviewManager::Instance().getPreviews("TestFacility", "Crystal");
    TS_ASSERT(previews.empty());
  }
  void test_get_preview_by_facility_and_technique_and_acquisition() {
    auto previews = PreviewManager::Instance().getPreviews("TestFacility", "SANS", "Mono");
    TS_ASSERT_EQUALS(previews.size(), 1);
    TS_ASSERT_EQUALS(previews[0], "BasicPreview");
  }
  void test_get_preview_by_facility_and_technique_and_non_existent_acquisition() {
    auto previews = PreviewManager::Instance().getPreviews("TestFacility", "SANS", "TOF");
    TS_ASSERT(previews.empty());
  }
  void test_get_preview_by_facility_and_acquisition() {
    auto previews = PreviewManager::Instance().getPreviews("TestFacility", "", "Mono");
    TS_ASSERT_EQUALS(previews.size(), 1);
    TS_ASSERT_EQUALS(previews[0], "BasicPreview");
  }
  void test_get_preview_by_name() {
    auto &preview = PreviewManager::Instance().getPreview("TestFacility", "SANS", "Mono", "1D", "BasicPreview");
    TS_ASSERT_EQUALS(preview.name(), "BasicPreview");
    TS_ASSERT_EQUALS(preview.facility(), "TestFacility");
    TS_ASSERT_EQUALS(preview.technique(), "SANS");
    TS_ASSERT_EQUALS(preview.type(), IPreview::PreviewType::SVIEW);
  }
  void test_get_preview_by_non_existent_name() {
    TS_ASSERT_THROWS(PreviewManager::Instance().getPreview("TestFacility", "SANS", "BasicPreview2"),
                     const std::runtime_error &)
  }
};
