// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IPreview.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include <cxxtest/TestSuite.h>

#include <boost/pointer_cast.hpp>

using Mantid::API::FrameworkManager;
using Mantid::API::IPreview;
using Mantid::API::Workspace;
using Mantid::API::Workspace_sptr;
using Mantid::API::WorkspaceFactory;

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

IPreview *createBasicPreview() { return new BasicPreview(); }
} // namespace

class IPreviewTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IPreviewTest *createSuite() { return new IPreviewTest(); }
  static void destroySuite(IPreviewTest *suite) { delete suite; }

  void setUp() override { FrameworkManager::Instance(); }

  void test_basic_preview() {
    IPreview *preview = createBasicPreview();
    TS_ASSERT_EQUALS(preview->name(), "BasicPreview")
    TS_ASSERT_EQUALS(preview->facility(), "TestFacility")
    TS_ASSERT_EQUALS(preview->technique(), "SANS")
    TS_ASSERT_EQUALS(preview->acquisition(), "Mono")
    auto inWS = WorkspaceFactory::Instance().create("Workspace2D", 5, 8, 7);
    Workspace_sptr ws = boost::dynamic_pointer_cast<Workspace>(inWS);
    auto outWS = preview->view(ws);
    TS_ASSERT(outWS);
    TS_ASSERT_DIFFERS(outWS, inWS);
  }
};
