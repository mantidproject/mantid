#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/ProgressAction.h"
#include <stdexcept>
#include <utility>

namespace Mantid {
namespace VATES {

vtkDataSetFactory::vtkDataSetFactory()
    : m_useTransform(false), m_bCheckDimensionality(true) {}

/**
 * Checks the successor and throws if invalid.
 * @throw std::runtime_error if types are the same
 * @throw std::invalid_argument if successor is nullptr
 */
void vtkDataSetFactory::checkSuccessor() const {
  if (m_successor) {
    if (m_successor->getFactoryTypeName() == this->getFactoryTypeName()) {
      throw std::runtime_error("Cannot assign a successor to vtkDataSetFactory "
                               "with the same type as the present "
                               "vtkDataSetFactory type.");
    }
  } else {
    throw std::invalid_argument("Null pointer passed as successor");
  }
}

/*
Set a flag indicating whether dimensionality should be checked
@param flag : TRUE to check dimensionality otherwise FALSE.
*/
void vtkDataSetFactory::setCheckDimensionality(bool flag) {
  m_bCheckDimensionality = flag;
}

/*
Get a flag indicating whether dimensionality should be checked
@return true if dimensionality is checked.
*/
bool vtkDataSetFactory::doesCheckDimensionality() const {
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
  this->initialize(std::move(ws));
  return this->create(progressUpdater);
}

// What we call the scalar array bearing the signal values in the vtk data set.
const std::string vtkDataSetFactory::ScalarName = "signal";
} // namespace VATES
} // namespace Mantid
