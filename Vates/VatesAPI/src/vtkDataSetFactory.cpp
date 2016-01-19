#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/ProgressAction.h"
#include <stdexcept>

namespace Mantid
{
namespace VATES
{

vtkDataSetFactory::vtkDataSetFactory() : m_useTransform(false), m_bCheckDimensionality(true)
{
}

/*
Set a flag indicating whether dimensionality should be checked
@param flag : TRUE to check dimensionality otherwise FALSE.
*/
void vtkDataSetFactory::setCheckDimensionality(bool flag)
{
  m_bCheckDimensionality = flag;
}

/*
Get a flag indicating whether dimensionality should be checked
@return true if dimensionality is checked.
*/
bool vtkDataSetFactory::doesCheckDimensionality() const
{
  return m_bCheckDimensionality;
}


/*
Convenience function. Creates an output visualisation data set in one-shot.

@param ws : input workspace to interpret a vtkDataSet from.
@param progressUpdater : object used to update the progress action.
@result vtkDataSet* interpreted from input.
*/
vtkSmartPointer<vtkDataSet>
vtkDataSetFactory::oneStepCreate(Mantid::API::Workspace_sptr ws,
                                 ProgressAction &progressUpdater) {
  this->initialize(ws);
  return this->create(progressUpdater);
}

// What we call the scalar array bearing the signal values in the vtk data set.
const std::string vtkDataSetFactory::ScalarName ="signal";

}

}
