#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"

#include "MantidVatesAPI/vtkMDHWSignalArray.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkNullStructuredGrid.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidKernel/ReadLock.h"

#include "vtkNew.h"
#include "vtkStructuredGrid.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"

#include <cmath>

using Mantid::API::IMDWorkspace;
using Mantid::API::IMDHistoWorkspace;
using Mantid::Kernel::CPUTimer;
using namespace Mantid::DataObjects;
using Mantid::Kernel::ReadLock;

namespace Mantid {
namespace VATES {

vtkMDHistoHexFactory::vtkMDHistoHexFactory(
    ThresholdRange_scptr thresholdRange,
    const VisualNormalization normalizationOption)
    : m_normalizationOption(normalizationOption),
      m_thresholdRange(thresholdRange) {}

/**
Assigment operator
@param other : vtkMDHistoHexFactory to assign to this instance from.
@return ref to assigned current instance.
*/
vtkMDHistoHexFactory &vtkMDHistoHexFactory::
operator=(const vtkMDHistoHexFactory &other) {
  if (this != &other) {
    this->m_normalizationOption = other.m_normalizationOption;
    this->m_thresholdRange = other.m_thresholdRange;
    this->m_workspace = other.m_workspace;
  }
  return *this;
}

/**
Copy Constructor
@param other : instance to copy from.
*/
vtkMDHistoHexFactory::vtkMDHistoHexFactory(const vtkMDHistoHexFactory &other) {
  this->m_normalizationOption = other.m_normalizationOption;
  this->m_thresholdRange = other.m_thresholdRange;
  this->m_workspace = other.m_workspace;
}

void vtkMDHistoHexFactory::initialize(Mantid::API::Workspace_sptr workspace) {
  m_workspace = doInitialize<MDHistoWorkspace, 3>(workspace);

  // Setup range values according to whatever strategy object has been injected.
  m_thresholdRange->setWorkspace(workspace);
  m_thresholdRange->calculate();
}

void vtkMDHistoHexFactory::validateWsNotNull() const {

  if (NULL == m_workspace.get()) {
    throw std::runtime_error("IMDWorkspace is null");
  }
}

void vtkMDHistoHexFactory::validate() const { validateWsNotNull(); }

/** Method for creating a 3D or 4D data set
 *
 * @param timestep :: index of the time step (4th dimension) in the workspace.
 *        Set to 0 for a 3D workspace.
 * @param progressUpdate: Progress updating. passes progress information up the
 *stack.
 * @return the vtkDataSet created
 */
vtkSmartPointer<vtkDataSet>
vtkMDHistoHexFactory::create3Dor4D(size_t timestep,
                                   ProgressAction &progressUpdate) const {
  // Acquire a scoped read-only lock to the workspace (prevent segfault from
  // algos modifying ws)
  ReadLock lock(*m_workspace);

  const size_t nDims = m_workspace->getNonIntegratedDimensions().size();

  std::vector<size_t> indexMultiplier(nDims, 0);

  // For quick indexing, accumulate these values
  // First multiplier
  indexMultiplier[0] = m_workspace->getDimension(0)->getNBins();
  for (size_t d = 1; d < nDims; d++) {
    indexMultiplier[d] =
        indexMultiplier[d - 1] * m_workspace->getDimension(d)->getNBins();
  }

  const int nBinsX = static_cast<int>(m_workspace->getXDimension()->getNBins());
  const int nBinsY = static_cast<int>(m_workspace->getYDimension()->getNBins());
  const int nBinsZ = static_cast<int>(m_workspace->getZDimension()->getNBins());

  const int imageSize = (nBinsX) * (nBinsY) * (nBinsZ);

  vtkSmartPointer<vtkStructuredGrid> visualDataSet =
      vtkSmartPointer<vtkStructuredGrid>::New();
  visualDataSet->SetDimensions(nBinsX+1,nBinsY+1,nBinsZ+1);

  // Array with true where the voxel should be shown
  double progressFactor = 0.5 / double(imageSize);

  std::size_t offset = 0;
  if (nDims == 4)
  {
    offset = timestep * indexMultiplier[2];
  }

  std::unique_ptr<MDHistoWorkspaceIterator> iterator(
      dynamic_cast<MDHistoWorkspaceIterator *>(createIteratorWithNormalization(
          m_normalizationOption, m_workspace.get())));

  vtkNew<vtkMDHWSignalArray<double>> signal;

  signal->SetName(vtkDataSetFactory::ScalarName.c_str());
  signal->InitializeArray(std::move(iterator), offset, imageSize);
  visualDataSet->GetCellData()->SetScalars(signal.GetPointer());


  for (vtkIdType index = 0; index < imageSize; ++index) {
    progressUpdate.eventRaised(double(index) * progressFactor);
    double signalScalar = signal->GetValue(index);
    bool maskValue =
        (!std::isfinite(signalScalar) || !m_thresholdRange->inRange(signalScalar));
    if (maskValue) {
      visualDataSet->BlankCell(index);
    }
  }

  vtkNew<vtkPoints> points;

  Mantid::coord_t in[2];

  const coord_t maxX = m_workspace->getXDimension()->getMaximum();
  const coord_t minX = m_workspace->getXDimension()->getMinimum();
  const coord_t maxY = m_workspace->getYDimension()->getMaximum();
  const coord_t minY = m_workspace->getYDimension()->getMinimum();
  const coord_t maxZ = m_workspace->getZDimension()->getMaximum();
  const coord_t minZ = m_workspace->getZDimension()->getMinimum();

  const coord_t incrementX = (maxX - minX) / static_cast<coord_t>(nBinsX);
  const coord_t incrementY = (maxY - minY) / static_cast<coord_t>(nBinsY);
  const coord_t incrementZ = (maxZ - minZ) / static_cast<coord_t>(nBinsZ);

  const vtkIdType nPointsX = nBinsX + 1;
  const vtkIdType nPointsY = nBinsY + 1;
  const vtkIdType nPointsZ = nBinsZ + 1;

  vtkFloatArray *pointsarray = vtkFloatArray::SafeDownCast(points->GetData());
  if (pointsarray == NULL) {
    throw std::runtime_error("Failed to cast vtkDataArray to vtkFloatArray.");
  } else if (pointsarray->GetNumberOfComponents() != 3) {
    throw std::runtime_error("points array must have 3 components.");
  }
  float *it = pointsarray->WritePointer(0, nPointsX * nPointsY * nPointsZ * 3);
  // Array with the point IDs (only set where needed)
  progressFactor = 0.5 / static_cast<double>(nPointsZ);
  double progressOffset = 0.5;
  for (int z = 0; z < nPointsZ; z++) {
    // Report progress updates for the last 50%
    progressUpdate.eventRaised(double(z) * progressFactor + progressOffset);
    in[1] = (minZ +
             (static_cast<coord_t>(z) * incrementZ)); // Calculate increment in z;
    for (int y = 0; y < nPointsY; y++) {
      in[0] = (minY + (static_cast<coord_t>(y) *
                       incrementY)); // Calculate increment in y;
      for (int x = 0; x < nPointsX; x++) {
        it[0] = (minX + (static_cast<coord_t>(x) *
                        incrementX)); // Calculate increment in x;
        it[1] = in[0];
        it[2] = in[1];
        std::advance(it, 3);
      }
    }
  }

  visualDataSet->SetPoints(points.GetPointer());
  visualDataSet->Register(NULL);
  visualDataSet->Squeeze();

  // Hedge against empty data sets
  if (visualDataSet->GetNumberOfPoints() <= 0) {
    vtkNullStructuredGrid nullGrid;
    visualDataSet = nullGrid.createNullData();
  }

  vtkSmartPointer<vtkDataSet> dataset = visualDataSet;
  return dataset;

}

/**
Create the vtkStructuredGrid from the provided workspace
@param progressUpdating: Reporting object to pass progress information up the
stack.
@return fully constructed vtkDataSet.
*/
vtkSmartPointer<vtkDataSet>
vtkMDHistoHexFactory::create(ProgressAction &progressUpdating) const {
  auto product =
      tryDelegatingCreation<MDHistoWorkspace, 3>(m_workspace, progressUpdating);
  if (product != NULL) {
    return product;
  } else {
    // Create in 3D mode
    return this->create3Dor4D(0, progressUpdating);
  }
}

vtkMDHistoHexFactory::~vtkMDHistoHexFactory() {}
}
}
