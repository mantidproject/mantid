#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"
#include "MantidVatesAPI/TimeToTimeStep.h"

using namespace Mantid::DataObjects;

namespace Mantid {
namespace VATES {

template <typename TimeMapper>
vtkMDHistoHex4DFactory<TimeMapper>::vtkMDHistoHex4DFactory(
    const VisualNormalization normalization, const double timestep)
    : vtkMDHistoHexFactory(normalization), m_timestep(timestep) {}

/**
Assigment operator
@param other : vtkMDHistoHex4DFactory to assign to this instance from.
@return ref to assigned current instance.
*/
template <typename TimeMapper>
vtkMDHistoHex4DFactory<TimeMapper> &vtkMDHistoHex4DFactory<TimeMapper>::
operator=(const vtkMDHistoHex4DFactory<TimeMapper> &other) {
  if (this != &other) {
    this->m_normalizationOption = other.m_normalizationOption;
    this->m_workspace = other.m_workspace;
    this->m_timestep = other.m_timestep;
    this->m_timeMapper = other.m_timeMapper;
  }
  return *this;
}

/**
Copy Constructor
@param other : instance to copy from.
*/
template <typename TimeMapper>
vtkMDHistoHex4DFactory<TimeMapper>::vtkMDHistoHex4DFactory(
    const vtkMDHistoHex4DFactory<TimeMapper> &other)
    : vtkMDHistoHexFactory(other), m_timestep(other.m_timestep),
      m_timeMapper(other.m_timeMapper) {}

template <typename TimeMapper>
void vtkMDHistoHex4DFactory<TimeMapper>::initialize(
    const Mantid::API::Workspace_sptr &workspace) {
  m_workspace = doInitialize<MDHistoWorkspace, 4>(workspace);
  if (m_workspace) {
    double tMax = m_workspace->getTDimension()->getMaximum();
    double tMin = m_workspace->getTDimension()->getMinimum();
    size_t nbins = m_workspace->getTDimension()->getNBins();

    m_timeMapper = TimeMapper::construct(tMin, tMax, nbins);
  }
}

template <typename TimeMapper>
void vtkMDHistoHex4DFactory<TimeMapper>::validate() const {
  validateWsNotNull();
}

/**
Create the vtkStructuredGrid from the provided workspace
@param progressUpdating: Reporting object to pass progress information up the
stack.
@return fully constructed vtkDataSet.
*/
template <typename TimeMapper>
vtkSmartPointer<vtkDataSet> vtkMDHistoHex4DFactory<TimeMapper>::create(
    ProgressAction &progressUpdating) const {
  auto product =
      tryDelegatingCreation<MDHistoWorkspace, 4>(m_workspace, progressUpdating);
  if (product != nullptr) {
    return product;
  } else {
    // Create the mesh in a 4D mode
    return this->create3Dor4D(m_timeMapper(m_timestep), progressUpdating);
  }
}

template <typename TimeMapper>
vtkMDHistoHex4DFactory<TimeMapper>::~vtkMDHistoHex4DFactory() {}

template class vtkMDHistoHex4DFactory<TimeToTimeStep>;
template class vtkMDHistoHex4DFactory<TimeStepToTimeStep>;
} // namespace VATES
} // namespace Mantid
