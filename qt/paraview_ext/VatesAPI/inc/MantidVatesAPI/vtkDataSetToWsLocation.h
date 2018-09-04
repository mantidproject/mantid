#ifndef VTKDATASET_TO_WS_LOCATION_H
#define VTKDATASET_TO_WS_LOCATION_H

#include "MantidKernel/System.h"
#include <string>

class vtkDataSet;
namespace Mantid {
namespace VATES {

/** @class vtkDataSetToImplicitFunction

Handles the extraction of existing ws location from a vtkDataSet by getting at
the field data and then processing the xml contained within.

@author Owen Arnold, Tessella Support Services plc
@date 22/08/2011

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class DLLExport vtkDataSetToWsLocation {
public:
  vtkDataSetToWsLocation &
  operator=(const vtkDataSetToWsLocation &other) = delete;
  vtkDataSetToWsLocation(const vtkDataSetToWsLocation &other) = delete;
  static std::string exec(vtkDataSet *dataSet);
  vtkDataSetToWsLocation(vtkDataSet *dataSet);
  std::string execute();
  ~vtkDataSetToWsLocation();

private:
  vtkDataSet *m_dataset;
};
} // namespace VATES
} // namespace Mantid

#endif