#include "MantidVatesAPI/vtkDataSetToPeaksFilteredDataSet.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/ReadLock.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidVatesAPI/ProgressAction.h"

#include <vtkExtractSelection.h>
#include <vtkIdTypeArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkFieldData.h>
#include <vtkIdList.h>
#include <vtkFieldData.h>

#include <boost/shared_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace Mantid {
namespace VATES {
/**
  * Standard constructor for object.
  * @param input : The dataset to peaks filter
  * @param output : The resulting peaks filtered dataset
  */
vtkDataSetToPeaksFilteredDataSet::vtkDataSetToPeaksFilteredDataSet(
    vtkSmartPointer<vtkUnstructuredGrid> input,
    vtkSmartPointer<vtkUnstructuredGrid> output)
    : m_radiusNoShape(0.2), m_radiusFactor(2), m_defaultRadius(0.1),
      m_radiusType(Geometry::PeakShape::RadiusType::Radius),
      m_isInitialised(false),
      m_coordinateSystem(Mantid::Kernel::SpecialCoordinateSystem::None),
      m_inputData(input), m_outputData(output) {
  if (nullptr == input) {
    throw std::runtime_error("Cannot construct "
                             "vtkDataSetToPeaksFilteredDataSet with NULL input "
                             "vtkUnstructuredGrid");
  }
  if (nullptr == output) {
    throw std::runtime_error("Cannot construct "
                             "vtkDataSetToPeaksFilteredDataSet with NULL "
                             "output vtkUnstructuredGrid");
  }
}

vtkDataSetToPeaksFilteredDataSet::~vtkDataSetToPeaksFilteredDataSet() {}

/**
  * Set the value for the underlying peaks workspace
  * @param peaksWorkspaces : A list of peak workspace names.
  * @param radiusNoShape : The peak radius for no shape.
  * @param radiusType : The type of the radius: Radius(0), Outer Radius(10,
 * Inner Radius(1)
  * @param coordinateSystem: A coordinate system.
  */
void vtkDataSetToPeaksFilteredDataSet::initialize(
    const std::vector<Mantid::API::IPeaksWorkspace_sptr> &peaksWorkspaces,
    double radiusNoShape, Geometry::PeakShape::RadiusType radiusType,
    int coordinateSystem) {
  m_peaksWorkspaces = peaksWorkspaces;
  m_radiusNoShape = radiusNoShape;
  m_radiusType = radiusType;
  m_isInitialised = true;
  m_coordinateSystem =
      static_cast<Mantid::Kernel::SpecialCoordinateSystem>(coordinateSystem);
}

/**
  * Process the input data. First, get all the peaks and their associated
  * geometry. Then filter
  * through the input to find the peaks which lie within a peak. Then apply then
  * to the output data.
  * Then update the metadata. See
  * http://www.vtk.org/Wiki/VTK/Examples/Cxx/PolyData/ExtractSelection
  * @param progressUpdating The handle for the progress bar.
  */
void vtkDataSetToPeaksFilteredDataSet::execute(
    ProgressAction &progressUpdating) {
  if (!m_isInitialised) {
    throw std::runtime_error("vtkDataSetToPeaksFilteredDataSet needs "
                             "initialize run before executing");
  }

  // Get the peaks location and the radius information
  std::vector<std::pair<Mantid::Kernel::V3D, double>> peaksInfo =
      getPeaksInfo(m_peaksWorkspaces);

  // Compare each element of the vtk data set and check which ones to keep
  vtkPoints *points = m_inputData->GetPoints();

  vtkSmartPointer<vtkIdTypeArray> ids = vtkSmartPointer<vtkIdTypeArray>::New();
  ids->SetNumberOfComponents(1);

  double progressFactor =
      1.0 / static_cast<double>(points->GetNumberOfPoints());
  for (int i = 0; i < points->GetNumberOfPoints(); i++) {
    progressUpdating.eventRaised(double(i) * progressFactor);
    double point[3];
    points->GetPoint(i, point);

    // Compare to Peaks
    for (const auto &peak : peaksInfo) {
      // Calcuate the difference between the vtkDataSet point and the peak.
      // Needs
      // to be smaller than the radius

      double squaredDifference = 0.;
      for (unsigned k = 0; k < 3; k++) {
        squaredDifference += pow(point[k] - peak.first[k], 2);
      }

      if (squaredDifference <= pow(peak.second, 2)) {
        ids->InsertNextValue(i);
        break;
      }
    }
  }

  // Create the selection node and tell it the type of selection
  auto selectionNode = vtkSmartPointer<vtkSelectionNode>::New();
  selectionNode->SetFieldType(vtkSelectionNode::POINT);
  selectionNode->SetContentType(vtkSelectionNode::INDICES);
  selectionNode->SetSelectionList(ids);

  auto selection = vtkSmartPointer<vtkSelection>::New();
  selection->AddNode(selectionNode);

  // We are not setting up a pipeline here, cannot access vtkAlgorithmOutput
  auto extractSelection = vtkSmartPointer<vtkExtractSelection>::New();
  extractSelection->SetInputData(0, m_inputData);
  extractSelection->SetInputData(1, selection);
  extractSelection->Update();

  // Extract
  m_outputData->ShallowCopy(extractSelection->GetOutput());
}

/**
 * Get the peaks information which is the position and the largest radius of the
 * peak.
 * @param peaksWorkspaces A list of peaks workspaces
 * @returns A list of pair information which contains the position and the
 * radius.
 */
std::vector<std::pair<Mantid::Kernel::V3D, double>>
vtkDataSetToPeaksFilteredDataSet::getPeaksInfo(
    const std::vector<Mantid::API::IPeaksWorkspace_sptr> &peaksWorkspaces) {
  std::vector<std::pair<Mantid::Kernel::V3D, double>> peaksInfo;
  // Iterate over all peaksworkspaces and add the their info to the output
  // vector
  for (const auto &workspace : peaksWorkspaces) {
    int numPeaks = workspace->getNumberPeaks();
    // Iterate over all peaks for the workspace
    for (int i = 0; i < numPeaks; i++) {
      const Mantid::Geometry::IPeak &peak = workspace->getPeak(i);
      peaksInfo.emplace_back(this->getPeakPosition(peak),
                             this->getPeakRadius(peak.getPeakShape()));
    }
  }
  return peaksInfo;
}

GNU_DIAG_OFF("strict-aliasing")
/**
 * Get the radius from a PeakShape object.
 * @param shape The PeakShape from which the information will be extracted.
 * @return radius of peak shape.
 */
double vtkDataSetToPeaksFilteredDataSet::getPeakRadius(
    const Mantid::Geometry::PeakShape &shape) {
  double radius = m_defaultRadius;

  boost::optional<double> rad = shape.radius(m_radiusType);
  if (rad.is_initialized()) {
    radius = rad.get();
  } else {
    if (shape.shapeName() == Mantid::DataObjects::NoShape::noShapeName()) {
      radius = m_radiusNoShape;
    }
  }

  return radius * m_radiusFactor;
}

/**
 * Get the position of a peak object.
 * @param peak The Peak from which the information will be extracted.
 * @return position of peak shape in the specified coordinate system.
 */
Kernel::V3D vtkDataSetToPeaksFilteredDataSet::getPeakPosition(
    const Mantid::Geometry::IPeak &peak) {
  // Get the position in the correct frame.
  Kernel::V3D position;
  switch (m_coordinateSystem) {
  case (Mantid::Kernel::SpecialCoordinateSystem::HKL):
    position = peak.getHKL();
    break;
  case (Mantid::Kernel::SpecialCoordinateSystem::QLab):
    position = peak.getQLabFrame();
    break;
  case (Mantid::Kernel::SpecialCoordinateSystem::QSample):
    position = peak.getQSampleFrame();
    break;
  default:
    throw std::invalid_argument("The special coordinate systems don't match.");
  }
  return position;
}

/**
 * Get the radius for no shape
 * @returns The shape of the radius.
 */
double vtkDataSetToPeaksFilteredDataSet::getRadiusNoShape() {
  return m_radiusNoShape;
}

/**
 * Get the radius factor which is used to calculate the radius of the culled
 * data set around each peak
 * The culling radius is the radius of the peak times the radius factor.
 * @returns The radius factor.
 */
double vtkDataSetToPeaksFilteredDataSet::getRadiusFactor() {
  return m_radiusFactor;
}
}
}
