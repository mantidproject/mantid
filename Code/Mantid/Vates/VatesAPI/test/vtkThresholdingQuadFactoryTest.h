#ifndef VTK_THRESHOLDING_QUAD_FACTORY_TEST_H
#define VTK_THRESHOLDING_QUAD_FACTORY_TEST_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include "MantidAPI/IMDIterator.h"
#include "MantidVatesAPI/vtkThresholdingQuadFactory.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include "MDDataObjects/MDIndexCalculator.h"

using Mantid::VATES::vtkThresholdingQuadFactory;
const int dimension_size = 9;
class vtkThresholdingQuadFactoryTest: public CxxTest::TestSuite
{

private:

  ///Helper class. Usage as fake MDCell.
  class FakeCell : public Mantid::Geometry::SignalAggregate
  {
  public:
    FakeCell(){}
    virtual std::vector<Mantid::Geometry::coordinate> getVertexes() const
    {
      return std::vector<Mantid::Geometry::coordinate>(4);
    }
    virtual double getSignal() const
    {
      return 1; //Hard-coded signal value. Required for threshold checking in tests that follow.
    }
    virtual double getError() const
    {
      throw std::runtime_error("Not implemented");
    }
    virtual std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > getContributingPoints() const
    {
      throw std::runtime_error("Not implemented");
    }
    virtual ~FakeCell(){};
  };

  /// Fake iterator helper. Ideally would not need to provide this sort of implementation in a test.
  class FakeIterator : public Mantid::API::IMDIterator
  {
  private:
    Mantid::API::IMDWorkspace const * const m_mockWorkspace;
    size_t m_currentPointer;
  public:
    FakeIterator(Mantid::API::IMDWorkspace const * const mockWorkspace) : m_mockWorkspace(mockWorkspace), m_currentPointer(0)
    {
    }
    /// Get the size of the data
    size_t getDataSize()const
    {
      return dimension_size * dimension_size; 
    }
    double getCoordinate(int i)const
    {
      std::string id = m_mockWorkspace->getDimensionIDs()[i];
      std::vector<size_t> indexes;
      Mantid::MDDataObjects::MDWorkspaceIndexCalculator indexCalculator(2); //2d
      indexes.resize(indexCalculator.getNDimensions());
      indexCalculator.calculateDimensionIndexes(m_currentPointer,indexes);
      return m_mockWorkspace->getDimension(id)->getX(indexes[i]);
    }
    bool next()
    {
      m_currentPointer++;
      if(m_currentPointer < (dimension_size * dimension_size) )
      {
        return true;
      }
      return false;
      
    }
    size_t getPointer()const
    {
      return m_currentPointer;
    }
  };

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
    MOCK_CONST_METHOD4(getSignalNormalizedAt, double(size_t index1, size_t index2, size_t index3, size_t index4));
    virtual Mantid::API::IMDIterator* createIterator() const
    {
      return new FakeIterator(this);
    }

    const Mantid::Geometry::SignalAggregate& getCell(...) const
    {
      throw std::runtime_error("Not Implemented");
    }

    virtual ~MockIMDWorkspace() {}
  };

    /// Mock to allow the behaviour of the chain of responsibility to be tested.
  class MockvtkDataSetFactory : public Mantid::VATES::vtkDataSetFactory 
  {
  public:
    MOCK_CONST_METHOD0(create,
      vtkDataSet*());
    MOCK_CONST_METHOD0(createMeshOnly,
      vtkDataSet*());
    MOCK_CONST_METHOD0(createScalarArray,
      vtkFloatArray*());
    MOCK_METHOD1(initialize,
      void(boost::shared_ptr<Mantid::API::IMDWorkspace>));
    MOCK_METHOD1(SetSuccessor,
      void(vtkDataSetFactory* pSuccessor));
    MOCK_CONST_METHOD0(hasSuccessor,
      bool());
    MOCK_CONST_METHOD0(validate,
      void());
  };


public:

  void testCreateMeshOnlyThrows()
  {
    vtkThresholdingQuadFactory factory("signal");
    TS_ASSERT_THROWS(factory.createMeshOnly() , std::runtime_error);
  }

  void testCreateScalarArrayThrows()
  {
    vtkThresholdingQuadFactory factory("signal");
    TS_ASSERT_THROWS(factory.createScalarArray() , std::runtime_error);
  }

  void testIsValidThrowsWhenNoWorkspace()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::API;

    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);

    vtkThresholdingQuadFactory factory("signal");

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateWithoutInitializeThrows()
  {
    vtkThresholdingQuadFactory factory("signal");
    TS_ASSERT_THROWS(factory.create(), std::runtime_error);
  }

  void testInsideThresholds()
  {
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getXDimension()).Times(3).WillRepeatedly(Return(IMDDimension_const_sptr(new FakeIMDDimension("x"))));
    EXPECT_CALL(*pMockWs, getYDimension()).Times(3).WillRepeatedly(Return(IMDDimension_const_sptr(new FakeIMDDimension("y"))));
    FakeCell fakeCell; //FakeCell is hard-coded to return a signal value of 1.
    EXPECT_CALL(*pMockWs, getCell(_,_)).Times(AtLeast(1)).WillRepeatedly(ReturnRef(fakeCell));
    EXPECT_CALL(*pMockWs, getZDimension()).Times(0);
    EXPECT_CALL(*pMockWs, getTDimension()).Times(0);
    EXPECT_CALL(*pMockWs, getNumDims()).WillRepeatedly(Return(2));

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall between the minimum 0 and maximum 2.
    vtkThresholdingQuadFactory inside("signal", 0, 2);
    inside.initialize(ws_sptr);
    vtkUnstructuredGrid* insideProduct = dynamic_cast<vtkUnstructuredGrid*>(inside.create());

    TS_ASSERT_EQUALS((9*9), insideProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(100, insideProduct->GetNumberOfPoints());
  }

  void testAboveThreshold()
  {
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getXDimension()).Times(3).WillRepeatedly(Return(IMDDimension_const_sptr(new FakeIMDDimension("x"))));
    EXPECT_CALL(*pMockWs, getYDimension()).Times(3).WillRepeatedly(Return(IMDDimension_const_sptr(new FakeIMDDimension("y"))));
    FakeCell fakeCell; //FakeCell is hard-coded to return a signal value of 1.
    EXPECT_CALL(*pMockWs, getCell(_,_)).Times(AtLeast(1)).WillRepeatedly(ReturnRef(fakeCell));
    EXPECT_CALL(*pMockWs, getZDimension()).Times(0);
    EXPECT_CALL(*pMockWs, getTDimension()).Times(0);
    EXPECT_CALL(*pMockWs, getNumDims()).WillRepeatedly(Return(2));

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall above and outside the minimum 0 and maximum 0.5.
    vtkThresholdingQuadFactory above("signal", 0, 0.5);
    above.initialize(ws_sptr);
    vtkUnstructuredGrid* aboveProduct = dynamic_cast<vtkUnstructuredGrid*>(above.create());

    TS_ASSERT_EQUALS(0, aboveProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(100, aboveProduct->GetNumberOfPoints());
  }

  void testBelowThreshold()
  {
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getXDimension()).Times(3).WillRepeatedly(Return(IMDDimension_const_sptr(new FakeIMDDimension("x"))));
    EXPECT_CALL(*pMockWs, getYDimension()).Times(3).WillRepeatedly(Return(IMDDimension_const_sptr(new FakeIMDDimension("y"))));
    FakeCell fakeCell; //FakeCell is hard-coded to return a signal value of 1.
    EXPECT_CALL(*pMockWs, getCell(_,_)).Times(AtLeast(1)).WillRepeatedly(ReturnRef(fakeCell));
    EXPECT_CALL(*pMockWs, getZDimension()).Times(0);
    EXPECT_CALL(*pMockWs, getTDimension()).Times(0);
    EXPECT_CALL(*pMockWs, getNumDims()).WillRepeatedly(Return(2));

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall below and outside the minimum 1.5 and maximum 2.
    vtkThresholdingQuadFactory below("signal", 1.5, 2);
    below.initialize(ws_sptr);
    vtkUnstructuredGrid* belowProduct = dynamic_cast<vtkUnstructuredGrid*>(below.create());

    TS_ASSERT_EQUALS(0, belowProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(100, belowProduct->GetNumberOfPoints());
  }

  void testInitializationDelegates()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getNumDims()).Times(1).WillOnce(Return(1)); //1 dimensions on the workspace.

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkThresholdingQuadFactory factory("signal", (double)0);

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    
    factory.initialize(ws_sptr);

    TSM_ASSERT("Workspace not used as expected", Mock::VerifyAndClearExpectations(pMockWs));
    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testInitializationDelegatesThrows()
  {
    //If the workspace provided is not a 2D imdworkspace, it should call the successor's initalization. If there is no successor an exception should be thrown.
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getNumDims()).Times(1).WillOnce(Return(1)); //1 dimensions on the workspace.

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkThresholdingQuadFactory factory =("signal");

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor was available.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateDeleagates()
  {
    //If the workspace provided is not a 2D imdworkspace, it should call the successor's initalization
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getNumDims()).Times(2).WillRepeatedly(Return(1)); //1 dimensions on the workspace.

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, create()).Times(1); //expect it then to call create on the successor.

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkThresholdingQuadFactory factory("signal");

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    
    factory.initialize(ws_sptr);
    factory.create(); // should be called on successor.

    TSM_ASSERT("Workspace not used as expected", Mock::VerifyAndClearExpectations(pMockWs));
    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

};

#endif