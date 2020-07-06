// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidVatesAPI/CompositePeaksPresenterVsi.h"
#include "MantidVatesAPI/ConcretePeaksPresenterVsi.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include <cxxtest/TestSuite.h>

#include <memory>
#include <stdexcept>

#include "MockObjects.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

using namespace ::testing;
using namespace Mantid::VATES;

class MockPeaksWorkspaceComposite : public Mantid::DataObjects::PeaksWorkspace {
public:
};

class CompositePeaksPresenterVsiTest : public CxxTest::TestSuite {
public:
  void testSetupPresenterCorrectly() {}

  void testThatGettingPeaksWorkspaceDirectlyIsNotAllowed() {
    // Arrange
    CompositePeaksPresenterVsi presenter;
    // Assert
    TS_ASSERT_THROWS(presenter.getPeaksWorkspace(), const std::runtime_error &);
  }

  void testThatGettingPeaksWorkspaceNameDirectlyIsNotAllowed() {
    // Arrange
    CompositePeaksPresenterVsi presenter;
    // Assert
    TS_ASSERT_THROWS(presenter.getPeaksWorkspaceName(),
                     const std::runtime_error &);
  }

  void testThatGetListOfNamesOfSubPresenters() {
    // Arrange
    CompositePeaksPresenterVsi presenter;

    std::string frame = "testFrame";

    LeftPlane left(1.0, 0.0, 0.0, 1.0);

    RightPlane right(-1.0, 0.0, 0.0, 1.0);
    BottomPlane bottom(0.0, 1.0, 0.0, 1.0);
    TopPlane top(0.0, -1.0, 0.0, 1.0);
    FarPlane farPlane(0.0, 0.0, 1.0, 1.0);
    NearPlane nearPlane(0.0, 0.0, -1.0, 1.0);
    ViewFrustum_const_sptr frustum =
        std::make_shared<const Mantid::VATES::ViewFrustum>(
            left, right, bottom, top, farPlane, nearPlane);

    std::shared_ptr<MockPeaksWorkspaceComposite> pw_ptr =
        std::make_shared<MockPeaksWorkspaceComposite>();

    std::string name = "pw1";
    PeaksPresenterVsi_sptr p1(
        new ConcretePeaksPresenterVsi(pw_ptr, frustum, frame));

    std::shared_ptr<MockPeaksWorkspaceComposite> pw_ptr2 =
        std::make_shared<MockPeaksWorkspaceComposite>();
    std::string name2 = "pw2";
    PeaksPresenterVsi_sptr p2(
        new ConcretePeaksPresenterVsi(pw_ptr2, frustum, frame));

    presenter.addPresenter(p1);
    presenter.addPresenter(p2);

    // Act
    std::vector<std::string> wsNames = presenter.getPeaksWorkspaceNames();

    // Assert, cannot mock the getName function as it is not virtual
    TSM_ASSERT_EQUALS("Should have two entries", wsNames.size(), 2);
  }

  void testThatGetsAllPeaksWorkspaces() {
    // Arrange
    CompositePeaksPresenterVsi presenter;

    std::string frame = "testFrame";

    LeftPlane left(1.0, 0.0, 0.0, 1.0);

    RightPlane right(-1.0, 0.0, 0.0, 1.0);
    BottomPlane bottom(0.0, 1.0, 0.0, 1.0);
    TopPlane top(0.0, -1.0, 0.0, 1.0);
    FarPlane farPlane(0.0, 0.0, 1.0, 1.0);
    NearPlane nearPlane(0.0, 0.0, -1.0, 1.0);
    ViewFrustum_const_sptr frustum =
        std::make_shared<const Mantid::VATES::ViewFrustum>(
            left, right, bottom, top, farPlane, nearPlane);

    std::shared_ptr<MockPeaksWorkspaceComposite> pw_ptr =
        std::make_shared<MockPeaksWorkspaceComposite>();
    std::string name = "pw1";
    PeaksPresenterVsi_sptr p1(
        new ConcretePeaksPresenterVsi(pw_ptr, frustum, frame));

    std::shared_ptr<MockPeaksWorkspaceComposite> pw_ptr2 =
        std::make_shared<MockPeaksWorkspaceComposite>();
    std::string name2 = "pw2";
    PeaksPresenterVsi_sptr p2(
        new ConcretePeaksPresenterVsi(pw_ptr2, frustum, frame));

    presenter.addPresenter(p1);
    presenter.addPresenter(p2);

    // Act
    std::vector<Mantid::API::IPeaksWorkspace_sptr> ws =
        presenter.getPeaksWorkspaces();

    // Assert
    TSM_ASSERT_EQUALS("Should have two entries", ws.size(), 2);
  }
};
