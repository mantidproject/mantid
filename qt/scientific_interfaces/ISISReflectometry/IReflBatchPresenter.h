#ifndef MANTID_ISISREFLECTOMETRY_IREFLBATCHPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLBATCHPRESENTER_H

#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflBatchPresenter

IReflBatchPresenter is the interface defining the functions that the main
window presenter needs to implement. This interface is used by tab presenters to
request information from other tabs.

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
class IReflBatchPresenter {
public:
  /// Destructor
  virtual ~IReflBatchPresenter(){};

  virtual void notifyReductionPaused(int group) = 0;
  virtual void notifyReductionResumed(int group) = 0;

  virtual void completedRowReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) = 0;

  virtual void completedGroupReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) = 0;

  /// Transmission runs for a specific run angle
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(int group, const double angle) const = 0;
  /// Whether there are per-angle transmission runs specified
  virtual bool hasPerAngleOptions(int group) const = 0;
  /// Pre-processing
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getTransmissionOptions(int group) const = 0;
  /// Processing
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getReductionOptions(int group) const = 0;
  /// Post-processing
  virtual std::string getStitchOptions(int group) const = 0;
  /// Time-slicing values
  virtual std::string getTimeSlicingValues(int group) const = 0;
  /// Time-slicing type
  virtual std::string getTimeSlicingType(int group) const = 0;
  /// Set the instrument name
  virtual void setInstrumentName(const std::string &instName) const = 0;
  /// Data processing check for all groups
  virtual bool isProcessing() const = 0;
  /// Data processing check for a specific group
  virtual bool isProcessing(int group) const = 0;

  virtual bool requestClose() const = 0;

  virtual void settingsChanged() = 0;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_IREFLBATCHPRESENTER_H */
