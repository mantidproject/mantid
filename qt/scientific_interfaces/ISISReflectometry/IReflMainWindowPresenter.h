// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWPRESENTER_H

#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflMainWindowPresenter

IReflMainWindowPresenter is the interface defining the functions that the main
window presenter needs to implement. This interface is used by tab presenters to
request information from other tabs.
*/
class IReflMainWindowPresenter {
public:
  /// Destructor
  virtual ~IReflMainWindowPresenter(){};
  enum class Flag { HelpPressed };

  virtual void notify(Flag flag) = 0;
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
  virtual void giveUserCritical(const std::string &prompt,
                                const std::string &title) = 0;
  /// Dialog to print information
  virtual void giveUserInfo(const std::string &prompt,
                            const std::string &title) = 0;
  /// Run a python algorithm
  virtual std::string runPythonAlgorithm(const std::string &pythonCode) = 0;
  /// Set the instrument name
  virtual void setInstrumentName(const std::string &instName) const = 0;
  /// Data processing check for all groups
  virtual bool isProcessing() const = 0;
  /// Data processing check for a specific group
  virtual bool isProcessing(int group) const = 0;

  virtual void settingsChanged(int group) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWPRESENTER_H */
