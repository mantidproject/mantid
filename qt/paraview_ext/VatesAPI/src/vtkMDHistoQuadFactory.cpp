// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/vtkMDHistoQuadFactory.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ReadLock.h"

#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkQuad.h"
#include <vector>

using Mantid::DataObjects::MDHistoWorkspace;
using Mantid::DataObjects::MDHistoWorkspaceIterator;
using Mantid::Kernel::CPUTimer;

namespace {
Mantid::Kernel::Logger g_log("vtkMDHistoQuadFactory");
}

namespace Mantid {

namespace VATES {
vtkMDHistoQuadFactory::vtkMDHistoQuadFactory(
    const VisualNormalization normalizationOption)
    : m_normalizationOption(normalizationOption) {}

/**
Assigment operator
@param other : vtkMDHistoQuadFactory to assign to this instance from.
@return ref to assigned current instance.
*/
vtkMDHistoQuadFactory &vtkMDHistoQuadFactory::
operator=(const vtkMDHistoQuadFactory &other) {
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
vtkMDHistoQuadFactory::vtkMDHistoQuadFactory(
    const vtkMDHistoQuadFactory &other) {
  this->m_normalizationOption = other.m_normalizationOption;
  this->m_workspace = other.m_workspace;
}

/**
Create the vtkStructuredGrid from the provided workspace
@param progressUpdating: Reporting object to pass progress information up the
stack.
@return fully constructed vtkDataSet.
*/
vtkSmartPointer<vtkDataSet>
vtkMDHistoQuadFactory::create(ProgressAction &progressUpdating) const {
  auto product =
      tryDelegatingCreation<MDHistoWorkspace, 2>(m_workspace, progressUpdating);
  if (product != nullptr) {
    return product;
  } else {
    g_log.warning() << "Factory " << this->getFactoryTypeName()
                    << " is being used. You are viewing data with less than "
                       "three dimensions in the VSI. \n";

    Mantid::Kernel::ReadLock lock(*m_workspace);
    CPUTimer tim;
    const int nBinsX =
        static_cast<int>(m_workspace->getXDimension()->getNBins());
    const int nBinsY =
        static_cast<int>(m_workspace->getYDimension()->getNBins());

    const coord_t maxX = m_workspace->getXDimension()->getMaximum();
    const coord_t minX = m_workspace->getXDimension()->getMinimum();
    const coord_t maxY = m_workspace->getYDimension()->getMaximum();
    const coord_t minY = m_workspace->getYDimension()->getMinimum();

    coord_t incrementX = (maxX - minX) / static_cast<coord_t>(nBinsX);
    coord_t incrementY = (maxY - minY) / static_cast<coord_t>(nBinsY);

    auto it = createIteratorWithNormalization(m_normalizationOption,
                                              m_workspace.get());
    auto iterator = dynamic_cast<MDHistoWorkspaceIterator *>(it.get());
    if (!iterator) {
      throw std::runtime_error(
          "Could not convert IMDIterator to a MDHistoWorkspaceIterator");
    }

    const int imageSize = (nBinsX) * (nBinsY);
    vtkNew<vtkPoints> points;
    points->Allocate(static_cast<int>(imageSize));

    vtkNew<vtkFloatArray> signal;
    signal->Allocate(imageSize);
    signal->SetName(vtkDataSetFactory::ScalarName.c_str());
    signal->SetNumberOfComponents(1);

    // The following represent actual calculated positions.

    float signalScalar;
    const int nPointsX =
        static_cast<int>(m_workspace->getXDimension()->getNBoundaries());
    const int nPointsY =
        static_cast<int>(m_workspace->getYDimension()->getNBoundaries());

    /* The idea of the next chunk of code is that you should only
    create the points that will be needed; so an array of pointNeeded
    is set so that all required vertices are marked, and created in a second
    step. */

    // Array of the points that should be created, set to false
    auto pointNeeded = std::vector<bool>(nPointsX * nPointsY, false);
    // Array with true where the voxel should be shown
    auto voxelShown = std::vector<bool>(nBinsX * nBinsY);

    double progressFactor = 0.5 / double(nBinsX);
    double progressOffset = 0.5;

    size_t index = 0;
    for (int i = 0; i < nBinsX; i++) {
      progressUpdating.eventRaised(progressFactor * double(i));

      for (int j = 0; j < nBinsY; j++) {
        index = j + nBinsY * i;
        iterator->jumpTo(index);
        signalScalar = static_cast<float>(
            iterator->getNormalizedSignal()); // Get signal normalized as per
                                              // m_normalizationOption

        if (!std::isfinite(signalScalar)) {
          // out of range
          voxelShown[index] = false;
        } else {
          // Valid data
          voxelShown[index] = true;
          signal->InsertNextValue(static_cast<float>(signalScalar));
          // Make sure all 4 neighboring points are set to true
          size_t pointIndex = i * nPointsY + j;
          pointNeeded[pointIndex] = true;
          pointIndex++;
          pointNeeded[pointIndex] = true;
          pointIndex += nPointsY - 1;
          pointNeeded[pointIndex] = true;
          pointIndex++;
          pointNeeded[pointIndex] = true;
        }
      }
    }

    std::cout << tim << " to check all the signal values.\n";

    // Get the transformation that takes the points in the TRANSFORMED space
    // back into the ORIGINAL (not-rotated) space.
    Mantid::API::CoordTransform const *transform = nullptr;
    if (m_useTransform)
      transform = m_workspace->getTransformToOriginal();

    Mantid::coord_t in[3];
    Mantid::coord_t out[3];
    in[2] = 0;

    // Array with the point IDs (only set where needed)
    std::vector<vtkIdType> pointIDs(nPointsX * nPointsY, 0);
    index = 0;
    for (int i = 0; i < nPointsX; i++) {
      progressUpdating.eventRaised((progressFactor * double(i)) +
                                   progressOffset);
      in[0] = minX + (static_cast<coord_t>(i) *
                      incrementX); // Calculate increment in x;
      for (int j = 0; j < nPointsY; j++) {
        // Create the point only when needed
        if (pointNeeded[index]) {
          in[1] = minY + (static_cast<coord_t>(j) *
                          incrementY); // Calculate increment in y;
          if (transform) {
            transform->apply(in, out);
            pointIDs[index] = points->InsertNextPoint(out);
          } else
            pointIDs[index] = points->InsertNextPoint(in);
        }
        index++;
      }
    }

    std::cout << tim << " to create the needed points.\n";

    auto visualDataSet = vtkSmartPointer<vtkUnstructuredGrid>::New();
    visualDataSet->Allocate(imageSize);
    visualDataSet->SetPoints(points.GetPointer());
    visualDataSet->GetCellData()->SetScalars(signal.GetPointer());

    // ------ Quad creation ----------------
    vtkNew<vtkQuad> quad; // Significant speed increase by creating ONE quad
                          // (assume vtkNew doesn't add significant
                          // overhead)
    index = 0;
    for (int i = 0; i < nBinsX; i++) {
      for (int j = 0; j < nBinsY; j++) {
        if (voxelShown[index]) {
          // The quad will be shown
          quad->GetPointIds()->SetId(0, pointIDs[(i)*nPointsY + j]);
          quad->GetPointIds()->SetId(1, pointIDs[(i + 1) * nPointsY + j]);
          quad->GetPointIds()->SetId(2, pointIDs[(i + 1) * nPointsY + j + 1]);
          quad->GetPointIds()->SetId(3, pointIDs[(i)*nPointsY + j + 1]);
          visualDataSet->InsertNextCell(VTK_QUAD, quad->GetPointIds());
        }
        index++;
      }
    }

    std::cout << tim << " to create and add the quads.\n";

    visualDataSet->Squeeze();

    // Hedge against empty data sets
    if (visualDataSet->GetNumberOfPoints() <= 0) {
      vtkNullUnstructuredGrid nullGrid;
      visualDataSet = nullGrid.createNullData();
    }

    vtkSmartPointer<vtkDataSet> dataSet = visualDataSet;
    return dataSet;
  }
}

void vtkMDHistoQuadFactory::initialize(
    const Mantid::API::Workspace_sptr &wspace_sptr) {
  m_workspace = doInitialize<MDHistoWorkspace, 2>(wspace_sptr);
}

void vtkMDHistoQuadFactory::validate() const {
  if (!m_workspace) {
    throw std::runtime_error("IMDWorkspace is null");
  }
}

/// Destructor
vtkMDHistoQuadFactory::~vtkMDHistoQuadFactory() {}
} // namespace VATES
} // namespace Mantid
