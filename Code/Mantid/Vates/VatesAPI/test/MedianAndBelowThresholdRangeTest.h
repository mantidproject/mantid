#ifndef MEDIAN_AND_BELOW_THRESHOLD_RANGE_TEST_H_
#define MEDIAN_AND_BELOW_THRESHOLD_RANGE_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MantidVatesAPI/MedianAndBelowThresholdRange.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"

using namespace Mantid;
using namespace testing;

//=====================================================================================
// Helper Fake/Mock types
//=====================================================================================
namespace
{

  //Helper type. Fake-out of an Iterator for the Mock workspace (see below) to return.
  class FakeIterator : public Mantid::API::IMDIterator
  {
  private:
    Mantid::API::IMDWorkspace const * const m_mockWorkspace;
    size_t m_currentPointer;
    size_t m_nCells;
  public:
    FakeIterator(Mantid::API::IMDWorkspace const * const mockWorkspace, size_t nCells) : m_mockWorkspace(mockWorkspace), m_currentPointer(-1), m_nCells(nCells)
    {
    }
    /// Get the size of the data
    size_t getDataSize()const
    {
      return m_nCells;
    }
    double getCoordinate(size_t)const
    {
      throw std::runtime_error("Not used during test");
    }
    bool next()
    {
      m_currentPointer++;
      if(m_currentPointer < getDataSize())
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

  //Helper type. Fake-out of a SignalAggregate for the Mock workspace (see below) to return.
  class FakeCell : public Mantid::Geometry::SignalAggregate
  {
  private:
    signal_t m_signalValue;
  public:
    FakeCell(signal_t signalValue=0) : m_signalValue(signalValue)
    {
    }
    void set(const signal_t otherValue)
    {
      m_signalValue = otherValue;
    }
    signal_t getSignal() const
    {
      return m_signalValue;
    }
    virtual std::vector<Mantid::Geometry::Coordinate> getVertexes() const{throw std::runtime_error("Not Implemented");}
    virtual signal_t getError() const{throw std::runtime_error("Not Implemented");}
    virtual std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > getContributingPoints() const{throw std::runtime_error("Not Implemented");}
    virtual ~FakeCell(){}; 
  };

  //Helper type. Mock IMDWorkspace.
  class MockIMDWorkspace: public Mantid::API::IMDWorkspace
  {
  private:
    size_t m_nCells;
  public:
    MockIMDWorkspace(size_t nCells) : m_nCells(nCells)
    {
    }
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
    MOCK_CONST_METHOD1(getSignalNormalizedAt, signal_t(size_t index1));
    MOCK_CONST_METHOD0(getNonIntegratedDimensions, Mantid::Geometry::VecIMDDimension_const_sptr());

    virtual Mantid::API::IMDIterator* createIterator() const
    {
      return new FakeIterator(this, m_nCells);
    }

    const Mantid::Geometry::SignalAggregate& getCell(...) const
    {
      throw std::runtime_error("Not Implemented");
    }

    virtual ~MockIMDWorkspace() {}
  };
}

//=====================================================================================
// Functional tests
//=====================================================================================

class MedianAndBelowThresholdRangeTest: public CxxTest::TestSuite
{
  private:

  FakeCell cellOne;
  FakeCell cellTwo;
  FakeCell cellThree;
  FakeCell cellFour;
  FakeCell cellFive;
  FakeCell cellSix;
  FakeCell cellSeven;
  FakeCell cellEight;

  public :

  void setUp()
  {
    //Set up a standard set of values for subsequent tests. Note that the following set gives a standard deviation of +/-2
    cellOne.set(-1);
    cellTwo.set(2);
    cellThree.set(2);
    cellFour.set(3);
    cellFive.set(4);
    cellSix.set(5);
    cellSeven.set(6);
    cellEight.set(7);
  }

  void testMedianCalculation()
  {
    MockIMDWorkspace* pWs = new MockIMDWorkspace(8);
    Mantid::API::IMDWorkspace_sptr sptrWs(pWs);

    EXPECT_CALL(*pWs, getCell(0)).WillRepeatedly(ReturnRef(cellOne));
    EXPECT_CALL(*pWs, getCell(1)).WillOnce(ReturnRef(cellTwo));
    EXPECT_CALL(*pWs, getCell(2)).WillOnce(ReturnRef(cellThree));
    EXPECT_CALL(*pWs, getCell(3)).WillOnce(ReturnRef(cellFour));
    EXPECT_CALL(*pWs, getCell(4)).WillOnce(ReturnRef(cellFive));
    EXPECT_CALL(*pWs, getCell(5)).WillOnce(ReturnRef(cellSix));
    EXPECT_CALL(*pWs, getCell(6)).WillOnce(ReturnRef(cellSeven));
    EXPECT_CALL(*pWs, getCell(7)).WillOnce(ReturnRef(cellEight));

    Mantid::VATES::MedianAndBelowThresholdRange medianCalculator;
    medianCalculator.setWorkspace(sptrWs);
    medianCalculator.calculate();
    //-1 + 2 + 2 + 3 + 4 + 5 + 6 + 7 / 8 = 3.5

    TSM_ASSERT_EQUALS("Wrong maximum value.", 3.5, medianCalculator.getMaximum());
    TSM_ASSERT_EQUALS("Wrong minimum value.", -1, medianCalculator.getMinimum());
  }

  void testInRange()
  {
    MockIMDWorkspace* pWs = new MockIMDWorkspace(8);
    Mantid::API::IMDWorkspace_sptr sptrWs(pWs);

    EXPECT_CALL(*pWs, getCell(0)).WillRepeatedly(ReturnRef(cellOne));
    EXPECT_CALL(*pWs, getCell(1)).WillOnce(ReturnRef(cellTwo));
    EXPECT_CALL(*pWs, getCell(2)).WillOnce(ReturnRef(cellThree));
    EXPECT_CALL(*pWs, getCell(3)).WillOnce(ReturnRef(cellFour));
    EXPECT_CALL(*pWs, getCell(4)).WillOnce(ReturnRef(cellFive));
    EXPECT_CALL(*pWs, getCell(5)).WillOnce(ReturnRef(cellSix));
    EXPECT_CALL(*pWs, getCell(6)).WillOnce(ReturnRef(cellSeven));
    EXPECT_CALL(*pWs, getCell(7)).WillOnce(ReturnRef(cellEight));

    Mantid::VATES::MedianAndBelowThresholdRange medianCalculator;
    medianCalculator.setWorkspace(sptrWs);
    medianCalculator.calculate();
    //0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 / 8 = 3.5

    TS_ASSERT_EQUALS(true, medianCalculator.inRange(3.499));
    TS_ASSERT_EQUALS(false, medianCalculator.inRange(3.501));
  }

};

#endif