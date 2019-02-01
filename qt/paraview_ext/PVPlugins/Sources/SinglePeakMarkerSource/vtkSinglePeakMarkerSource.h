// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef _vtkSinglePeakMarkerSource_h
#define _vtkSinglePeakMarkerSource_h
#include "vtkPolyDataAlgorithm.h"

/**
    This source is used to mark a single peak.

    @date 23/02/2015

*/

// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkSinglePeakMarkerSource : public vtkPolyDataAlgorithm {
public:
  static vtkSinglePeakMarkerSource *New();
  vtkSinglePeakMarkerSource(const vtkSinglePeakMarkerSource &) = delete;
  vtkSinglePeakMarkerSource &
  operator=(const vtkSinglePeakMarkerSource &) = delete;
  // clang-format off
  vtkTypeMacro(vtkSinglePeakMarkerSource, vtkPolyDataAlgorithm)
  void PrintSelf(ostream &os, vtkIndent indent) override;
  // clang-format on
  void SetRadiusMarker(double radius);
  void SetPosition1(double position1);
  void SetPosition2(double position2);
  void SetPosition3(double position3);

protected:
  vtkSinglePeakMarkerSource();
  ~vtkSinglePeakMarkerSource() override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

private:
  /// Position information
  double m_position1;
  double m_position2;
  double m_position3;
  double m_radius;
};
#endif
