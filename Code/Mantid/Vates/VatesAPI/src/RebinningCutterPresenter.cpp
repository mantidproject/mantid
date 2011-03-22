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
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
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
#include "MantidAPI/AnalysisDataService.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/DOM/NamedNodeMap.h"

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

Mantid::MDDataObjects::MDWorkspace_sptr RebinningCutterPresenter::applyRebinningAction(
    RebinningIterationAction action,
    ProgressAction& eventHandler) const
{
     //Verify that constuction has occured properly first/
     VerifyInitalization();

     const std::string outputWorkspace = "RebinnedWS";
     if(RecalculateAll == action)
     {
       //Get the input workspace location and name.
       std::string wsLocation = m_serializer.getWorkspaceLocation();
       std::string wsName = m_serializer.getWorkspaceName();

       MDWorkspace_sptr baseWs = constructMDWorkspace(wsLocation);
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
       MDWorkspace_sptr outputWs = boost::dynamic_pointer_cast<MDWorkspace>(
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

Dimension_sptr RebinningCutterPresenter::getXDimensionFromDS(vtkDataSet* vtkDataSetInput) const
{
  Poco::XML::Element* geometryXMLElement = findExistingGeometryInformation(vtkDataSetInput, XMLDefinitions::metaDataId().c_str() );

  std::vector<boost::shared_ptr<IMDDimension> > dimensionVec = getDimensions(geometryXMLElement);

  //Find the requested xDimension alignment from the dimension id provided in the xml.
  Poco::XML::Element* xDimensionElement = geometryXMLElement->getChildElement("XDimension");
  std::string xDimId = xDimensionElement->getChildElement("RefDimensionId")->innerText();
  DimensionVecIterator xDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(xDimId));

  if (xDimensionIt == dimensionVec.end())
  {
    throw std::invalid_argument("Cannot determine x-dimension mapping.");
  }

  return *xDimensionIt;
}

Dimension_sptr RebinningCutterPresenter::getYDimensionFromDS(vtkDataSet* vtkDataSetInput) const
{
  Poco::XML::Element* geometryXMLElement = findExistingGeometryInformation(vtkDataSetInput, XMLDefinitions::metaDataId().c_str() );

  std::vector<boost::shared_ptr<IMDDimension> > dimensionVec = getDimensions(geometryXMLElement);

  //Find the requested xDimension alignment from the dimension id provided in the xml.
  Poco::XML::Element* yDimensionElement = geometryXMLElement->getChildElement("YDimension");
  std::string yDimId = yDimensionElement->getChildElement("RefDimensionId")->innerText();
  DimensionVecIterator yDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(yDimId));

  if (yDimensionIt == dimensionVec.end())
  {
    throw std::invalid_argument("Cannot determine y-dimension mapping.");
  }

  return *yDimensionIt;
}

Dimension_sptr RebinningCutterPresenter::getZDimensionFromDS(vtkDataSet* vtkDataSetInput) const
{

  Poco::XML::Element* geometryXMLElement = findExistingGeometryInformation(vtkDataSetInput, XMLDefinitions::metaDataId().c_str() );

  std::vector<boost::shared_ptr<IMDDimension> > dimensionVec = getDimensions(geometryXMLElement);

  //Find the requested xDimension alignment from the dimension id provided in the xml.
  Poco::XML::Element* zDimensionElement = geometryXMLElement->getChildElement("ZDimension");
  std::string zDimId = zDimensionElement->getChildElement("RefDimensionId")->innerText();
  DimensionVecIterator zDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(zDimId));

  if (zDimensionIt == dimensionVec.end())
  {
    throw std::invalid_argument("Cannot determine z-dimension mapping.");
  }

  return *zDimensionIt;
}

Dimension_sptr RebinningCutterPresenter::getTDimensionFromDS(vtkDataSet* vtkDataSetInput) const
{
  Poco::XML::Element* geometryXMLElement = findExistingGeometryInformation(vtkDataSetInput, XMLDefinitions::metaDataId().c_str() );

  std::vector<boost::shared_ptr<IMDDimension> > dimensionVec = getDimensions(geometryXMLElement);

  //Find the requested xDimension alignment from the dimension id provided in the xml.
  Poco::XML::Element* tDimensionElement = geometryXMLElement->getChildElement("TDimension");
  std::string tDimId = tDimensionElement->getChildElement("RefDimensionId")->innerText();
  DimensionVecIterator tDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(tDimId));

  if (tDimensionIt == dimensionVec.end())
  {
    throw std::invalid_argument("Cannot determine t-dimension mapping.");
  }

  return *tDimensionIt;
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

Mantid::VATES::Dimension_sptr createDimension(const std::string& dimensionXMLString)
{
  DimensionFactory factory = DimensionFactory::createDimensionFactory(dimensionXMLString);
  return Mantid::VATES::Dimension_sptr(factory.create());
}

Mantid::VATES::Dimension_sptr createDimension(const std::string& dimensionXMLString, int nBins)
{
  DimensionFactory factory = DimensionFactory::createDimensionFactory(dimensionXMLString);
  MDDimension* dimension = factory.createAsMDDimension();
  double currentMin = dimension->getMinimum();
  double currentMax = dimension->getMaximum();
  //Set the number of bins to use for a given dimension.
  dimension->setRange(currentMin, currentMax, nBins);
  return Mantid::VATES::Dimension_sptr(dimension);
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

// helper method to construct a near-complete geometry.
std::string constructGeometryXML(
  DimensionVec dimensions,
  Dimension_sptr dimensionX,
  Dimension_sptr dimensionY,
  Dimension_sptr dimensionZ,
  Dimension_sptr dimensiont)
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

  //TODO: Get Rotation matrix from Plane ImplicitFunction
        RotationMatrix identityMatrix(9, 0);
        identityMatrix[0] = 1;
        identityMatrix[4] = 1;
        identityMatrix[8] = 1;
  MDGeometryDescription description(dimensions, dimensionX, dimensionY, dimensionZ, dimensiont, identityMatrix);

  //Create a geometry.
  MDGeometry geometry(basis, description);
  return geometry.toXMLString();

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
 Mantid::MDDataObjects::MDWorkspace_sptr constructMDWorkspace(const std::string& wsLocation)
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






