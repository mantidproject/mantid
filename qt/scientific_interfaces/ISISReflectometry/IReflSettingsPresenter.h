// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLSETTINGSPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLSETTINGSPRESENTER_H

#include "IReflSettingsTabPresenter.h"
#include "MantidAPI/Algorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowPresenter;

/** @class IReflSettingsPresenter

IReflSettingsPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Settings' presenter
*/
class IReflSettingsPresenter {
public:
  virtual ~IReflSettingsPresenter(){};
  /// Transmission runs for a particular angle
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(const double angle) const = 0;
  /// Whether per-angle transmission runs are set
  virtual bool hasPerAngleOptions() const = 0;
  /// Pre-processing
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getTransmissionOptions() const = 0;
  /// Processing
  virtual Mantid::API::IAlgorithm_sptr createReductionAlg() = 0;
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getReductionOptions() const = 0;
  /// Post-processing
  virtual std::string getStitchOptions() const = 0;
  virtual void acceptTabPresenter(IReflSettingsTabPresenter *tabPresenter) = 0;

  enum Flag {
    ExpDefaultsFlag,
    InstDefaultsFlag,
    SettingsChangedFlag,
    SummationTypeChanged
  };

  /// Tell the presenter something happened
  virtual void notify(IReflSettingsPresenter::Flag flag) = 0;
  /// Set current instrument name
  virtual void setInstrumentName(const std::string &instName) = 0;

  virtual void onReductionPaused() = 0;
  virtual void onReductionResumed() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLSETTINGSPRESENTER_H */
