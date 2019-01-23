// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLSETTINGSTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLSETTINGSTABPRESENTER_H

#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowPresenter;

/** @class IReflSettingsTabPresenter

IReflSettingsTabPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Settings' tab presenter
*/
class IReflSettingsTabPresenter {
public:
  virtual ~IReflSettingsTabPresenter(){};
  /// Transmission runs for a particular run angle
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(int group, const double angle) const = 0;
  /// Whether per-angle transmission runs are specified
  virtual bool hasPerAngleOptions(int group) const = 0;
  /// Pre-processing
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getTransmissionOptions(int group) const = 0;
  /// Processing
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getReductionOptions(int group) const = 0;
  /// Post-processing
  virtual std::string getStitchOptions(int group) const = 0;
  /// Set current instrument name
  virtual void setInstrumentName(const std::string &instName) = 0;
  virtual void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) = 0;
  virtual void settingsChanged(int group) = 0;
  virtual void onReductionPaused(int group) = 0;
  virtual void onReductionResumed(int group) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLSETTINGSTABPRESENTER_H */
