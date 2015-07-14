#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"

#include "MantidVatesAPI/vtkStructuredPointsArray.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidKernel/ReadLock.h"

#include "vtkNew.h"
#include "vtkStructuredGrid.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"


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
vtkDataSet *
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

  const coord_t maxX = m_workspace->getXDimension()->getMaximum();
  const coord_t minX = m_workspace->getXDimension()->getMinimum();
  const coord_t maxY = m_workspace->getYDimension()->getMaximum();
  const coord_t minY = m_workspace->getYDimension()->getMinimum();
  const coord_t maxZ = m_workspace->getZDimension()->getMaximum();
  const coord_t minZ = m_workspace->getZDimension()->getMinimum();

  coord_t incrementX = (maxX - minX) / static_cast<coord_t>(nBinsX);
  coord_t incrementY = (maxY - minY) / static_cast<coord_t>(nBinsY);
  coord_t incrementZ = (maxZ - minZ) / static_cast<coord_t>(nBinsZ);

  const int imageSize = (nBinsX) * (nBinsY) * (nBinsZ);
  
  vtkNew<vtkStructuredGrid> visualDataSet;
  visualDataSet->SetDimensions(nBinsX+1,nBinsY+1,nBinsZ+1);

  const int nPointsX = nBinsX+1;
  const int nPointsY = nBinsY+1;
  const int nPointsZ = nBinsZ+1;

  // Array with true where the voxel should be shown
  double progressFactor = 0.5 / double(imageSize);
  double progressOffset = 0.5;

  std::size_t offset = 0;
  if (nDims == 4)
  {
    offset = timestep * indexMultiplier[2];
  }

  if(m_normalizationOption == VATES::NoNormalization)
  {
    vtkNew<vtkDoubleArray> signal;
    signal->SetName(vtkDataSetFactory::ScalarName.c_str());
    signal->SetArray(m_workspace->getSignalArray()+offset,imageSize,1);

    for (vtkIdType index = 0; index < imageSize; ++index) {
      progressUpdate.eventRaised(double(index) * progressFactor);
      double signalScalar = signal->GetValue(index);
      bool maskValue = (isSpecial( signalScalar ) || !m_thresholdRange->inRange(signalScalar));
      if (maskValue)
      {
        visualDataSet->BlankCell(index);
      }
    }

    visualDataSet->GetCellData()->SetScalars(signal.GetPointer());
  }
  else
  {
    vtkNew<vtkFloatArray> signal;
    signal->SetName(vtkDataSetFactory::ScalarName.c_str());
    signal->Allocate(static_cast<int>(imageSize));

    boost::scoped_ptr<MDHistoWorkspaceIterator> iterator(
           dynamic_cast<MDHistoWorkspaceIterator *>(createIteratorWithNormalization(
           m_normalizationOption, m_workspace.get())));
    iterator->jumpTo(offset);
    for (vtkIdType index = 0; index < imageSize; ++index) {
      progressUpdate.eventRaised(double(index) * progressFactor);
      // Normalized by the requested method applied above.
      const double signalScalar = iterator->getNormalizedSignal();
      signal->InsertNextValue(signalScalar);
      bool maskValue = (isSpecial( signalScalar ) || !m_thresholdRange->inRange(signalScalar));
      if (maskValue)
      {
        visualDataSet->BlankCell(index);
      }
      iterator->next();
    }
    visualDataSet->GetCellData()->SetScalars(signal.GetPointer());
  }

  /*vtkNew<vtkPoints> points;
  points->Allocate(static_cast<int>(imageSize));
  //Get the transformation that takes the points in the TRANSFORMED space back
  //into the ORIGINAL (not-rotated) space.
  Mantid::API::CoordTransform const *transform = NULL;
  if (m_useTransform)
    transform = m_workspace->getTransformToOriginal();

  Mantid::coord_t in[3];
  Mantid::coord_t out[3];

  progressFactor = 0.5 / static_cast<double>(nPointsZ);

  for (int z = 0; z < nPointsZ; z++) {
    // Report progress updates for the last 50%
    progressUpdate.eventRaised(double(z) * progressFactor + progressOffset);
    in[2] = (minZ + (static_cast<coord_t>(z) *
                     incrementZ)); // Calculate increment in z;
    for (int y = 0; y < nPointsY; y++) {
      in[1] = (minY + (static_cast<coord_t>(y) *
                       incrementY)); // Calculate increment in y;
      for (int x = 0; x < nPointsX; x++) {
        // Create the point only when needed
        in[0] = (minX + (static_cast<coord_t>(x) * incrementX)); // Calculate increment in x;
        if (transform)
        {
            transform->apply(in, out);
            points->InsertNextPoint(out);
        }
        else
        {
            points->InsertNextPoint(in);
        }
      }
    }
  }*/
    
  vtkNew<vtkStructuredPointsArray<double>> implicitPoints;
  implicitPoints->InitializeArray(m_workspace.get(),m_useTransform);
  vtkNew<vtkPoints> newPoints;
  newPoints->SetData(implicitPoints.GetPointer());
  /*for ( auto i = 0; i < points->GetNumberOfPoints();++i)
    {
      double* oldPoint = points->GetPoint(i);
      double* newPoint = newPoints->GetPoint(i);
      for (auto j = 0; j < 3;++j)
      {
        if (std::abs(newPoint[j]-oldPoint[j])>1.0e-4)
          std::cout << "error at point #" << i << std::endl;
      }
    }*/
  
    
  visualDataSet->SetPoints(newPoints.GetPointer());
  visualDataSet->Register(NULL);
  visualDataSet->Squeeze();

  // TODO: fix for vtkStructuredGrid
  // Hedge against empty data sets
  //if (visualDataSet->GetNumberOfPoints() <= 0) {
  //  visualDataSet->Delete();
  //  vtkNullUnstructuredGrid nullGrid;
  //  visualDataSet = nullGrid.createNullData();
  //}

  return visualDataSet.GetPointer();
}

/**
Create the vtkStructuredGrid from the provided workspace
@param progressUpdating: Reporting object to pass progress information up the
stack.
@return fully constructed vtkDataSet.
*/
vtkDataSet *
vtkMDHistoHexFactory::create(ProgressAction &progressUpdating) const {
  vtkDataSet *product =
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
