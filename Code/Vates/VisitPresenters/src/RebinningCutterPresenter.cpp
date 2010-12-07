#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include <sstream>
#include <exception>
#include <boost/shared_ptr.hpp>
#include <vtkImplicitFunction.h>
#include "MantidVisitPresenters/RebinningCutterPresenter.h"
#include "MantidMDAlgorithms/NormalParameter.h"
#include <boost/algorithm/string.hpp>
namespace Mantid
{
namespace VATES
{


void RebinningCutterPresenter::metaDataToFieldData(vtkFieldData* fieldData, std::string metaData,
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

  for(int i = 0 ; i < metaData.size(); i++)
  {
    newArry->InsertNextValue(metaData.at(i));
  }
}

std::string RebinningCutterPresenter::fieldDataToMetaData(vtkFieldData* fieldData, const char* id)
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

Mantid::API::ImplicitFunction* RebinningCutterPresenter::findExistingRebinningDefinitions(
    vtkDataSet *in_ds, const char* id)
{
  Mantid::API::ImplicitFunction* function = NULL;
  std::string xmlString = fieldDataToMetaData(in_ds->GetFieldData(), id);
  if (false == xmlString.empty())
  {
    function = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(xmlString);
  }
  return function;
}

Mantid::API::ImplicitFunction* RebinningCutterPresenter::constructReductionKnowledge(vtkDataSet *in_ds,
    std::vector<double>& normal, std::vector<double>& origin, const char* id)
{
  if (normal.size() != 3)
  {
    throw std::invalid_argument("Three normal components expected.");
  }
  if (origin.size() != 3)
  {
    throw std::invalid_argument("Three origin components expected.");
  }
  Mantid::MDAlgorithms::NormalParameter normalParam = Mantid::MDAlgorithms::NormalParameter(
      normal.at(0), normal.at(1), normal.at(2));
  Mantid::MDAlgorithms::OriginParameter originParam = Mantid::MDAlgorithms::OriginParameter(
      origin.at(0), origin.at(1), origin.at(2));

  Mantid::MDAlgorithms::CompositeImplicitFunction* compFunc =
      new Mantid::MDAlgorithms::CompositeImplicitFunction;
  Mantid::MDAlgorithms::PlaneImplicitFunction* planeFunc =
      new Mantid::MDAlgorithms::PlaneImplicitFunction(normalParam, originParam);

  //Add the new plane function.
  compFunc->addFunction(boost::shared_ptr<Mantid::API::ImplicitFunction>(planeFunc));

  //Add existing functions.
  Mantid::API::ImplicitFunction* existingFunctions = findExistingRebinningDefinitions(in_ds, id);
  if (existingFunctions != NULL)
  {
    compFunc->addFunction(boost::shared_ptr<Mantid::API::ImplicitFunction>(existingFunctions));
  }

  return compFunc;
}

vtkUnstructuredGrid* RebinningCutterPresenter::applyReductionKnowledge(Clipper* clipper,
    vtkDataSet *in_ds, Mantid::API::ImplicitFunction * const function)
{

  vtkUnstructuredGrid *ug = vtkUnstructuredGrid::New();

  this->applyReductionKnowledgeToComposite(clipper, in_ds, ug, function);
  return ug;
}

void RebinningCutterPresenter::applyReductionKnowledgeToComposite(Clipper* clipper, vtkDataSet* in_ds,
    vtkUnstructuredGrid * out_ds, Mantid::API::ImplicitFunction * const function)
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
      PlaneImplicitFunction* planeFunction = dynamic_cast<PlaneImplicitFunction*> ((*it).get());
      if (NULL != planeFunction)
      {
        vtkPlane* plane = vtkPlane::New();
        plane->SetOrigin(planeFunction->getOriginX(), planeFunction->getOriginY(),
            planeFunction->getOriginZ());
        plane->SetNormal(planeFunction->getNormalX(), planeFunction->getNormalY(),
            planeFunction->getNormalZ());

        clipper->SetInput(in_ds);
        clipper->SetClipFunction(plane);
        clipper->SetInsideOut(true);
        clipper->SetRemoveWholeCells(true);
        clipper->SetOutput(out_ds);
        clipper->Update();
        plane->Delete();
      }
      else
      {
        applyReductionKnowledgeToComposite(clipper, in_ds, out_ds, (*it).get());
      }
    }
  }
}

void RebinningCutterPresenter::persistReductionKnowledge(vtkUnstructuredGrid * out_ds,
    Mantid::API::ImplicitFunction const * const function, const char* id)
{
  vtkFieldData* fd = vtkFieldData::New();

  metaDataToFieldData(fd, function->toXMLString().c_str(), id);
  out_ds->SetFieldData(fd);
}

const char*  RebinningCutterPresenter::getMetadataID()
{
  return "1";
}

}

}






