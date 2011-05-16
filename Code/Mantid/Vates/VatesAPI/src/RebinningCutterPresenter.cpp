#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include <vtkHexahedron.h>
#include <sstream>
#include <exception>
#include <boost/shared_ptr.hpp>
#include <vtkImplicitFunction.h>
#include <vtkStructuredGrid.h>
#include <vtkRectilinearGrid.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkMath.h>

#include "MantidMDAlgorithms/Load_MDWorkspace.h"
#include "MantidMDAlgorithms/BoxInterpreter.h"
#include "MantidVatesAPI/RebinningCutterPresenter.h"
#include "MantidVatesAPI/RebinningXMLGenerator.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/vtkStructuredGridFactory.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"

#include "MantidMDAlgorithms/DimensionFactory.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidMDAlgorithms/DynamicRebinFromXML.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NamedNodeMap.h>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidMDAlgorithms/CenterpieceRebinning.h"
#include "MantidMDAlgorithms/Load_MDWorkspace.h"

namespace Mantid
{
namespace VATES
{

using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDDataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::API;

struct findID : public std::unary_function <IMDDimension, bool>
{
  const std::string m_id;
  findID(const std::string id) :
    m_id(id)
  {
  }
  bool operator ()(const boost::shared_ptr<IMDDimension> obj) const
  {
    return m_id == obj->getDimensionId();
  }
};

RebinningCutterPresenter::RebinningCutterPresenter() : m_initalized(false), m_serializer()
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
    vtkDataSet* inputDataSet)
{
  using namespace Mantid::MDAlgorithms;

  //Apply the geometry.
  m_serializer.setGeometryXML(constructGeometryXML(dimensions, dimensionX, dimensionY, dimensionZ, dimensiont));
  //Apply the workspace name after extraction from the input xml.
  m_serializer.setWorkspaceName( findExistingWorkspaceName(inputDataSet, XMLDefinitions::metaDataId().c_str()));
  //Apply the workspace location after extraction from the input xml.
  m_serializer.setWorkspaceLocation( findExistingWorkspaceLocation(inputDataSet, XMLDefinitions::metaDataId().c_str()));

  if(m_serializer.hasGeometryInfo() )
  {
    this->m_initalized = true;
  }
}

void RebinningCutterPresenter::addFunctionKnowledge(CompositeImplicitFunction* compFunction, vtkDataSet* inputDataSet)
{
  //Add existing functions.
    Mantid::API::ImplicitFunction* existingFunctions = findExistingRebinningDefinitions(inputDataSet, XMLDefinitions::metaDataId().c_str());
    if (existingFunctions != NULL)
    {
      compFunction->addFunction(boost::shared_ptr<ImplicitFunction>(existingFunctions));
    }

    m_function = boost::shared_ptr<ImplicitFunction>(compFunction);
    //Apply the implicit function.
    m_serializer.setImplicitFunction(m_function);

}

void RebinningCutterPresenter::constructReductionKnowledge(
    DimensionVec dimensions,
    Dimension_sptr dimensionX,
    Dimension_sptr dimensionY,
    Dimension_sptr dimensionZ,
    Dimension_sptr dimensiont,
    CompositeImplicitFunction* compFunction,
    vtkDataSet* inputDataSet)
{
  using namespace Mantid::MDAlgorithms;
  addFunctionKnowledge(compFunction, inputDataSet);
  constructReductionKnowledge(dimensions, dimensionX, dimensionY, dimensionZ, dimensiont, inputDataSet);
}

/** Apply the rebinning action by calling the necessary Mantid algorithm.
 *
 * @param action :: TODO
 * @param eventHandler :: handle progress notifications
 * @return the MDWorkspace containing the generated image.
 */
Mantid::API::IMDWorkspace_sptr RebinningCutterPresenter::applyRebinningAction(
    RebinningIterationAction action,
    ProgressAction& eventHandler) const
{
     //Verify that constuction has occured properly first/
     VerifyInitalization();

     const std::string outputWorkspace = XMLDefinitions::RebinnedWSName();
     if(RecalculateAll == action)
     {
       //Get the input workspace location and name.
       std::string wsLocation = m_serializer.getWorkspaceLocation();
       std::string wsName = m_serializer.getWorkspaceName();

       Mantid::API::IMDWorkspace_sptr baseWs = constructMDWorkspace(wsLocation);
       AnalysisDataService::Instance().addOrReplace(wsName, baseWs);

       Mantid::MDAlgorithms::DynamicRebinFromXML xmlRebinAlg;
       xmlRebinAlg.setRethrows(true);
       xmlRebinAlg.initialize();

       xmlRebinAlg.setPropertyValue("OutputWorkspace", outputWorkspace);

       //Use the serialisation utility to generate well-formed xml expressing the rebinning operation.
       std::string xmlString = m_serializer.createXMLString();
       xmlRebinAlg.setPropertyValue("XMLInputString", xmlString);

       Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(eventHandler, &ProgressAction::handler);
       //Add observer.
       xmlRebinAlg.addObserver(observer);
       //Run the rebinning algorithm.
       xmlRebinAlg.execute();
       //Remove observer
       xmlRebinAlg.removeObserver(observer);

      }

       //Use the generated workspace to access the underlying image, which may be rendered.
       IMDWorkspace_sptr outputWs = boost::dynamic_pointer_cast<IMDWorkspace>(
           AnalysisDataService::Instance().retrieve(outputWorkspace));

       
       return outputWs;
}


vtkDataSet* RebinningCutterPresenter::createVisualDataSet(vtkDataSetFactory_sptr spvtkDataSetFactory)
{

  VerifyInitalization();

  //Generate the visualisation dataset.
  vtkDataSet* visualImageData = spvtkDataSetFactory->create();

  //save the work performed as part of this filter instance into the pipeline.
  persistReductionKnowledge(visualImageData, this->m_serializer, XMLDefinitions::metaDataId().c_str());
  return visualImageData;
}

boost::shared_ptr<Mantid::API::ImplicitFunction> RebinningCutterPresenter::getFunction() const
{
  VerifyInitalization();
  return m_function;
}

Dimension_const_sptr RebinningCutterPresenter::getDimensionFromWorkspace(const std::string& id)
{
  //Simply pass through and let workspace handle request. 
  const std::string outputWorkspace = XMLDefinitions::RebinnedWSName();
  MDWorkspace_sptr outputWs = boost::dynamic_pointer_cast<MDWorkspace>(
             AnalysisDataService::Instance().retrieve(outputWorkspace));
  //Get the dimension from the workspace.
  return outputWs->getDimension(id);
}

const std::string& RebinningCutterPresenter::getWorkspaceGeometry() const
{
  VerifyInitalization();
  return m_serializer.getWorkspaceGeometry();
}

void RebinningCutterPresenter::VerifyInitalization() const
{
   if(!m_initalized)
   {
     //To ensure that constructReductionKnowledge is always called first.
     throw std::runtime_error("This instance has not been properly initialised via the construct method.");
   }
}

// helper method to construct a near-complete geometry.
std::string RebinningCutterPresenter::constructGeometryXML(
  DimensionVec dimensions,
  Dimension_sptr dimensionX,
  Dimension_sptr dimensionY,
  Dimension_sptr dimensionZ,
  Dimension_sptr dimensiont)
{
  Mantid::Geometry::MDGeometryBuilderXML xmlBuilder;
  DimensionVec::iterator it = dimensions.begin();
  for(;it != dimensions.end(); ++it)
  {
    xmlBuilder.addOrdinaryDimension(*it);
  }
  xmlBuilder.addXDimension(dimensionX);
  xmlBuilder.addYDimension(dimensionY);
  xmlBuilder.addZDimension(dimensionZ);
  xmlBuilder.addTDimension(dimensiont);
  return xmlBuilder.create();
}


std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > getDimensions(
    Poco::XML::Element* geometryXMLElement, bool nonIntegratedOnly)
{
  using namespace Mantid::Geometry;
  Poco::XML::NodeList* dimensionsXML = geometryXMLElement->getElementsByTagName("Dimension");
  std::vector<boost::shared_ptr<IMDDimension> > dimensionVec;

  //Extract dimensions
  int nDimensions = dimensionsXML->length();
  for (int i = 0; i < nDimensions; i++)
  {
    Poco::XML::Element* dimensionXML = static_cast<Poco::XML::Element*> (dimensionsXML->item(i));
    DimensionFactory factory(dimensionXML);
    IMDDimension* dimension = factory.create();
    if (!nonIntegratedOnly || (dimension->getNBins() > 1))
    {
      dimensionVec.push_back(boost::shared_ptr<IMDDimension>(dimension));
    }
  }
  return dimensionVec;
}

std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > getDimensions(
    const std::string& geometryXMLString, bool nonIntegratedOnly)
{
  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc = pParser.parseString(geometryXMLString);
  Poco::XML::Element* pGeometryElem = pDoc->documentElement();
  return getDimensions(pGeometryElem, nonIntegratedOnly);
}

std::vector<double> getBoundingBox(const std::string& functionXMLString)
{
  using namespace Mantid::API;
  ImplicitFunction*  function =ImplicitFunctionFactory::Instance().createUnwrapped(functionXMLString);
  Mantid::MDAlgorithms::BoxInterpreter box;
  return box(function);
}

void persistReductionKnowledge(vtkDataSet* out_ds, const
    RebinningXMLGenerator& xmlGenerator, const char* id)
{
  vtkFieldData* fd = vtkFieldData::New();

  MetadataToFieldData convert;
  convert(fd, xmlGenerator.createXMLString().c_str(), id);

  out_ds->SetFieldData(fd);
  fd->Delete();
}


Mantid::API::ImplicitFunction* findExistingRebinningDefinitions(
    vtkDataSet* inputDataSet, const char* id)
{
  Mantid::API::ImplicitFunction* function = NULL;

  FieldDataToMetadata convert;
  std::string xmlString = convert(inputDataSet->GetFieldData(), id);
  if (false == xmlString.empty())
  {
    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc = pParser.parseString(xmlString);
    Poco::XML::Element* pRootElem = pDoc->documentElement();
    Poco::XML::Element* functionElem = pRootElem->getChildElement(XMLDefinitions::functionElementName());
    if(NULL != functionElem)
    {
      function = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(functionElem);
    }
  }
  return function;
}

//Get the workspace location from the xmlstring.
 std::string findExistingWorkspaceName(vtkDataSet *inputDataSet, const char* id)
{
  FieldDataToMetadata convert;
  std::string xmlString = convert(inputDataSet->GetFieldData(), id);

  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc = pParser.parseString(xmlString);
  Poco::XML::Element* pRootElem = pDoc->documentElement();
  Poco::XML::Element* wsNameElem = pRootElem->getChildElement(XMLDefinitions::workspaceNameElementName());
  if(wsNameElem == NULL)
  {
    throw std::runtime_error("The element containing the workspace name must be present.");
  }
  return wsNameElem->innerText();

}

 //Get the workspace location from the xmlstring.
 std::string findExistingWorkspaceLocation(vtkDataSet *inputDataSet, const char* id)
 {
   FieldDataToMetadata convert;
   std::string xmlString = convert(inputDataSet->GetFieldData(), id);

   Poco::XML::DOMParser pParser;
   Poco::XML::Document* pDoc = pParser.parseString(xmlString);
   Poco::XML::Element* pRootElem = pDoc->documentElement();
   Poco::XML::Element* wsLocationElem = pRootElem->getChildElement(XMLDefinitions::workspaceLocationElementName());
   if(wsLocationElem == NULL)
   {
     throw std::runtime_error("The element containing the workspace location must be present.");
   }
   return wsLocationElem->innerText();
 }

Poco::XML::Element* findExistingGeometryInformation(vtkDataSet* inputDataSet, const char* id)
{
  FieldDataToMetadata convert;
  std::string xmlString = convert(inputDataSet->GetFieldData(), id);

  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc = pParser.parseString(xmlString);
  Poco::XML::Element* pRootElem = pDoc->documentElement();
  Poco::XML::Element* geometryElem = pRootElem->getChildElement(XMLDefinitions::workspaceGeometryElementName());
  if (geometryElem == NULL)
  {
    throw std::runtime_error("The element containing the workspace geometry must be present.");
  }
  return geometryElem;
}


 //NB: At present, the input workspace is required by the dynamicrebinningfromxml algorithm, but not by the
 //sub-algorithm running centerpiece rebinning.
 Mantid::API::IMDWorkspace_sptr constructMDWorkspace(const std::string& wsLocation)
 {
   using namespace Mantid::MDDataObjects;
   using namespace Mantid::Geometry;
   using namespace Mantid::API;

   Mantid::MDAlgorithms::Load_MDWorkspace wsLoaderAlg;
   wsLoaderAlg.initialize();
   std::string wsId = "InputMDWs";
   wsLoaderAlg.setPropertyValue("inFilename", wsLocation);
   wsLoaderAlg.setPropertyValue("MDWorkspace", wsId);
   wsLoaderAlg.execute();
   Workspace_sptr result=AnalysisDataService::Instance().retrieve(wsId);
   MDWorkspace_sptr workspace = boost::dynamic_pointer_cast<MDWorkspace>(result);

   return workspace;
 }

 bool canProcessInput(vtkDataSet* inputDataSet)
 {
   vtkFieldData* fd = inputDataSet->GetFieldData();
   return NULL != fd->GetArray(XMLDefinitions::metaDataId().c_str());
 }

}
}






