#ifndef VTK_STRUCTURED_GRID_FACTORY_TEST_H_
#define VTK_STRUCTURED_GRID_FACTORY_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkStructuredGridFactory.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"


class vtkStructuredGridFactoryTest: public CxxTest::TestSuite
{
private:

  ///Helper class. Concrete instance of IMDDimension.
  class FakeIMDDimension: public Mantid::Geometry::IMDDimension
  {
  private:
    std::string m_id;
  public:
    FakeIMDDimension(std::string id) : m_id(id) {}
    std::string getName() const {throw std::runtime_error("Not implemented");}
    std::string getUnits() const {throw std::runtime_error("Not implemented");}
    std::string getDimensionId() const {return m_id;}
    double getMaximum() const {return 10;}
    double getMinimum() const {return 0;};
    size_t getNBins() const {return 10;};
    std::string toXMLString() const {throw std::runtime_error("Not implemented");};
    double getX(size_t) const {throw std::runtime_error("Not implemented");};
    virtual ~FakeIMDDimension()
    {
    }
  };

  /// Mock IMDDimension.
  class MockIMDWorkspace: public Mantid::API::IMDWorkspace
  {
  public:

    MOCK_CONST_METHOD0(id, const std::string());
    MOCK_CONST_METHOD0(getMemorySize, size_t());
    MOCK_CONST_METHOD1(getPoint,const Mantid::Geometry::SignalAggregate&(size_t index));
    MOCK_CONST_METHOD1(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment));
    MOCK_CONST_METHOD2(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment, size_t dim2Increment));
    MOCK_CONST_METHOD3(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment));
    MOCK_CONST_METHOD4(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment, size_t dim4Increment));

    MOCK_CONST_METHOD0(getWSLocation,std::string());
    MOCK_CONST_METHOD0(getGeometryXML,std::string());

    MOCK_CONST_METHOD0(getXDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD0(getYDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD0(getZDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD0(getTDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD1(getDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>(std::string id));
    MOCK_METHOD1(getDimensionNum,boost::shared_ptr<Mantid::Geometry::IMDDimension>(size_t index));
    MOCK_CONST_METHOD0(getDimensionIDs,const std::vector<std::string>());
    MOCK_CONST_METHOD0(getNPoints, uint64_t());
    MOCK_CONST_METHOD0(getNumDims, size_t());
    MOCK_CONST_METHOD4(getSignalAt, double(size_t index1, size_t index2, size_t index3, size_t index4));

    const Mantid::Geometry::SignalAggregate& getCell(...) const
    {
      throw std::runtime_error("Not Implemented");
    }

    virtual ~MockIMDWorkspace() {}
  };

public:

  void testCopy()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;
    TimeStepToTimeStep timeMapper;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getSignalAt(_, _, _, _)).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*pMockWs, getXDimension()).Times(8).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("x"))));
    EXPECT_CALL(*pMockWs, getYDimension()).Times(8).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("y"))));
    EXPECT_CALL(*pMockWs, getZDimension()).Times(8).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("z"))));

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkStructuredGridFactory<TimeStepToTimeStep> factoryA =
        vtkStructuredGridFactory<TimeStepToTimeStep> (ws_sptr, "signal", 0, timeMapper);

    vtkStructuredGridFactory<TimeStepToTimeStep> factoryB(factoryA);
    //Test factory copies indirectly via the products.
    vtkStructuredGrid* productA = factoryA.create();
    vtkStructuredGrid* productB = factoryB.create();

    TSM_ASSERT_EQUALS("Not copied correctly. Mesh data mismatch.", productA->GetNumberOfPoints(), productB->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Not copied correctly. Signal data mismatch.", std::string(productA->GetCellData()->GetArray(0)->GetName()), std::string(productB->GetCellData()->GetArray(0)->GetName()));
    productA->Delete();
    productB->Delete();
  }

  void testAssignment()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;
    TimeStepToTimeStep timeMapper;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getSignalAt(_, _, _, _)).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*pMockWs, getXDimension()).Times(8).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("x"))));
    EXPECT_CALL(*pMockWs, getYDimension()).Times(8).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("y"))));
    EXPECT_CALL(*pMockWs, getZDimension()).Times(8).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("z"))));

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkStructuredGridFactory<TimeStepToTimeStep> factoryA =
        vtkStructuredGridFactory<TimeStepToTimeStep> (ws_sptr, "signal", 0, timeMapper);

    vtkStructuredGridFactory<TimeStepToTimeStep> factoryB =
        vtkStructuredGridFactory<TimeStepToTimeStep> (ws_sptr, "other", 0, timeMapper);

    factoryB = factoryA;
    //Test factory assignments indirectly via the factory products.
    vtkStructuredGrid* productA = factoryA.create();
    vtkStructuredGrid* productB = factoryB.create();

    TSM_ASSERT_EQUALS("Not assigned correctly. Mesh data mismatch.", productA->GetNumberOfPoints(), productB->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Not assigned correctly. Signal data mismatch.", std::string(productA->GetCellData()->GetArray(0)->GetName()), std::string(productB->GetCellData()->GetArray(0)->GetName()));
    productA->Delete();
    productB->Delete();
  }

  void testMeshOnly()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;
    TimeStepToTimeStep timeMapper;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getSignalAt(_, _, _, _)).Times(0); //Shouldn't access getSignal At
    EXPECT_CALL(*pMockWs, getXDimension()).Times(3).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("x"))));
    EXPECT_CALL(*pMockWs, getYDimension()).Times(3).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("y"))));
    EXPECT_CALL(*pMockWs, getZDimension()).Times(3).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("z"))));
    EXPECT_CALL(*pMockWs, getTDimension()).Times(0); //Shouldn't consult time dimension.

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkStructuredGridFactory<TimeStepToTimeStep> factory =
        vtkStructuredGridFactory<TimeStepToTimeStep>::constructAsMeshOnly(ws_sptr, timeMapper);

    //Invoke's mocked methods on MockIMDWorkspace.
    vtkStructuredGrid* product = factory.createMeshOnly();

    int predictedNPoints = (10 + 1) * (10 + 1) * (10 + 1);
    TSM_ASSERT_EQUALS("Wrong number of points generated", predictedNPoints, product->GetNumberOfPoints());
    TSM_ASSERT("This is not a mesh-only product.", testing::Mock::VerifyAndClearExpectations(pMockWs));
    product->Delete();
  }

  void testMeshOnlyCausesThrow()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;
    TimeStepToTimeStep timeMapper;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkStructuredGridFactory<TimeStepToTimeStep> factory =
        vtkStructuredGridFactory<TimeStepToTimeStep>::constructAsMeshOnly(ws_sptr, timeMapper);

    TSM_ASSERT_THROWS("Cannot access non-mesh information when factory constructed as mesh-only", factory.createScalarArray(), std::runtime_error);
  }

  void testSignalAspects()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;
    TimeStepToTimeStep timeMapper;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getSignalAt(_, _, _, _)).WillRepeatedly(Return(1)); //Shouldn't access getSignal At
    EXPECT_CALL(*pMockWs, getXDimension()).Times(AtLeast(1)).WillRepeatedly(Return(
        IMDDimension_const_sptr(new FakeIMDDimension("x"))));
    EXPECT_CALL(*pMockWs, getYDimension()).Times(AtLeast(1)).WillRepeatedly(Return(
        IMDDimension_const_sptr(new FakeIMDDimension("y"))));
    EXPECT_CALL(*pMockWs, getZDimension()).Times(AtLeast(1)).WillRepeatedly(Return(
        IMDDimension_const_sptr(new FakeIMDDimension("z"))));
    EXPECT_CALL(*pMockWs, getTDimension()).Times(0);

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkStructuredGridFactory<TimeStepToTimeStep> factory =
        vtkStructuredGridFactory<TimeStepToTimeStep> (ws_sptr, "signal", 1, timeMapper);

    vtkDataSet* product = factory.create();
    TSM_ASSERT_EQUALS("A single array should be present on the product dataset.", 1, product->GetCellData()->GetNumberOfArrays());
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    TSM_ASSERT_EQUALS("The obtained cell data has the wrong name.", std::string("signal"), signalData->GetName());
    const int correctCellNumber = (10) * (10) * (10);
    TSM_ASSERT_EQUALS("The number of signal values generated is incorrect.", correctCellNumber, signalData->GetSize());
    product->Delete();
  }

  void testIsVtkDataSetFactory()
  {
    //TODO
  }
};

#endif
