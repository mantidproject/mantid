#include "MantidVatesAPI/vtkClipperDataSetFactory.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidMDAlgorithms/BoxInterpreter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkBox.h"

namespace Mantid
{
namespace VATES
{
/// Constructor
vtkClipperDataSetFactory::vtkClipperDataSetFactory(
    boost::shared_ptr<Mantid::API::ImplicitFunction> implicitFunction,
    vtkDataSet* dataset, Clipper* clipper) :
  m_implicitFunction(implicitFunction), m_dataset(dataset), m_clipper(clipper)
{
}

vtkClipperDataSetFactory::~vtkClipperDataSetFactory()
{
}

void vtkClipperDataSetFactory::initialize(Mantid::API::IMDWorkspace_sptr workspace)
{
  throw std::runtime_error("initialize with a workspace does not apply for this type of factory.");
}

vtkDataSet* vtkClipperDataSetFactory::create() const
{
  using namespace Mantid::MDAlgorithms;

  BoxInterpreter interpreter;
  boxVector boxFunctions = interpreter.getAllBoxes(m_implicitFunction.get());
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::New();

  boxVector::const_iterator it = boxFunctions.begin();

  for (; it != boxFunctions.end(); ++it)
  {
    boost::shared_ptr<BoxImplicitFunction> boxFunction = *it;
    vtkBox* box = vtkBox::New();
    box->SetBounds(
        boxFunction->getLowerX(),
        boxFunction->getUpperX(),
        boxFunction->getLowerY(),
        boxFunction->getUpperY(),
        boxFunction->getLowerZ(),
        boxFunction->getUpperZ());

    m_clipper->SetInput(m_dataset);
    m_clipper->SetClipFunction(box);
    m_clipper->SetInsideOut(true);
    m_clipper->SetRemoveWholeCells(true);
    m_clipper->SetOutput(output);
    m_clipper->Update();
    box->Delete();
  }
  return output;
}

vtkDataSet* vtkClipperDataSetFactory::createMeshOnly() const
{
  throw std::runtime_error("::createMeshOnly() does not apply for this type of factory.");
}

vtkFloatArray* vtkClipperDataSetFactory::createScalarArray() const
{
  throw std::runtime_error("::createScalarArray() does not apply for this type of factory.");
}

}
}
