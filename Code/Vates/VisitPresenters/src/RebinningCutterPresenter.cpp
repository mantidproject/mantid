#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include <sstream>
#include <exception>
#include <boost/shared_ptr.hpp>
#include <vtkImplicitFunction.h>
#include "MantidVisitPresenters/RebinningCutterPresenter.h"
#include "MantidVisitPresenters/RebinningXMLGenerator.h"
#include "MantidVisitPresenters/RebinningCutterXMLDefinitions.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"

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

  OriginParameter originParam = OriginParameter(origin.at(0), origin.at(1), origin.at(2));

  WidthParameter widthParam = WidthParameter(width);
  HeightParameter heightParam = HeightParameter(height);
  DepthParameter depthParam = DepthParameter(depth);

  //create the composite holder.
  Mantid::MDAlgorithms::CompositeImplicitFunction* compFunction = new Mantid::MDAlgorithms::CompositeImplicitFunction;

  //create the box.
  BoxImplicitFunction* boxFunc =
      new BoxImplicitFunction(widthParam, heightParam, depthParam, originParam);

  //Add the new function.
  compFunction->addFunction(boost::shared_ptr<Mantid::API::ImplicitFunction>(boxFunc));

  //Add existing functions.
  Mantid::API::ImplicitFunction* existingFunctions = findExistingRebinningDefinitions(m_inputDataSet, getMetadataID());
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
  m_serializing.setWorkspaceName( findExistingWorkspaceNameFromXML(m_inputDataSet, getMetadataID()));
  //Apply the workspace location after extraction from the input xml.
  m_serializing.setWorkspaceLocation( findExistingWorkspaceLocationFromXML(m_inputDataSet, getMetadataID()));

  this->m_initalized = true;
}

vtkUnstructuredGrid* RebinningCutterPresenter::applyReductionKnowledge(Clipper* clipper)
{

  if(true == m_initalized)
  {
  vtkUnstructuredGrid *ug = vtkUnstructuredGrid::New();
  applyReductionKnowledgeToComposite(clipper, m_inputDataSet, ug, this->m_function.get());
  persistReductionKnowledge(ug, this->m_serializing, getMetadataID());
  return ug;
  }
  else
  {
    //To ensure that constructReductionKnowledge is always called first.
    throw std::runtime_error("This instance has not been properly initalized via the construct method.");
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


void persistReductionKnowledge(vtkUnstructuredGrid * out_ds, const
    RebinningXMLGenerator& xmlGenerator, const char* id)
{
  vtkFieldData* fd = vtkFieldData::New();

  metaDataToFieldData(fd, xmlGenerator.createXMLString().c_str(), id);
  out_ds->SetFieldData(fd);
}

const char* getMetadataID()
{
  return "1"; //value unimportant. Identifier to recognise a particular vktArray in the vtkFieldData.
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



void applyReductionKnowledgeToComposite(Clipper* clipper, vtkDataSet* in_ds,
    vtkUnstructuredGrid * out_ds, Mantid::API::ImplicitFunction* function)
{
  using namespace Mantid::MDAlgorithms;
  using namespace Mantid::API;

  CompositeImplicitFunction* compFunction = dynamic_cast<CompositeImplicitFunction*> (function);
  if (NULL != compFunction)
  {
    std::vector<boost::shared_ptr<Mantid::API::ImplicitFunction> > returnedFuncs =
        compFunction->getFunctions();
    std::vector<boost::shared_ptr<Mantid::API::ImplicitFunction> >::const_iterator it =
        returnedFuncs.begin();
    for (it; it != returnedFuncs.end(); ++it)
    {
      BoxImplicitFunction* boxFunction = dynamic_cast<BoxImplicitFunction*> ((*it).get());
      if (NULL != boxFunction)
      {
        vtkBox* box = vtkBox::New();

        //Map implicit function to box function.
        box->SetBounds(boxFunction->getLowerX(), boxFunction->getUpperX(), boxFunction->getLowerY(), boxFunction->getUpperY(), boxFunction->getLowerZ(), boxFunction->getUpperZ());

        clipper->SetInput(in_ds);
        clipper->SetClipFunction(box);
        clipper->SetInsideOut(true);
        clipper->SetRemoveWholeCells(true);
        clipper->SetOutput(out_ds);
        clipper->Update();
        box->Delete();
      }
      else
      {
        applyReductionKnowledgeToComposite(clipper, in_ds, out_ds, (*it).get());
      }
    }
  }
}

}

}






