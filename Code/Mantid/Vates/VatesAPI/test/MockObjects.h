#ifndef VATESAPI_TEST_MOCKOBJECTS_H
#define VATESAPI_TEST_MOCKOBJECTS_H

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MDDataObjects/MDIndexCalculator.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidVatesAPI/MDRebinningView.h"
#include "MantidVatesAPI/Clipper.h"
#include <gmock/gmock.h>

using Mantid::VATES::MDRebinningView;

//=====================================================================================
// Test Helper Types. These are shared by several tests in VatesAPI
//=====================================================================================
namespace
{


const int dimension_size = 9;


//=================================================================================================
///Helper class. Usage as fake MDCell.
class FakeCell : public Mantid::Geometry::SignalAggregate
{
public:
  FakeCell(){}
  virtual std::vector<Mantid::Geometry::Coordinate> getVertexes() const
  {
    return std::vector<Mantid::Geometry::Coordinate>(4);
  }
  virtual Mantid::signal_t getSignal() const
  {
    return 1; //Hard-coded signal value. Required for threshold checking in tests that follow.
  }
  virtual Mantid::signal_t getError() const
  {
    throw std::runtime_error("Not implemented");
  }
  virtual std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > getContributingPoints() const
  {
    throw std::runtime_error("Not implemented");
  }
  virtual ~FakeCell(){};
};

//=================================================================================================
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
  double getCoordinate(size_t i)const
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


//=================================================================================================
///Helper class. Concrete instance of IMDDimension.
class FakeIMDDimension: public Mantid::Geometry::IMDDimension
{
private:
  std::string m_id;
  const unsigned int m_nbins;
public:
  FakeIMDDimension(std::string id, unsigned int nbins=10) : m_id(id), m_nbins(nbins) {}
  std::string getName() const {throw std::runtime_error("Not implemented");}
  std::string getUnits() const {throw std::runtime_error("Not implemented");}
  std::string getDimensionId() const {return m_id;}
  double getMaximum() const {return 10;}
  double getMinimum() const {return 0;};
  size_t getNBins() const {return m_nbins;};
  std::string toXMLString() const {throw std::runtime_error("Not implemented");};
  double getX(size_t) const {throw std::runtime_error("Not implemented");};
  virtual ~FakeIMDDimension()
  {
  }
};

//=================================================================================================
/// Concrete mocked implementation of IMDWorkspace for testing.
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
  MOCK_CONST_METHOD1(getSignalNormalizedAt, Mantid::signal_t(size_t index1));
  MOCK_CONST_METHOD2(getSignalNormalizedAt, double(size_t index1, size_t index2));
  MOCK_CONST_METHOD3(getSignalNormalizedAt, double(size_t index1, size_t index2, size_t index3));
  MOCK_CONST_METHOD4(getSignalNormalizedAt, double(size_t index1, size_t index2, size_t index3, size_t index4));
  MOCK_CONST_METHOD0(getNonIntegratedDimensions, Mantid::Geometry::VecIMDDimension_const_sptr());

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



//=================================================================================================
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
    void(Mantid::API::Workspace_sptr));
  MOCK_METHOD1(SetSuccessor,
    void(vtkDataSetFactory* pSuccessor));
  MOCK_CONST_METHOD0(hasSuccessor,
    bool());
  MOCK_CONST_METHOD0(validate,
    void());
  MOCK_CONST_METHOD0(getFactoryTypeName, std::string());

};


} // namespace

#endif
