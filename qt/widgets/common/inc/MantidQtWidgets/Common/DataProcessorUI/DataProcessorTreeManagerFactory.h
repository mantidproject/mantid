#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGERFACTORY_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGERFACTORY_H
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorTreeManager.h"
#include <memory>
namespace MantidQt {
namespace MantidWidgets {
class GenericDataProcessorPresenter;
/** @class DataProcessorTreeManagerFactory

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DataProcessorTreeManagerFactory {
public:
  enum class PostProcessing { Yes, No };
  virtual std::pair<std::unique_ptr<DataProcessorTreeManager>, PostProcessing>
  fromPostProcessorName(GenericDataProcessorPresenter& presenter,
                        const QString &postprocessorName,
                        DataProcessorWhiteList whitelist) = 0;
};
}
}
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGERFACTORY_H
