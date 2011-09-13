#ifndef VATESAPI_TEST_MOCKOBJECTS_H
#define VATESAPI_TEST_MOCKOBJECTS_H

#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidVatesAPI/Clipper.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/MDRebinningView.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/RebinningActionManager.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MDDataObjects/MDIndexCalculator.h"
#include <gmock/gmock.h>
#include <vtkFieldData.h>
#include <vtkCharArray.h>

using Mantid::VATES::MDRebinningView;
using Mantid::Geometry::MDHistoDimension;
using Mantid::Geometry::MDHistoDimension_sptr;

//=====================================================================================
// Test Helper Types. These are shared by several tests in VatesAPI
//=====================================================================================
namespace
{


const int dimension_size = 9;


//=================================================================================================
///Helper class peforms no transformation.
class NullTransform : public Mantid::API::CoordTransform
{
public:
  NullTransform() : Mantid::API::CoordTransform(3, 3){}
  std::string toXMLString() const { throw std::runtime_error("Not Implemented");}
  void apply(const Mantid::coord_t * inputVector, Mantid::coord_t * outVector) const
  {
    for(size_t i = 0; i < 3; i++)
    {
      outVector[i] = inputVector[i];
    }
  }
};

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
    std::string id = m_mockWorkspace->getDimensionNum(i)->getDimensionId();
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

  MOCK_CONST_METHOD0(getNPoints, uint64_t());
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
  MOCK_METHOD1(setRecursionDepth, void(size_t));
};

//=================================================================================================
/// Mock to allow the behaviour of MDLoadingPresenters to be tested.
class MockMDLoadingView : public Mantid::VATES::MDLoadingView
{
public:
  MOCK_CONST_METHOD0(getTime, double());
  MOCK_CONST_METHOD0(getRecursionDepth, size_t());
  MOCK_CONST_METHOD0(getLoadInMemory, bool());
  MOCK_METHOD1(updateAlgorithmProgress, void(double));
  ~MockMDLoadingView(){}
};

class MockMDRebinningView : public MDRebinningView 
{
public:
  MOCK_CONST_METHOD0(getImplicitFunction,
    vtkImplicitFunction*());
  MOCK_CONST_METHOD0(getMaxThreshold,
    double());
  MOCK_CONST_METHOD0(getMinThreshold,
    double());
  MOCK_CONST_METHOD0(getApplyClip,
    bool());
  MOCK_CONST_METHOD0(getTimeStep,
    double());
  MOCK_CONST_METHOD0(getAppliedGeometryXML,
    const char*());
  MOCK_METHOD1(updateAlgorithmProgress,
    void(double));
  MOCK_CONST_METHOD0(getWidth, double());
};

class MockClipper: public Mantid::VATES::Clipper
{
public:
  MOCK_METHOD1(SetInput, void(vtkDataSet* in_ds));
  MOCK_METHOD1(SetClipFunction, void(vtkImplicitFunction* func));
  MOCK_METHOD1(SetInsideOut, void(bool insideout));
  MOCK_METHOD1(SetRemoveWholeCells, void(bool removeWholeCells));
  MOCK_METHOD1(SetOutput, void(vtkUnstructuredGrid* out_ds));
  MOCK_METHOD0(Update, void());
  MOCK_METHOD0(Delete,void());
  MOCK_METHOD0(GetOutput, vtkDataSet*());
  MOCK_METHOD0(die, void());
  virtual ~MockClipper(){}
};

class MockRebinningActionManager : public Mantid::VATES::RebinningActionManager
{
public:
  MOCK_METHOD1(ask, void(Mantid::VATES::RebinningIterationAction));
  MOCK_CONST_METHOD0(action, Mantid::VATES::RebinningIterationAction());
  MOCK_METHOD0(reset, void());
  virtual ~MockRebinningActionManager(){}
};


class MockWorkspaceProvider : public Mantid::VATES::WorkspaceProvider
{
public:
  MOCK_CONST_METHOD1(canProvideWorkspace, bool(std::string));
  MOCK_CONST_METHOD1(fetchWorkspace, Mantid::API::Workspace_sptr(std::string));
  MOCK_CONST_METHOD1(disposeWorkspace, void(std::string));
  ~MockWorkspaceProvider(){}
};

class FakeProgressAction : public Mantid::VATES::ProgressAction
{
  virtual void eventRaised(double)
  {
  }
};

Mantid::MDEvents::MDHistoWorkspace_sptr getFakeMDHistoWorkspace(double signal, size_t numDims, size_t numBins = 10)
{
  Mantid::MDEvents::MDHistoWorkspace * ws;
  if (numDims == 3)
  {
    ws = new Mantid::MDEvents::MDHistoWorkspace(
        MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, 10.0, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("y","y","m", 0.0, 10.0, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("z","z","m", 0.0, 10.0, numBins))   );
  }
  else if (numDims == 4)
  {
    ws = new Mantid::MDEvents::MDHistoWorkspace(
        MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, 10.0, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("y","y","m", 0.0, 10.0, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("z","z","m", 0.0, 10.0, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("t","z","m", 0.0, 10.0, numBins))
        );
  }
  Mantid::MDEvents::MDHistoWorkspace_sptr ws_sptr(ws);
  ws_sptr->setTo(signal, signal);
  return ws_sptr;
}

  vtkFieldData* createFieldDataWithCharArray(std::string testData)
  {
    vtkFieldData* fieldData = vtkFieldData::New();
    vtkCharArray* charArray = vtkCharArray::New();
    charArray->SetName(Mantid::VATES::XMLDefinitions::metaDataId().c_str());
    charArray->Allocate(100);
    for(unsigned int i = 0; i < testData.size(); i++)
    {
      char cNextVal = testData.at(i);
      if(int(cNextVal) > 1)
      {
        charArray->InsertNextValue(cNextVal);

      }
    }
    fieldData->AddArray(charArray);
    charArray->Delete();
    return fieldData;
  }

  std::string constrctGeometryOnlyXML(const std::string& xDimensionIdMapping, const std::string& yDimensionIdMapping, const std::string& zDimensionIdMapping, const std::string& tDimensionIdMapping
    ,std::string xBins = "10",
    std::string yBins = "10",
    std::string zBins = "10",
    std::string tBins = "10"
    )
  {
    std::string body = std::string("<DimensionSet>") +
      "<Dimension ID=\"en\">" +
      "<Name>Energy</Name>" +
      "<Units>None</Units>" +
      "<UpperBounds>150.0000</UpperBounds>" +
      "<LowerBounds>0.0000</LowerBounds>" +
      "<NumberOfBins>" + xBins + "</NumberOfBins>" +
      "</Dimension>" +
      "<Dimension ID=\"qx\">" +
      "<Name>Qx</Name>" +
      "<Units>None</Units>" +
      "<UpperBounds>5.0000</UpperBounds>" +
      "<LowerBounds>-1.5000</LowerBounds>" +
      "<NumberOfBins>" + yBins + "</NumberOfBins>" +
      "</Dimension>" +
      "<Dimension ID=\"qy\">" +
      "<Name>Qy</Name>" +
      "<Units>None</Units>" +
      "<UpperBounds>6.6000</UpperBounds>" +
      "<LowerBounds>-6.6000</LowerBounds>" +
      "<NumberOfBins>" + zBins  + "</NumberOfBins>" +
      "</Dimension>" +
      "<Dimension ID=\"qz\">" +
      "<Name>Qz</Name>" +
      "<Units>None</Units>" +
      "<UpperBounds>6.6000</UpperBounds>" +
      "<LowerBounds>-6.6000</LowerBounds>" +
      "<NumberOfBins>" + tBins + "</NumberOfBins>" +
      "</Dimension>" +
      "<XDimension>" +
      "<RefDimensionId>" +
      xDimensionIdMapping +
      "</RefDimensionId>" +
      "</XDimension>" +
      "<YDimension>" +
      "<RefDimensionId>" +
      yDimensionIdMapping +
      "</RefDimensionId>" +
      "</YDimension>" +
      "<ZDimension>" +
      "<RefDimensionId>" + 
      zDimensionIdMapping +
      "</RefDimensionId>" +
      "</ZDimension>" +
      "<TDimension>" +
      "<RefDimensionId>" +
      tDimensionIdMapping +
      "</RefDimensionId>" +
      "</TDimension>" +
      "</DimensionSet>";
    return body;
  }

  std::string constructXML(const std::string& xDimensionIdMapping, const std::string& yDimensionIdMapping, const std::string& zDimensionIdMapping, const std::string& tDimensionIdMapping)
  {
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
      "<MDInstruction>" +
      "<MDWorkspaceName>Input</MDWorkspaceName>" +
      "<MDWorkspaceLocation>test_horace_reader.sqw</MDWorkspaceLocation>" +
      constrctGeometryOnlyXML(xDimensionIdMapping, yDimensionIdMapping, zDimensionIdMapping, tDimensionIdMapping) +
      "<Function>" +
      "<Type>CompositeImplicitFunction</Type>" +
      "<ParameterList/>" +
      "<Function>" +
      "<Type>BoxImplicitFunction</Type>" +
      "<ParameterList>" +
      "<Parameter>" +
      "<Type>HeightParameter</Type>" +
      "<Value>6</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>WidthParameter</Type>" +
      "<Value>1.5</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>DepthParameter</Type>" +
      "<Value>6</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>OriginParameter</Type>" +
      "<Value>0, 0, 0</Value>" +
      "</Parameter>" +
      "</ParameterList>" +
      "</Function>" +
      "<Function>" +
      "<Type>CompositeImplicitFunction</Type>" +
      "<ParameterList/>" +
      "<Function>" +
      "<Type>BoxImplicitFunction</Type>" +
      "<ParameterList>" +
      "<Parameter>" +
      "<Type>WidthParameter</Type>" +
      "<Value>4</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>HeightParameter</Type>" +
      "<Value>1.5</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>DepthParameter</Type>" +
      "<Value>6</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>OriginParameter</Type>" +
      "<Value>0, 0, 0</Value>" +
      "</Parameter>" +
      "</ParameterList>" +
      "</Function>" +
      "</Function>" +
      "</Function>" +
      "</MDInstruction>";
  }

} // namespace

#endif
