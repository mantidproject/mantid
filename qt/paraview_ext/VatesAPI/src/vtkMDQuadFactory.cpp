// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/vtkMDQuadFactory.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ReadLock.h"

#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkQuad.h>
#include <vtkUnstructuredGrid.h>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("vtkMDQuadFactory");
}

namespace Mantid {
namespace VATES {
/// Constructor
vtkMDQuadFactory::vtkMDQuadFactory(
    const VisualNormalization normalizationOption)
    : m_normalizationOption(normalizationOption) {}

/// Destructor
vtkMDQuadFactory::~vtkMDQuadFactory() {}

/**
Create the vtkStructuredGrid from the provided workspace
@param progressUpdating: Reporting object to pass progress information up the
stack.
@return fully constructed vtkDataSet.
*/
vtkSmartPointer<vtkDataSet>
vtkMDQuadFactory::create(ProgressAction &progressUpdating) const {
  auto product = tryDelegatingCreation<IMDEventWorkspace, 2>(m_workspace,
                                                             progressUpdating);
  if (product != nullptr) {
    return product;
  } else {
    g_log.warning() << "Factory " << this->getFactoryTypeName()
                    << " is being used. You are viewing data with less than "
                       "three dimensions in the VSI. \n";

    IMDEventWorkspace_sptr imdws =
        this->castAndCheck<IMDEventWorkspace, 2>(m_workspace);
    // Acquire a scoped read-only lock to the workspace (prevent segfault from
    // algos modifying ws)
    Mantid::Kernel::ReadLock lock(*imdws);

    const size_t nDims = imdws->getNumDims();
    size_t nNonIntegrated = imdws->getNonIntegratedDimensions().size();

    /*
    Write mask array with correct order for each internal dimension.
    */
    auto masks = std::make_unique<bool[]>(nDims);
    for (size_t i_dim = 0; i_dim < nDims; ++i_dim) {
      bool bIntegrated = imdws->getDimension(i_dim)->getIsIntegrated();
      masks[i_dim] =
          !bIntegrated; // TRUE for unmaksed, integrated dimensions are masked.
    }

    // Make iterator, which will use the desired normalization. Ensure
    // destruction in any eventuality.
    auto it =
        createIteratorWithNormalization(m_normalizationOption, imdws.get());

    // Create 4 points per box.
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(it->getDataSize() * 4);

    // One scalar per box
    vtkNew<vtkFloatArray> signals;
    signals->Allocate(it->getDataSize());
    signals->SetName(vtkDataSetFactory::ScalarName.c_str());
    signals->SetNumberOfComponents(1);

    size_t nVertexes;

    auto visualDataSet = vtkSmartPointer<vtkUnstructuredGrid>::New();
    visualDataSet->Allocate(it->getDataSize());

    vtkNew<vtkIdList> quadPointList;
    quadPointList->SetNumberOfIds(4);

    Mantid::API::CoordTransform const *transform = nullptr;
    if (m_useTransform) {
      transform = imdws->getTransformToOriginal();
    }

    Mantid::coord_t out[2];
    auto useBox = std::vector<bool>(it->getDataSize());

    double progressFactor = 0.5 / double(it->getDataSize());
    double progressOffset = 0.5;

    size_t iBox = 0;
    do {
      progressUpdating.eventRaised(progressFactor * double(iBox));

      Mantid::signal_t signal = it->getNormalizedSignal();
      if (std::isfinite(signal)) {
        useBox[iBox] = true;
        signals->InsertNextValue(static_cast<float>(signal));

        auto coords =
            it->getVertexesArray(nVertexes, nNonIntegrated, masks.get());
        // Iterate through all coordinates. Candidate for speed improvement.
        for (size_t v = 0; v < nVertexes; ++v) {
          coord_t *coord = coords.get() + v * 2;
          size_t id = iBox * 4 + v;
          if (m_useTransform) {
            transform->apply(coord, out);
            points->SetPoint(id, out[0], out[1], 0);
          } else {
            points->SetPoint(id, coord[0], coord[1], 0);
          }
        }
      } // valid number of vertexes returned
      else {
        useBox[iBox] = false;
      }
      ++iBox;
    } while (it->next());

    for (size_t ii = 0; ii < it->getDataSize(); ++ii) {
      progressUpdating.eventRaised((progressFactor * double(ii)) +
                                   progressOffset);

      if (useBox[ii] == true) {
        vtkIdType pointIds = vtkIdType(ii * 4);

        quadPointList->SetId(0, pointIds + 0); // xyx
        quadPointList->SetId(1, pointIds + 1); // dxyz
        quadPointList->SetId(2, pointIds + 3); // dxdyz
        quadPointList->SetId(3, pointIds + 2); // xdyz
        visualDataSet->InsertNextCell(VTK_QUAD, quadPointList.GetPointer());
      } // valid number of vertexes returned
    }

    signals->Squeeze();
    points->Squeeze();

    visualDataSet->SetPoints(points.GetPointer());
    visualDataSet->GetCellData()->SetScalars(signals.GetPointer());
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

/// Initalize with a target workspace.
void vtkMDQuadFactory::initialize(const Mantid::API::Workspace_sptr &ws) {
  m_workspace = doInitialize<IMDEventWorkspace, 2>(ws);
}

/// Get the name of the type.
std::string vtkMDQuadFactory::getFactoryTypeName() const {
  return "vtkMDQuadFactory";
}

/// Template Method pattern to validate the factory before use.
void vtkMDQuadFactory::validate() const {
  if (!m_workspace) {
    throw std::runtime_error(
        "vtkMDQuadFactory has no workspace to run against");
  }
}
} // namespace VATES
} // namespace Mantid
