#ifndef MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTER_H

#include "MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMainPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class ReflDataProcessorPresenter

ReflDataProcessorPresenter is a presenter class that inherits from
GenericDataProcessorPresenter and re-implements some methods

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
class ReflDataProcessorPresenter
    : public MantidQt::MantidWidgets::GenericDataProcessorPresenter {
public:
  // Constructor
  ReflDataProcessorPresenter(
      const MantidQt::MantidWidgets::DataProcessorWhiteList &whitelist,
      const std::map<
          std::string,
          MantidQt::MantidWidgets::DataProcessorPreprocessingAlgorithm> &
          preprocessMap,
      const MantidQt::MantidWidgets::DataProcessorProcessingAlgorithm &
          processor,
      const MantidQt::MantidWidgets::DataProcessorPostprocessingAlgorithm &
          postprocessor,
      const std::map<std::string, std::string> &postprocessMap =
          std::map<std::string, std::string>(),
      const std::string &loader = "Load");
  ~ReflDataProcessorPresenter() override;

private:
  // process selected rows
  void process() override;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTER_H*/
