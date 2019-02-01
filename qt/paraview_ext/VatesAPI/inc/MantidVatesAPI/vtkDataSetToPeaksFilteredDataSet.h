// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_PeaksFilter_H
#define MANTID_VATES_PeaksFilter_H

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/PeakShapeBase.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "vtkSmartPointer.h"
#include <string>

class vtkUnstructuredGrid;

namespace Mantid {
namespace VATES {

/**
  Class that selects only those data points which lie within the geometry of a
  peak.

  @date 17/02/2015
*/

class DLLExport vtkDataSetToPeaksFilteredDataSet {
public:
  vtkDataSetToPeaksFilteredDataSet(vtkSmartPointer<vtkUnstructuredGrid> input,
                                   vtkSmartPointer<vtkUnstructuredGrid> output);
  vtkDataSetToPeaksFilteredDataSet(const vtkDataSetToPeaksFilteredDataSet &) =
      default;
  vtkDataSetToPeaksFilteredDataSet &
  operator=(const vtkDataSetToPeaksFilteredDataSet &) = delete;
  virtual ~vtkDataSetToPeaksFilteredDataSet();
  /// Set the name of the peaks workspace
  void initialize(
      const std::vector<Mantid::API::IPeaksWorkspace_sptr> &peaksWorkspaces,
      double radiusNoShape, Geometry::PeakShape::RadiusType radiusType,
      int coordinateSystem);
  /// Apply the peak filtering
  void execute(ProgressAction &progressUpdating);
  /// Get radius of no shape
  double getRadiusNoShape();
  /// Get radius factor
  double getRadiusFactor();

private:
  std::vector<std::pair<Mantid::Kernel::V3D, double>> getPeaksInfo(
      const std::vector<Mantid::API::IPeaksWorkspace_sptr> &peaksWorkspaces);
  Kernel::V3D getPeakPosition(const Mantid::Geometry::IPeak &peak);
  double getPeakRadius(const Mantid::Geometry::PeakShape &shape);
  double m_radiusNoShape; ///< The radius for peaks with no peak shape.
  double m_radiusFactor;  ///< By how much we want to trim the data set.
  double m_defaultRadius; ///< A default radius.
  Geometry::PeakShape::RadiusType m_radiusType;
  bool m_isInitialised; ///< Flag if the filter is initialized
  Mantid::Kernel::SpecialCoordinateSystem
      m_coordinateSystem;                            ///< A coordinate system.
  vtkSmartPointer<vtkUnstructuredGrid> m_inputData;  ///< Data to peak filter
  vtkSmartPointer<vtkUnstructuredGrid> m_outputData; ///< Peak filtered data
  std::vector<Mantid::API::IPeaksWorkspace_sptr>
      m_peaksWorkspaces; ///< A list of peaks workspace names.
};
} // namespace VATES
} // namespace Mantid
#endif
