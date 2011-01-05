#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include <sstream>
#include <exception>
#include <boost/shared_ptr.hpp>
#include <vtkImplicitFunction.h>
#include <vtkStructuredGrid.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include "MantidMDAlgorithms/Load_MDWorkspace.h"
#include "MantidVisitPresenters/RebinningCutterPresenter.h"
#include "MantidVisitPresenters/RebinningXMLGenerator.h"
#include "MantidVisitPresenters/RebinningCutterXMLDefinitions.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidMDAlgorithms/DynamicRebinFromXML.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "MantidAPI/AnalysisDataService.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"

#include <boost/algorithm/string.hpp>
namespace Mantid
{
namespace VATES
{

RebinningCutterPresenter::RebinningCutterPresenter(vtkDataSet* inputDataSet) : m_initalized(false), m_inputDataSet(inputDataSet)
{
}

RebinningCutterPresenter::~RebinningCutterPresenter()
{

}

void RebinningCutterPresenter::constructReductionKnowledge(
    DimensionVec dimensions,
    Dimension_sptr dimensionX,
    Dimension_sptr dimensionY,
    Dimension_sptr dimensionZ,
    Dimension_sptr dimensiont,
    double height,
    double width,
    double depth,
    std::vector<double>& origin)
{
  using namespace Mantid::MDAlgorithms;

  if (origin.size() != 3)
  {
    throw std::invalid_argument("Three origin components expected.");
  }

  //Create domain parameters.
  OriginParameter originParam = OriginParameter(origin.at(0), origin.at(1), origin.at(2));
  WidthParameter widthParam = WidthParameter(width);
  HeightParameter heightParam = HeightParameter(height);
  DepthParameter depthParam = DepthParameter(depth);

  //Create the composite holder.
  Mantid::MDAlgorithms::CompositeImplicitFunction* compFunction = new Mantid::MDAlgorithms::CompositeImplicitFunction;

  //Create the box. This is specific to this type of presenter and this type of filter. Other rebinning filters may use planes etc.
  BoxImplicitFunction* boxFunc =
      new BoxImplicitFunction(widthParam, heightParam, depthParam, originParam);

  //Add the new function.
  compFunction->addFunction(boost::shared_ptr<Mantid::API::ImplicitFunction>(boxFunc));

  //Add existing functions.
  Mantid::API::ImplicitFunction* existingFunctions = findExistingRebinningDefinitions(m_inputDataSet, metaDataId.c_str());
  if (existingFunctions != NULL)
  {
    compFunction->addFunction(boost::shared_ptr<Mantid::API::ImplicitFunction>(existingFunctions));
  }

  m_function = boost::shared_ptr<Mantid::API::ImplicitFunction>(compFunction);
  //Apply the implicit function.
  m_serializing.setImplicitFunction(m_function);
  //Apply the geometry.
  m_serializing.setGeometryXML( constructGeometryXML(dimensions, dimensionX, dimensionY, dimensionZ, dimensiont, height, width, depth, origin) );
  //Apply the workspace name after extraction from the input xml.
  m_serializing.setWorkspaceName( findExistingWorkspaceNameFromXML(m_inputDataSet, metaDataId.c_str()));
  //Apply the workspace location after extraction from the input xml.
  m_serializing.setWorkspaceLocation( findExistingWorkspaceLocationFromXML(m_inputDataSet, metaDataId.c_str()));

  this->m_initalized = true;
}

vtkDataSet* RebinningCutterPresenter::applyReductionKnowledge()
{

  if (true == m_initalized)
  {
    //call the rebinning routines and generate a resulting image for visualisation.
    vtkDataSet* visualImageData = generateVisualImage(m_serializing);

    //save the work performed as part of this filter instance into the pipeline.
    persistReductionKnowledge(visualImageData, this->m_serializing, metaDataId.c_str());
    return visualImageData;
  }
  else
  {
    //To ensure that constructReductionKnowledge is always called first.
    throw std::runtime_error("This instance has not been properly initialised via the construct method.");
  }

}

boost::shared_ptr<const Mantid::API::ImplicitFunction> RebinningCutterPresenter::getFunction() const
{
  return m_function;
}

// helper method to construct a near-complete geometry.
std::string constructGeometryXML(
  DimensionVec dimensions,
  Dimension_sptr dimensionX,
  Dimension_sptr dimensionY,
  Dimension_sptr dimensionZ,
  Dimension_sptr dimensiont,
  double height,
  double width,
  double depth,
  std::vector<double>& origin)
{
  using namespace Mantid::Geometry;
  std::set<MDBasisDimension> basisDimensions;
  for(unsigned int i = 0; i < dimensions.size(); i++)
  {
    //read dimension.
    std::string dimensionId = dimensions[i]->getDimensionId();
    bool isReciprocal = dimensions[i]->isReciprocal();
    //basis dimension.
    basisDimensions.insert(MDBasisDimension(dimensionId, isReciprocal, i));

    //NB: Geometry requires both a basis and geometry description to work. Initially all cuts and dimensions treated as orthogonal.
    //So that congruent checks pass on the geometry, the basis is fabricated from the dimensions. This is not an ideal implementation. Other designs will
    //be considered.
  }

  UnitCell cell; // Unit cell currently does nothing.
  MDGeometryBasis basis(basisDimensions, cell);

  MDGeometryDescription description(dimensions, dimensionX, dimensionY, dimensionZ, dimensiont);

  //Create a geometry.
  MDGeometry geometry(basis, description);
  return geometry.toXMLString();

}

void metaDataToFieldData(vtkFieldData* fieldData, std::string metaData,
    const char* id)
{
  //clean out existing.
  vtkDataArray* arry = fieldData->GetArray(id);
  if(NULL != arry)
  {
    fieldData->RemoveArray(id);
  }
  //create new.
  vtkCharArray* newArry = vtkCharArray::New();
  newArry->Allocate(metaData.size());
  newArry->SetName(id);
  fieldData->AddArray(newArry);

  for(unsigned int i = 0 ; i < metaData.size(); i++)
  {
    newArry->InsertNextValue(metaData.at(i));
  }
}

std::string fieldDataToMetaData(vtkFieldData* fieldData, const char* id)
{
  std::string sXml;
  vtkDataArray* arry = fieldData->GetArray(id);

  if (vtkCharArray* carry = dynamic_cast<vtkCharArray*> (arry))
  {
    carry->Squeeze();
    for (int i = 0; i < carry->GetSize(); i++)
    {
      char c = carry->GetValue(i);
      if (int(c) > 1)
      {
        sXml.push_back(c);
      }
    }
    boost::trim(sXml);
  }

  return sXml;
}


void persistReductionKnowledge(vtkDataSet* out_ds, const
    RebinningXMLGenerator& xmlGenerator, const char* id)
{
  vtkFieldData* fd = vtkFieldData::New();

  metaDataToFieldData(fd, xmlGenerator.createXMLString().c_str(), id);
  out_ds->SetFieldData(fd);
}


Mantid::API::ImplicitFunction* findExistingRebinningDefinitions(
    vtkDataSet* inputDataSet, const char* id)
{
  Mantid::API::ImplicitFunction* function = NULL;
  std::string xmlString = fieldDataToMetaData(inputDataSet->GetFieldData(), id);
  if (false == xmlString.empty())
  {
    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc = pParser.parseString(xmlString);
    Poco::XML::Element* pRootElem = pDoc->documentElement();
    Poco::XML::Element* functionElem = pRootElem->getChildElement(XMLDefinitions::functionElementName);

    function = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(functionElem);
  }
  return function;
}

//Get the workspace location from the xmlstring.
 std::string findExistingWorkspaceNameFromXML(vtkDataSet *inputDataSet, const char* id)
{
  std::string xmlString = fieldDataToMetaData(inputDataSet->GetFieldData(), id);

  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc = pParser.parseString(xmlString);
  Poco::XML::Element* pRootElem = pDoc->documentElement();
  Poco::XML::Element* wsNameElem = pRootElem->getChildElement(XMLDefinitions::workspaceNameElementName);
  if(wsNameElem == NULL)
  {
    throw std::runtime_error("The element containing the workspace name must be present.");
  }
  return wsNameElem->innerText();

}

 //Get the workspace location from the xmlstring.
 std::string findExistingWorkspaceLocationFromXML(vtkDataSet *inputDataSet, const char* id)
 {
   std::string xmlString = fieldDataToMetaData(inputDataSet->GetFieldData(), id);

   Poco::XML::DOMParser pParser;
   Poco::XML::Document* pDoc = pParser.parseString(xmlString);
   Poco::XML::Element* pRootElem = pDoc->documentElement();
   Poco::XML::Element* wsLocationElem = pRootElem->getChildElement(XMLDefinitions::workspaceLocationElementName);
   if(wsLocationElem == NULL)
   {
     throw std::runtime_error("The element containing the workspace location must be present.");
   }
   return wsLocationElem->innerText();
 }

 //NB: At present, the input workspace is required by the dynamicrebinningfromxml algorithm, but not by the
 //sub-algorithm running centerpiece rebinning.
 Mantid::MDDataObjects::MDWorkspace_sptr constructMDWorkspace(const std::string& wsLocation)
 {
   using namespace Mantid::MDDataObjects;
   using namespace Mantid::Geometry;
   using namespace Mantid::API;

   Mantid::MDAlgorithms::Load_MDWorkspace wsLoaderAlg;
   wsLoaderAlg.setPropertyValue("inFilename", wsLocation);
   wsLoaderAlg.setPropertyValue("MDWorkspace","InputWs");
   wsLoaderAlg.execute();
   Workspace_sptr result=AnalysisDataService::Instance().retrieve("InputWs");
   MDWorkspace_sptr workspace = boost::dynamic_pointer_cast<MDWorkspace>(result);

   return workspace;
 }


vtkDataSet* generateVisualImage(RebinningXMLGenerator serializingUtility)
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::API;
  //Get the input workspace location and name.
  std::string wsLocation = serializingUtility.getWorkspaceLocation();
  std::string wsName = serializingUtility.getWorkspaceName();
  MDWorkspace_sptr baseWs = constructMDWorkspace(wsLocation);
  AnalysisDataService::Instance().addOrReplace(wsName, baseWs);

  Mantid::MDAlgorithms::DynamicRebinFromXML xmlRebinAlg;
  xmlRebinAlg.setRethrows(true);
  xmlRebinAlg.initialize();
  xmlRebinAlg.setPropertyValue("OutputWorkspace", "RebinnedWS");

  //Use the serialisation utility to generate well-formed xml expressing the rebinning operation.
  std::string xmlString = serializingUtility.createXMLString();
  xmlRebinAlg.setPropertyValue("XMLInputString", xmlString);

  //Run the rebinning algorithm.
  xmlRebinAlg.execute();

  //Use the generated workspace to access the underlying image, which may be rendered.
  MDWorkspace_sptr outputWs = boost::dynamic_pointer_cast<MDWorkspace>(
      AnalysisDataService::Instance().retrieve("RebinnedWS"));

  const int imageSize = outputWs->get_spMDImage()->getDataSize();

  vtkStructuredGrid* visualDataSet = vtkStructuredGrid::New();
  vtkPoints *points = vtkPoints::New();
  points->Allocate(imageSize);
  vtkDoubleArray* scalars = vtkDoubleArray::New();
  scalars->SetName("Signal");
  scalars->Allocate(imageSize);

  const int sizeX = outputWs->getXDimension()->getNBins();
  const int sizeY = outputWs->getYDimension()->getNBins();
  const int sizeZ = outputWs->getZDimension()->getNBins();

  //Loop through dimensions
  for (int i = 0; i < sizeX; i++)
  {
    for (int j = 0; j < sizeY; j++)
    {
      for (int k = 0; k < sizeZ; k++)
      {
        //Create an image from the point data.
        MD_image_point point = outputWs->get_spMDImage()->getPoint(i, j, k);
        scalars->InsertNextValue(point.s);
        points->InsertNextPoint(i, j, k);
      }
    }
  }

  //Attach points to dataset.
  visualDataSet->SetPoints(points);
  visualDataSet->GetPointData()->AddArray(scalars);
  visualDataSet->SetDimensions(sizeX, sizeY, sizeZ);
  points->Delete();
  scalars->Delete();
  return visualDataSet;
}

const std::string RebinningCutterPresenter::metaDataId="1";


}

}






