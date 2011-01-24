#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include <vtkHexahedron.h>
#include <sstream>
#include <exception>
#include <boost/shared_ptr.hpp>
#include <vtkImplicitFunction.h>
#include <vtkStructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkCellData.h>

#include "MantidMDAlgorithms/Load_MDWorkspace.h"
#include "MantidMDAlgorithms/BoxInterpreter.h"
#include "MantidVisitPresenters/RebinningCutterPresenter.h"
#include "MantidVisitPresenters/RebinningXMLGenerator.h"
#include "MantidVisitPresenters/RebinningCutterXMLDefinitions.h"
#include "MantidVisitPresenters/GenerateStructuredGrid.h"
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

struct findID : public std::unary_function <Mantid::Geometry::IMDDimension, bool>
{
  const std::string m_id;
  findID(const std::string id) :
    m_id(id)
  {
  }
  bool operator ()(const boost::shared_ptr<Mantid::Geometry::IMDDimension> obj) const
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
    const double width,
    const double height,
    const double depth,
    std::vector<double>& origin, vtkDataSet* inputDataSet)
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
  Mantid::API::ImplicitFunction* existingFunctions = findExistingRebinningDefinitions(inputDataSet, XMLDefinitions::metaDataId.c_str());
  if (existingFunctions != NULL)
  {
    compFunction->addFunction(boost::shared_ptr<Mantid::API::ImplicitFunction>(existingFunctions));
  }

  m_function = boost::shared_ptr<Mantid::API::ImplicitFunction>(compFunction);
  //Apply the implicit function.
  m_serializer.setImplicitFunction(m_function);
  //Apply the geometry.
  m_serializer.setGeometryXML( constructGeometryXML(dimensions, dimensionX, dimensionY, dimensionZ, dimensiont, height, width, depth, origin) );
  //Apply the workspace name after extraction from the input xml.
  m_serializer.setWorkspaceName( findExistingWorkspaceName(inputDataSet, XMLDefinitions::metaDataId.c_str()));
  //Apply the workspace location after extraction from the input xml.
  m_serializer.setWorkspaceLocation( findExistingWorkspaceLocation(inputDataSet, XMLDefinitions::metaDataId.c_str()));

  //Now actually perform rebinning operation and cache the rebinnned workspace.
  this->m_rebinnedWs = rebin(m_serializer);

  this->m_initalized = true;
}

vtkDataSet* RebinningCutterPresenter::createVisualDataSet(const std::string& scalarName,
    bool isUnstructured, int timestep)
{

  VerifyInitalization();
  //call the rebinning routines and generate a resulting image for visualisation.
  vtkDataSet* visualImageData = generateVisualImage(m_rebinnedWs, scalarName, isUnstructured, timestep);

  //save the work performed as part of this filter instance into the pipeline.
  persistReductionKnowledge(visualImageData, this->m_serializer, XMLDefinitions::metaDataId.c_str());
  return visualImageData;
}

boost::shared_ptr<const Mantid::API::ImplicitFunction> RebinningCutterPresenter::getFunction() const
{
  VerifyInitalization();
  return m_function;
}

Dimension_sptr RebinningCutterPresenter::getXDimensionFromDS(vtkDataSet* vtkDataSetInput) const
{
  using namespace Mantid::Geometry;

  Poco::XML::Element* geometryXMLElement = findExistingGeometryInformation(vtkDataSetInput, XMLDefinitions::metaDataId.c_str() );
  Poco::XML::NodeList* dimensionsXML = geometryXMLElement->getElementsByTagName("Dimension");

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
  using namespace Mantid::Geometry;

  Poco::XML::Element* geometryXMLElement = findExistingGeometryInformation(vtkDataSetInput, XMLDefinitions::metaDataId.c_str() );

  Poco::XML::NodeList* dimensionsXML = geometryXMLElement->getElementsByTagName("Dimension");
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
  using namespace Mantid::Geometry;

  Poco::XML::Element* geometryXMLElement = findExistingGeometryInformation(vtkDataSetInput, XMLDefinitions::metaDataId.c_str() );

  Poco::XML::NodeList* dimensionsXML = geometryXMLElement->getElementsByTagName("Dimension");
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
  using namespace Mantid::Geometry;

  Poco::XML::Element* geometryXMLElement = findExistingGeometryInformation(vtkDataSetInput, XMLDefinitions::metaDataId.c_str() );

  Poco::XML::NodeList* dimensionsXML = geometryXMLElement->getElementsByTagName("Dimension");
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
  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc = pParser.parseString(dimensionXMLString);
  Poco::XML::Element* pDimensionElement = pDoc->documentElement();
  return Mantid::VATES::Dimension_sptr(createDimension(pDimensionElement));
}

Mantid::VATES::Dimension_sptr createDimension(const std::string& dimensionXMLString, int nBins)
{
  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc = pParser.parseString(dimensionXMLString);
  Poco::XML::Element* pDimensionElement = pDoc->documentElement();
  Mantid::Geometry::MDDimension* dimension = createDimension(pDimensionElement);
  double currentMin = dimension->getMinimum();
  double currentMax = dimension->getMaximum();
  //Set the number of bins to use for a given dimension.
  dimension->setRange(currentMin, currentMax, nBins);
  return Mantid::VATES::Dimension_sptr(dimension);
}

Mantid::Geometry::MDDimension* createDimension(Poco::XML::Element* dimensionXML)
{
  using namespace Mantid::Geometry;
  Poco::XML::NamedNodeMap* attributes = dimensionXML->attributes();

  //First and only attribute is the dimension id.
  Poco::XML::Node* dimensionId = attributes->item(0);
  std::string id = dimensionId->innerText();

  Poco::XML::Element* reciprocalMapping = dimensionXML->getChildElement("ReciprocalDimensionMapping");
  MDDimension* mdDimension;

  if (NULL != reciprocalMapping)
  {
    rec_dim recipPrimitiveDirection;

    //Reciprocal dimensions are either q1, q2, or  q3.
    static const boost::regex q1Match("(q1)|(qx)");
    static const boost::regex q2Match("(q2)|(qy)");

    if (regex_match(reciprocalMapping->innerText(), q1Match))
    {
      recipPrimitiveDirection = q1; //rec_dim::q1
    }
    else if (regex_match(reciprocalMapping->innerText(), q2Match))
    {
      recipPrimitiveDirection = q2; //rec_dim::q2
    }
    else
    {
      recipPrimitiveDirection = q3; //rec_dim::q3
    }
    //Create the dimension as a reciprocal dimension
    mdDimension = new MDDimensionRes(id, recipPrimitiveDirection);
  }
  else
  {
    //Create the dimension as an orthogonal dimension.
    mdDimension = new MDDimension(id);
  }

  std::string name = dimensionXML->getChildElement("Name")->innerText();
  mdDimension->setName(name);

  double upperBounds = atof(dimensionXML->getChildElement("UpperBounds")->innerText().c_str());
  double lowerBounds = atof(dimensionXML->getChildElement("LowerBounds")->innerText().c_str());
  unsigned int nBins = atoi(dimensionXML->getChildElement("NumberOfBins")->innerText().c_str());
  Poco::XML::Element* integrationXML = dimensionXML->getChildElement("Integrated");

  if (NULL != integrationXML)
  {
    double upperLimit = atof(integrationXML->getChildElement("UpperLimit")->innerText().c_str());
    double lowerLimit = atof(integrationXML->getChildElement("LowerLimit")->innerText().c_str());

    //As it is not currently possible to set integration ranges on a MDDimension or MDGeometryDescription, boundaries become integration ranges.
    upperBounds = upperLimit;
    lowerBounds = lowerLimit;
  }

  mdDimension->setRange(lowerBounds, upperBounds, nBins);
  return mdDimension;
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
    IMDDimension* dimension = createDimension(dimensionXML);
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
  vtkDataArray* arry =  fieldData->GetArray(id);
  if(arry == NULL)
  {
    throw std::runtime_error("The specified vtk array does not exist");
  }
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
 std::string findExistingWorkspaceLocation(vtkDataSet *inputDataSet, const char* id)
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

Poco::XML::Element* findExistingGeometryInformation(vtkDataSet* inputDataSet, const char* id)
{
  std::string xmlString = fieldDataToMetaData(inputDataSet->GetFieldData(), id);

  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc = pParser.parseString(xmlString);
  Poco::XML::Element* pRootElem = pDoc->documentElement();
  Poco::XML::Element* geometryElem = pRootElem->getChildElement(XMLDefinitions::workspaceGeometryElementName);
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


 Mantid::MDDataObjects::MDWorkspace_sptr rebin(const RebinningXMLGenerator& serializingUtility)
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
     const std::string outputWorkspace = "RebinnedWS";
     xmlRebinAlg.setPropertyValue("OutputWorkspace", outputWorkspace);

     //Use the serialisation utility to generate well-formed xml expressing the rebinning operation.
     std::string xmlString = serializingUtility.createXMLString();
     xmlRebinAlg.setPropertyValue("XMLInputString", xmlString);

     //Run the rebinning algorithm.
     xmlRebinAlg.execute();

     //Use the generated workspace to access the underlying image, which may be rendered.
     MDWorkspace_sptr outputWs = boost::dynamic_pointer_cast<MDWorkspace>(
         AnalysisDataService::Instance().retrieve(outputWorkspace));

     return outputWs;
 }

vtkDataSet* generateVisualImage(Mantid::MDDataObjects::MDWorkspace_sptr rebinnedWs, const std::string& scalarName, bool isUnstructured, const int timestep)
{
  vtkDataSet* visualDataSet;
  if(isUnstructured)
  {
    visualDataSet = generateVTKUnstructuredImage(rebinnedWs, scalarName, timestep);
  }
  else
  {
    visualDataSet = generateVTKStructuredImage(rebinnedWs, scalarName, timestep);
  }
  return visualDataSet;
}

vtkDataSet* generateVTKUnstructuredImage(Mantid::MDDataObjects::MDWorkspace_sptr spWorkspace, const std::string& scalarName, const int timestep)
{
  using namespace Mantid::MDDataObjects;
  const int numberOfPoints = spWorkspace->get_spMDImage()->getDataSize();

  vtkPoints * newPoints = vtkPoints::New();
  newPoints->Allocate(numberOfPoints);

  std::vector<std::vector<std::vector<vtkIdType> > > pointMap;
  pointMap.reserve(numberOfPoints);

  const int nbinsx = spWorkspace->getGeometry()->getXDimension()->getNBins();
  const int nbinsy = spWorkspace->getGeometry()->getYDimension()->getNBins();
  const int nbinsz = spWorkspace->getGeometry()->getZDimension()->getNBins();

  double x[3];
  vtkPoints * points = vtkPoints::New();
  vtkDoubleArray * signal = vtkDoubleArray::New();
  signal->SetName(scalarName.c_str());
  signal->SetNumberOfComponents(1);
  signal->Allocate(numberOfPoints);

  for (double i=0; i<nbinsx; i++)
  {
    std::vector<std::vector<vtkIdType> > plane;
    for(double j=0; j<nbinsy; j++)
    {
      std::vector<vtkIdType> col;
      for(double k=0; k<nbinsz; k++)
      {
        x[0] = i;
        x[1] = j;
        x[2] = k;

        MD_image_point point = spWorkspace->get_spMDImage()->getPoint(i, j, k, timestep);
        vtkIdType pointId = newPoints->InsertNextPoint(i, j, k);
        signal->InsertNextValue(point.s);

        points->InsertPoint(pointId, x);
        col.push_back(pointId);
      }
      plane.push_back(col);
    }
    pointMap.push_back(plane);
  }

  vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
  visualDataSet->Allocate();
  visualDataSet->SetPoints(points);
  visualDataSet->GetCellData()->SetScalars(signal);

  for(int i = 0; i < nbinsx-1 ; i++)
  {
    for(int j = 0; j < nbinsy-1 ; j++)
    {
      for(int k=0; k < nbinsz-1; k++)
      {
        //Identify points for hexahedron
        vtkIdType id_xyz = pointMap[i][j][k];
        vtkIdType id_dxyz = pointMap[i+1][j][k];
        vtkIdType id_dxdyz = pointMap[i+1][j+1][k];
        vtkIdType id_xdyz = pointMap[i][j+1][k];

        vtkIdType id_xydz = pointMap[i][j][k+1];
        vtkIdType id_dxydz = pointMap[i+1][j][k+1];
        vtkIdType id_dxdydz = pointMap[i+1][j+1][k+1];
        vtkIdType id_xdydz = pointMap[i][j+1][k+1];

        //create the hexahedron
        vtkHexahedron *theHex = vtkHexahedron::New();
        theHex->GetPointIds()->SetId(0, id_xyz);
        theHex->GetPointIds()->SetId(1, id_dxyz);
        theHex->GetPointIds()->SetId(2, id_dxdyz);
        theHex->GetPointIds()->SetId(3, id_xdyz);
        theHex->GetPointIds()->SetId(4, id_xydz);
        theHex->GetPointIds()->SetId(5, id_dxydz);
        theHex->GetPointIds()->SetId(6, id_dxdydz);
        theHex->GetPointIds()->SetId(7, id_xdydz);

        visualDataSet->InsertNextCell(VTK_HEXAHEDRON, theHex->GetPointIds());
      }
    }
  }

  newPoints->Delete();
  points->Delete();
  signal->Delete();
  return visualDataSet;

}

vtkDataSet* generateVTKStructuredImage(Mantid::MDDataObjects::MDWorkspace_sptr spWorkspace,
    const std::string& scalarName, const int timestep)
{
  using namespace MDDataObjects;
  const int imageSize = spWorkspace->get_spMDImage()->getDataSize();

  //Creates the visualisation mesh.
  GenerateStructuredGrid meshGenerator(spWorkspace);
  vtkDataSet* visualDataSet = meshGenerator.execute();

  //Add scalar data to the mesh.
  vtkFloatArray* scalars = vtkFloatArray::New();

  const int sizeX = spWorkspace->getXDimension()->getNBins();
  const int sizeY = spWorkspace->getYDimension()->getNBins();
  const int sizeZ = spWorkspace->getZDimension()->getNBins();
  scalars->Allocate(sizeX * sizeY * sizeZ);
  scalars->SetName(scalarName.c_str());

  for (int i = 0; i < sizeX; i++)
  {
    for (int j = 0; j < sizeY; j++)
    {
      for (int k = 0; k < sizeZ; k++)
      {
        // Create an image from the point data.
        MD_image_point point = spWorkspace->get_spMDImage()->getPoint(i, j, k, timestep);
        // Insert scalar data.
        scalars->InsertNextValue(point.s);
      }
    }
  }
  scalars->Squeeze();
  //Attach points to dataset.
  visualDataSet->GetCellData()->AddArray(scalars);

  scalars->Delete();
  return visualDataSet;
}

}

}






