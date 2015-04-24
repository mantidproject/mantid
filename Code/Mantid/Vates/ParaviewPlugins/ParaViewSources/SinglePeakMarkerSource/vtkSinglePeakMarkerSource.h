#ifndef _vtkSinglePeakMarkerSource_h
#define _vtkSinglePeakMarkerSource_h
#include "vtkPolyDataAlgorithm.h"

/**
    This source is used to mark a single peak.

    @date 23/02/2015

    Copyright &copy; 2007-11 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>

*/

// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkSinglePeakMarkerSource : public vtkPolyDataAlgorithm
{
public:
  static vtkSinglePeakMarkerSource*New();
  vtkTypeMacro(vtkSinglePeakMarkerSource,vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent);
  void SetRadiusMarker(double radius);
  void SetPosition1(double position1);
  void SetPosition2(double position2);
  void SetPosition3(double position3);
protected:
  vtkSinglePeakMarkerSource();
  ~vtkSinglePeakMarkerSource();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  /// Position information
  double m_position1;
  double m_position2;
  double m_position3;
  double m_radius;

  vtkSinglePeakMarkerSource(const vtkSinglePeakMarkerSource&);
  void operator = (const vtkSinglePeakMarkerSource&);
};
#endif
