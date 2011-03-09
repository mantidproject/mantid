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

vtkDataSet* vtkClipperDataSetFactory::create() const
{
  using namespace Mantid::MDAlgorithms;

  BoxInterpreter interpreter;
  boxVector boxFunctions = interpreter.getAllBoxes(m_implicitFunction.get());
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::New();

  boxVector::const_iterator it = boxFunctions.begin();

  for (it; it != boxFunctions.end(); ++it)
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
}
}
