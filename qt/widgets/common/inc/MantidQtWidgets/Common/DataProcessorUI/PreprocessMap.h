#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORPREPROCESSMAP_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORPREPROCESSMAP_H

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PreprocessingAlgorithm.h"
#include <map>

#include <QString>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class PreprocessMap

PreprocessMap defines a pre-processor algorithm that will
be
responsible for pre-processsing a specific column in a Data Processor UI.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class EXPORT_OPT_MANTIDQT_COMMON PreprocessMap {
public:
  // Default constructor
  PreprocessMap();
  // Destructor
  virtual ~PreprocessMap();
  // Add a column to pre-process
  void addElement(const QString &column, const QString &algorithm,
                  const QString &prefix = "", const QString &separator = "",
                  const QString &blacklist = "");
  // Returns a map where keys are columns and values pre-processing algorithms
  std::map<QString, PreprocessingAlgorithm> asMap() const;

private:
  // A map where keys are columns and values pre-processing algorithms
  std::map<QString, PreprocessingAlgorithm> m_map;
};
}
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORPREPROCESSMAP_H*/
