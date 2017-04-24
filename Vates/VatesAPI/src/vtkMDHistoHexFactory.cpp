#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkNullStructuredGrid.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidKernel/ReadLock.h"

// For vtkMDHWSignalArray.h
#include "vtkArrayDispatchArrayList.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkSMPTools.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"

#include <cmath>

using namespace Mantid::DataObjects;
using Mantid::Kernel::ReadLock;

namespace Mantid {
namespace VATES {

vtkMDHistoHexFactory::vtkMDHistoHexFactory(
    const VisualNormalization normalizationOption)
    : m_normalizationOption(normalizationOption) {}

/**
Assigment operator
@param other : vtkMDHistoHexFactory to assign to this instance from.
@return ref to assigned current instance.
*/
vtkMDHistoHexFactory &vtkMDHistoHexFactory::
operator=(const vtkMDHistoHexFactory &other) {
  if (this != &other) {
    this->m_normalizationOption = other.m_normalizationOption;
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
  this->m_workspace = other.m_workspace;
}

void vtkMDHistoHexFactory::initialize(
    const Mantid::API::Workspace_sptr &workspace) {
  m_workspace = doInitialize<MDHistoWorkspace, 3>(workspace);
}

void vtkMDHistoHexFactory::validateWsNotNull() const {
  if (!m_workspace) {
    throw std::runtime_error("IMDWorkspace is null");
  }
}

void vtkMDHistoHexFactory::validate() const { validateWsNotNull(); }

namespace {

template <class Array> struct CellGhostArrayWorker {
  Array *m_signal;
  vtkUnsignedCharArray *m_cga;
  CellGhostArrayWorker(Array *signal, vtkUnsignedCharArray *cga)
      : m_signal(signal), m_cga(cga) {}
  void operator()(vtkIdType begin, vtkIdType end) {
    for (vtkIdType index = begin; index < end; ++index) {
      if (!std::isfinite(m_signal->GetValue(index))) {
        m_cga->SetValue(index, m_cga->GetValue(index) |
                                   vtkDataSetAttributes::HIDDENCELL);
      }
    }
  }
};

struct PointsWorker {
  vtkPoints *m_pts;
  coord_t incrementX, incrementY, incrementZ;
  coord_t minX, minY, minZ;
  vtkIdType nPointsX, nPointsY;
  PointsWorker(Mantid::DataObjects::MDHistoWorkspace &ws, vtkPoints *pts)
      : m_pts(pts) {
    int nBinsX = static_cast<int>(ws.getXDimension()->getNBins());
    int nBinsY = static_cast<int>(ws.getYDimension()->getNBins());
    int nBinsZ = static_cast<int>(ws.getZDimension()->getNBins());

    minX = ws.getXDimension()->getMinimum();
    minY = ws.getYDimension()->getMinimum();
    minZ = ws.getZDimension()->getMinimum();
    coord_t maxX = ws.getXDimension()->getMaximum();
    coord_t maxY = ws.getYDimension()->getMaximum();
    coord_t maxZ = ws.getZDimension()->getMaximum();

    incrementX = (maxX - minX) / static_cast<coord_t>(nBinsX);
    incrementY = (maxY - minY) / static_cast<coord_t>(nBinsY);
    incrementZ = (maxZ - minZ) / static_cast<coord_t>(nBinsZ);

    nPointsX = nBinsX + 1;
    nPointsY = nBinsY + 1;
  }
  void operator()(vtkIdType begin, vtkIdType end) {
    float in[3];
    vtkIdType pos = begin * nPointsX * nPointsY;
    for (int z = static_cast<int>(begin); z < static_cast<int>(end); ++z) {
      in[2] = minZ + static_cast<coord_t>(z) * incrementZ;
      for (int y = 0; y < nPointsY; ++y) {
        in[1] = minY + static_cast<coord_t>(y) * incrementY;
        for (int x = 0; x < nPointsX; ++x) {
          in[0] = minX + static_cast<coord_t>(x) * incrementX;
          m_pts->SetPoint(pos, in);
          ++pos;
        }
      }
    }
  }
};
} // end anon namespace

template <class ValueTypeT>
static void InitializevtkMDHWSignalArray(
    const MDHistoWorkspace &ws, VisualNormalization normalization,
    vtkIdType offset, vtkMDHWSignalArray<ValueTypeT> *signal) {
  const vtkIdType nBinsX = static_cast<int>(ws.getXDimension()->getNBins());
  const vtkIdType nBinsY = static_cast<int>(ws.getYDimension()->getNBins());
  const vtkIdType nBinsZ = static_cast<int>(ws.getZDimension()->getNBins());
  const vtkIdType imageSize = (nBinsX) * (nBinsY) * (nBinsZ);
  auto norm = static_cast<SignalArrayNormalization>(normalization);

  signal->InitializeArray(ws.getSignalArray(), ws.getNumEventsArray(),
                          ws.getInverseVolume(), norm, imageSize, offset);
}

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
                                   ProgressAction &progress) const {
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

  const vtkIdType imageSize = static_cast<vtkIdType>(nBinsX) * nBinsY * nBinsZ;

  auto visualDataSet = vtkSmartPointer<vtkStructuredGrid>::New();
  visualDataSet->SetDimensions(nBinsX + 1, nBinsY + 1, nBinsZ + 1);

  // Array with true where the voxel should be shown

  vtkIdType offset = 0;
  if (nDims == 4) {
    offset = timestep * indexMultiplier[2];
  }

  VisualNormalization norm;
  if (m_normalizationOption == AutoSelect) {
    // enum to enum.
    norm =
        static_cast<VisualNormalization>(m_workspace->displayNormalization());
  } else {
    norm = static_cast<VisualNormalization>(m_normalizationOption);
  }

  progress.eventRaised(0.0);

  vtkDataArray *signal = nullptr;
  if (norm == NoNormalization) {
    vtkNew<vtkDoubleArray> raw;
    raw->SetVoidArray(m_workspace->getSignalArray(), imageSize, 1);
    visualDataSet->GetCellData()->SetScalars(raw.Get());
    auto cga = visualDataSet->AllocateCellGhostArray();
    CellGhostArrayWorker<vtkDoubleArray> cgafunc(raw.Get(), cga);
    vtkSMPTools::For(0, imageSize, cgafunc);
    signal = raw.Get();
  } else {
    vtkNew<vtkMDHWSignalArray<double>> normalized;
    InitializevtkMDHWSignalArray(*m_workspace, norm, offset, normalized.Get());
    visualDataSet->GetCellData()->SetScalars(normalized.GetPointer());
    auto cga = visualDataSet->AllocateCellGhostArray();
    CellGhostArrayWorker<vtkMDHWSignalArray<double>> cgafunc(normalized.Get(),
                                                             cga);
    vtkSMPTools::For(0, imageSize, cgafunc);
    signal = normalized.Get();
  }

  signal->SetName(vtkDataSetFactory::ScalarName.c_str());
  progress.eventRaised(0.33);

  vtkNew<vtkPoints> points;
  const vtkIdType nPointsX = nBinsX + 1;
  const vtkIdType nPointsY = nBinsY + 1;
  const vtkIdType nPointsZ = nBinsZ + 1;
  points->SetNumberOfPoints(nPointsX * nPointsY * nPointsZ);

  PointsWorker ptsfunc(*m_workspace, points.GetPointer());
  vtkSMPTools::For(0, nPointsZ, ptsfunc);
  progress.eventRaised(0.67);

  visualDataSet->SetPoints(points.GetPointer());
  visualDataSet->Register(nullptr);
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
  if (product) {
    return product;
  } else {
    // Create in 3D mode
    return this->create3Dor4D(0, progressUpdating);
  }
}

vtkMDHistoHexFactory::~vtkMDHistoHexFactory() {}
}
}
