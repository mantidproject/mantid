#ifndef COMOPOSITE_PEAKS_PRESENTER_VSI_TEST_H_
#define COMOPOSITE_PEAKS_PRESENTER_VSI_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/CompositePeaksPresenterVsi.h"
#include "MantidVatesAPI/ConcretePeaksPresenterVsi.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"


#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <stdexcept>

#include "MockObjects.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>


#include <string>

using namespace ::testing;
using namespace Mantid::VATES;

class MockPeaksWorkspaceComposite : public Mantid::DataObjects::PeaksWorkspace
{
public:
};

class CompositePeaksPresenterVsiTest : public CxxTest::TestSuite {
public:
  void testSetupPresenterCorrectly() {

  }

  void testThatGettingPeaksWorkspaceDirectlyIsNotAllowed() {
    // Arrange
    CompositePeaksPresenterVsi presenter;
    // Assert
    TS_ASSERT_THROWS(presenter.getPeaksWorkspace(), std::runtime_error);
  }

  void testThatGettingPeaksWorkspaceNameDirectlyIsNotAllowed() {
    // Arrange
    CompositePeaksPresenterVsi presenter;
    // Assert
    TS_ASSERT_THROWS(presenter.getPeaksWorkspaceName(), std::runtime_error);
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
    NearPlane nearPlane(0.0, 0.0, -1.0,1.0);
    ViewFrustum_const_sptr frustum = boost::make_shared<const Mantid::VATES::ViewFrustum>(left, right, bottom, top, farPlane, nearPlane);

    boost::shared_ptr<MockPeaksWorkspaceComposite> pw_ptr(new MockPeaksWorkspaceComposite());
    
    std::string name = "pw1";
    PeaksPresenterVsi_sptr p1(new ConcretePeaksPresenterVsi(pw_ptr, frustum, frame));


    boost::shared_ptr<MockPeaksWorkspaceComposite> pw_ptr2(new MockPeaksWorkspaceComposite());
    std::string name2 = "pw2";
    PeaksPresenterVsi_sptr p2(new ConcretePeaksPresenterVsi(pw_ptr2, frustum, frame));

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
    NearPlane nearPlane(0.0, 0.0, -1.0,1.0);
    ViewFrustum_const_sptr frustum = boost::make_shared<const Mantid::VATES::ViewFrustum>(left, right, bottom, top, farPlane, nearPlane);

    boost::shared_ptr<MockPeaksWorkspaceComposite> pw_ptr(new MockPeaksWorkspaceComposite());
    std::string name = "pw1";
    PeaksPresenterVsi_sptr p1(new ConcretePeaksPresenterVsi(pw_ptr, frustum, frame));

    boost::shared_ptr<MockPeaksWorkspaceComposite> pw_ptr2(new MockPeaksWorkspaceComposite());
    std::string name2 = "pw2";
    PeaksPresenterVsi_sptr p2(new ConcretePeaksPresenterVsi(pw_ptr2, frustum, frame));

    presenter.addPresenter(p1);
    presenter.addPresenter(p2);

    // Act
    std::vector<Mantid::API::IPeaksWorkspace_sptr> ws = presenter.getPeaksWorkspaces();

    // Assert
    TSM_ASSERT_EQUALS("Should have two entries", ws.size(), 2);
  }
};

#endif