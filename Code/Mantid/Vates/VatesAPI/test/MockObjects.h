#ifndef VATESAPI_TEST_MOCKOBJECTS_H
#define VATESAPI_TEST_MOCKOBJECTS_H

//#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/UnitLabel.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/VatesXMLDefinitions.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidAPI/FrameworkManager.h"
#include <gmock/gmock.h>
#include <vtkFieldData.h>
#include <vtkCharArray.h>
#include <vtkStringArray.h>

using Mantid::Geometry::MDHistoDimension;
using Mantid::Geometry::MDHistoDimension_sptr;
using Mantid::coord_t;

//=====================================================================================
// Test Helper Types. These are shared by several tests in VatesAPI
//=====================================================================================
namespace
{


const int dimension_size = 9;



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
  const Mantid::Kernel::UnitLabel getUnits() const {throw std::runtime_error("Not implemented");}
  std::string getDimensionId() const {return m_id;}
  coord_t getMaximum() const {return 10;}
  coord_t getMinimum() const {return 0;};
  size_t getNBins() const {return m_nbins;};
  std::string toXMLString() const {throw std::runtime_error("Not implemented");};
  coord_t getX(size_t) const {throw std::runtime_error("Not implemented");};
  virtual void setRange(size_t /*nBins*/, coord_t /*min*/, coord_t /*max*/){ };
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
  MOCK_CONST_METHOD0(getGeometryXML,std::string());
  MOCK_CONST_METHOD0(getNPoints, uint64_t());
  MOCK_CONST_METHOD0(getNEvents, uint64_t());  
  MOCK_CONST_METHOD1(getSignalNormalizedAt, Mantid::signal_t(size_t index1));
  MOCK_CONST_METHOD2(getSignalNormalizedAt, double(size_t index1, size_t index2));
  MOCK_CONST_METHOD3(getSignalNormalizedAt, double(size_t index1, size_t index2, size_t index3));
  MOCK_CONST_METHOD4(getSignalNormalizedAt, double(size_t index1, size_t index2, size_t index3, size_t index4));
  MOCK_CONST_METHOD0(getNonIntegratedDimensions, Mantid::Geometry::VecIMDDimension_const_sptr());
  MOCK_METHOD1(setMDMasking, void(Mantid::Geometry::MDImplicitFunction*));
  MOCK_METHOD0(clearMDMasking,void());
  MOCK_CONST_METHOD0(getSpecialCoordinateSystem, Mantid::Kernel::SpecialCoordinateSystem()); 

  virtual void getLinePlot(const Mantid::Kernel::VMD & , const Mantid::Kernel::VMD & ,
    Mantid::API::MDNormalization , std::vector<Mantid::coord_t> & , std::vector<Mantid::signal_t> & , std::vector<Mantid::signal_t> & ) const
  {}

  virtual std::vector<Mantid::API::IMDIterator*> createIterators(size_t  = 1,
      Mantid::Geometry::MDImplicitFunction *  = NULL) const
  {
    throw std::runtime_error("Not Implemented");
  }

  virtual Mantid::signal_t getSignalAtCoord(const Mantid::coord_t * , const Mantid::API::MDNormalization & ) const
  {
    return 0;
  }

  MockIMDWorkspace()
  : IMDWorkspace()
  {
  }

  virtual ~MockIMDWorkspace() {}
};


//=================================================================================================
/// Mock to allow the behaviour of the chain of responsibility to be tested.
class MockvtkDataSetFactory : public Mantid::VATES::vtkDataSetFactory
{
public:
  MOCK_CONST_METHOD1(create,
    vtkDataSet*(Mantid::VATES::ProgressAction&));
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
  MOCK_METHOD2(updateAlgorithmProgress, void(double, const std::string&));
  ~MockMDLoadingView(){}
};

class MockWorkspaceProvider : public Mantid::VATES::WorkspaceProvider
{
public:
  MOCK_CONST_METHOD1(canProvideWorkspace, bool(std::string));
  MOCK_CONST_METHOD1(fetchWorkspace, Mantid::API::Workspace_sptr(std::string));
  MOCK_CONST_METHOD1(disposeWorkspace, void(std::string));
  ~MockWorkspaceProvider(){}
};

class MockProgressAction : public Mantid::VATES::ProgressAction
{
public:
  MOCK_METHOD1(eventRaised, void(double));
};

class FakeProgressAction : public Mantid::VATES::ProgressAction
{
  virtual void eventRaised(double)
  {
  }
};

/**
Create a field data entry containing (as contents) the argument text.
@param testData : Text to enter
@return new vtkFieldData object containing text.
*/
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

  /**
  Construct an example Geometry section of the XML passed via field-data.

  Note that this function doesn't give complete control over the geometry. For example, the Upper and Lower bounds are hard-coded.

  @param xDimensionIdMapping : Dimension name for dimension to be used as the x-dimension in the view.
  @param yDimensionIdMapping : Dimension name for dimension y-dimension in the view.
  @param zDimensionIdMapping : Dimension name for dimension z-dimension in the view.
  @param tDimensionIdMapping : Dimension name for dimension t-dimension in the view.
  @param xBins : number of bins in the x dimension
  @param yBins : number of bins in the x dimension
  @param zBins : number of bins in the x dimension
  @param tBins : number of bins in the x dimension
  @return xml snippet as string.
  */
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


  /**
  Construct test xml describing the transformations and the inputs.
  @param xDimensionIdMapping : Dimension name for dimension to be used as the x-dimension in the view.
  @param yDimensionIdMapping : Dimension name for dimension y-dimension in the view.
  @param zDimensionIdMapping : Dimension name for dimension z-dimension in the view.
  @param tDimensionIdMapping : Dimension name for dimension t-dimension in the view.
  @return full xml as string.
  */
  std::string constructXML(const std::string& xDimensionIdMapping, const std::string& yDimensionIdMapping, const std::string& zDimensionIdMapping, const std::string& tDimensionIdMapping)
  {
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
      "<MDInstruction>" +
      "<MDWorkspaceName>Input</MDWorkspaceName>" +
      "<MDWorkspaceLocation>test_horace_reader.sqw</MDWorkspaceLocation>" +
      constrctGeometryOnlyXML(xDimensionIdMapping, yDimensionIdMapping, zDimensionIdMapping, tDimensionIdMapping) +
      "</MDInstruction>";
  }

  /**
  Construct an example Geometry section of the XML passed via field-data.

  Note that this function doesn't give complete control over the geometry. For example, the Upper and Lower bounds are hard-coded.

  @param xDimensionIdMapping : Dimension name for dimension to be used as the x-dimension in the view.
  @param yDimensionIdMapping : Dimension name for dimension y-dimension in the view.
  @param zDimensionIdMapping : Dimension name for dimension z-dimension in the view.
  @param tDimensionIdMapping : Dimension name for dimension t-dimension in the view.
  @param xBins : number of bins in the x dimension
  @param yBins : number of bins in the x dimension
  @param zBins : number of bins in the x dimension
  @param tBins : number of bins in the x dimension
  @return xml snippet as string.
  */
  std::string constructGeometryOnlyXMLForMDEvHelperData(\
      const std::string& xDimensionIdMapping,
      const std::string& yDimensionIdMapping,
      const std::string& zDimensionIdMapping,
      const std::string& tDimensionIdMapping,
      std::string xBins = "10",
      std::string yBins = "10",
      std::string zBins = "10",
      std::string tBins = "10"
      )
  {
    std::string cardDirSpec = std::string("<DimensionSet>") +
        "<Dimension ID=\"Axis0\">" +
        "<Name>Axis0</Name>" +
        "<Units>m</Units>" +
        "<UpperBounds>10.0000</UpperBounds>" +
        "<LowerBounds>0.0000</LowerBounds>" +
        "<NumberOfBins>" + xBins + "</NumberOfBins>" +
        "</Dimension>" +
        "<Dimension ID=\"Axis1\">" +
        "<Name>Axis1</Name>" +
        "<Units>m</Units>" +
        "<UpperBounds>10.0000</UpperBounds>" +
        "<LowerBounds>0.0000</LowerBounds>" +
        "<NumberOfBins>" + yBins + "</NumberOfBins>" +
        "</Dimension>" +
        "<Dimension ID=\"Axis2\">" +
        "<Name>Axis2</Name>" +
        "<Units>m</Units>" +
        "<UpperBounds>10.0000</UpperBounds>" +
        "<LowerBounds>0.0000</LowerBounds>" +
        "<NumberOfBins>" + zBins  + "</NumberOfBins>" +
        "</Dimension>";
    std::string timeSpec;
    if (!tDimensionIdMapping.empty())
    {
      timeSpec = std::string("<Dimension ID=\"Axis3\">") +
          "<Name>Axis3</Name>" +
          "<Units>s</Units>" +
          "<UpperBounds>10.0000</UpperBounds>" +
          "<LowerBounds>0.0000</LowerBounds>" +
          "<NumberOfBins>" + tBins + "</NumberOfBins>" +
          "</Dimension>";
    }
    std::string cardDirRef = std::string("<XDimension>") +
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
        "</ZDimension>";
    std::string timeRef;
    if (!tDimensionIdMapping.empty())
    {
      timeRef = std::string("<TDimension>") +
          "<RefDimensionId>" +
          tDimensionIdMapping +
          "</RefDimensionId>" +
          "</TDimension>";
    }
    else
    {
      timeRef = std::string("<TDimension>") +
          "<RefDimensionId>" +
          "</RefDimensionId>" +
          "</TDimension>";
    }
    std::string endTag = "</DimensionSet>";
    std::string body = cardDirSpec;
    if (!timeSpec.empty())
    {
      body += timeSpec;
    }
    body += cardDirRef;
    body += timeRef;
    body += endTag;
    return body;
  }

  /**
  Construct test xml describing the transformations and the inputs.
  @param xDimensionIdMapping : Dimension name for dimension to be used as the x-dimension in the view.
  @param yDimensionIdMapping : Dimension name for dimension y-dimension in the view.
  @param zDimensionIdMapping : Dimension name for dimension z-dimension in the view.
  @param tDimensionIdMapping : Dimension name for dimension t-dimension in the view.
  @return full xml as string.
  */
  std::string constructXMLForMDEvHelperData(const std::string& xDimensionIdMapping,
                                            const std::string& yDimensionIdMapping,
                                            const std::string& zDimensionIdMapping,
                                            const std::string& tDimensionIdMapping)
  {
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
      "<MDInstruction>" +
      "<MDWorkspaceName>Input</MDWorkspaceName>" +
      "<MDWorkspaceLocation>test_horace_reader.sqw</MDWorkspaceLocation>" +
      constructGeometryOnlyXMLForMDEvHelperData(xDimensionIdMapping,
                                                yDimensionIdMapping,
                                                zDimensionIdMapping,
                                                tDimensionIdMapping) +
      "</MDInstruction>";
  }

  Mantid::API::Workspace_sptr createSimple3DWorkspace()
  {
    using namespace Mantid::API;
    AnalysisDataService::Instance().remove("3D_Workspace");
    IAlgorithm* create = FrameworkManager::Instance().createAlgorithm("CreateMDWorkspace");

    create->initialize();
    create->setProperty("Dimensions", 4);
    create->setPropertyValue("Extents","0,5,0,5,0,5,0,5");
    create->setPropertyValue("Names","A,B,C,D");
    create->setPropertyValue("Units","A,A,A,A");
    create->setPropertyValue("OutputWorkspace", "3D_Workspace");
    create->execute();
    return AnalysisDataService::Instance().retrieve("3D_Workspace");
  }

  Mantid::API::Workspace_sptr get3DWorkspace(bool integratedTDimension, bool sliceMD)
  {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    Mantid::API::Workspace_sptr inputWs = createSimple3DWorkspace();

    AnalysisDataService::Instance().remove("binned");
    std::string binningAlgName;
    if(sliceMD)
    {
      binningAlgName = "SliceMD";
    }
    else
    {
      binningAlgName = "BinMD";
    }
    IAlgorithm_sptr binningAlg = AlgorithmManager::Instance().createUnmanaged(binningAlgName);
    binningAlg->initialize();
    binningAlg->setProperty("InputWorkspace", inputWs);
    binningAlg->setPropertyValue("AlignedDim0","A,0,5,2");
    binningAlg->setPropertyValue("AlignedDim1","B,0,5,2");
    binningAlg->setPropertyValue("AlignedDim2","C,0,5,2");

    if(integratedTDimension)
    {
      binningAlg->setPropertyValue("AlignedDim3","D,0,5,1");
    }
    else
    {
      binningAlg->setPropertyValue("AlignedDim3","D,0,5,2");
    }
    binningAlg->setPropertyValue("OutputWorkspace", "binned");
    binningAlg->execute();

    return AnalysisDataService::Instance().retrieve("binned");
  }

  /**
   * Get a string array from a particular field data entry in a vtkDataSet.
   * @param ds : The dataset to retrieve the field data from
   * @param fieldName : The requested field data entry
   * @return The value of the requested field data entry
   */
  std::string getStringFieldDataValue(vtkDataSet *ds, std::string fieldName)
  {
    vtkAbstractArray *value = ds->GetFieldData()->GetAbstractArray(fieldName.c_str());
    vtkStringArray *array = vtkStringArray::SafeDownCast(value);
    return static_cast<std::string>(array->GetValue(0));
  }

} // namespace

#endif
