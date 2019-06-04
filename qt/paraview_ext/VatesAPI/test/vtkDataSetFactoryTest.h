// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef VTKDATASETFACTORYTEST_H_
#define VTKDATASETFACTORYTEST_H_

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace testing;
using Mantid::VATES::vtkDataSetFactory;

class vtkDataSetFactoryTest : public CxxTest::TestSuite {
private:
  /// Mocked helper type
  class MockvtkDataSetFactory : public Mantid::VATES::vtkDataSetFactory {
  public:
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD1(
        create, vtkSmartPointer<vtkDataSet>(Mantid::VATES::ProgressAction &));
    MOCK_METHOD1(initialize,
                 void(const boost::shared_ptr<Mantid::API::Workspace> &));
    MOCK_CONST_METHOD0(validate, void());
    MOCK_CONST_METHOD0(getFactoryTypeName, std::string());
    GNU_DIAG_ON_SUGGEST_OVERRIDE
    void setSuccessorConcrete(std::unique_ptr<vtkDataSetFactory> pSuccessor) {
      vtkDataSetFactory::setSuccessor(pSuccessor);
    }
    bool hasSuccessorConcrete() const {
      return vtkDataSetFactory::hasSuccessor();
    }
  };

  /// Fake helper type.
  class FakeProgressAction : public Mantid::VATES::ProgressAction {
    void eventRaised(double) override {}
  };

public:
  void testSetSuccessor() {
    MockvtkDataSetFactory factory;
    auto successor = new MockvtkDataSetFactory();
    auto uniqueSuccessor = std::unique_ptr<MockvtkDataSetFactory>(successor);

    EXPECT_CALL(factory, getFactoryTypeName())
        .WillOnce(testing::Return("TypeA"));
    EXPECT_CALL(*successor, getFactoryTypeName())
        .WillOnce(testing::Return("TypeB")); // Different type name, so setting
                                             // the successor should work.
    factory.setSuccessor(std::move(uniqueSuccessor));

    TSM_ASSERT("Successor should have been set", factory.hasSuccessor());
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&factory));
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(successor));
  }

  void testSetSuccessorThrows() {
    MockvtkDataSetFactory factory;
    auto successor = new MockvtkDataSetFactory();
    auto uniqueSuccessor = std::unique_ptr<MockvtkDataSetFactory>(successor);
    EXPECT_CALL(factory, getFactoryTypeName())
        .WillOnce(testing::Return("TypeA"));
    EXPECT_CALL(*successor, getFactoryTypeName())
        .WillOnce(testing::Return("TypeA")); // Same type name. should NOT work.
    TSM_ASSERT_THROWS("By default, should throw when successor type is the "
                      "same as the container.",
                      factory.setSuccessor(std::move(uniqueSuccessor)),
                      const std::runtime_error &);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&factory));
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(successor));
  }

  void testEnumValues() {
    using Mantid::VATES::vtkDataSetFactory;
    int oneD = vtkDataSetFactory::OneDimensional;
    int twoD = vtkDataSetFactory::TwoDimensional;
    int threeD = vtkDataSetFactory::ThreeDimensional;
    int fourD = vtkDataSetFactory::FourDimensional;
    TS_ASSERT_EQUALS(1, oneD);
    TS_ASSERT_EQUALS(2, twoD);
    TS_ASSERT_EQUALS(3, threeD);
    TS_ASSERT_EQUALS(4, fourD);
  }

  void testCheckDimensionalityByDefault() {
    MockvtkDataSetFactory factory;
    TS_ASSERT(factory.doesCheckDimensionality());
  }

  void testSetCheckDimensionality() {
    MockvtkDataSetFactory factory;
    factory.setCheckDimensionality(false);
    TS_ASSERT(!factory.doesCheckDimensionality());
    factory.setCheckDimensionality(true);
    TS_ASSERT(factory.doesCheckDimensionality());
  }

  void testOneStepCreate() {
    FakeProgressAction progressUpdater;

    MockvtkDataSetFactory factory;
    EXPECT_CALL(factory, initialize(_)).Times(1);
    EXPECT_CALL(factory, create(Ref(progressUpdater)))
        .Times(1)
        .WillOnce(Return(vtkSmartPointer<vtkStructuredGrid>::New()));

    IMDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);
    auto product = factory.oneStepCreate(ws_sptr, progressUpdater);
    TS_ASSERT(product != nullptr);
    TSM_ASSERT_EQUALS("Output not wired up correctly to ::create() method",
                      "vtkStructuredGrid",
                      std::string(product->GetClassName()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }
};

#endif
