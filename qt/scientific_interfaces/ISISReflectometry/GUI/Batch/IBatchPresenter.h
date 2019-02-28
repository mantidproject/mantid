// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IBATCHPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IBATCHPRESENTER_H

#include "MantidGeometry/Instrument_fwd.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IBatchPresenter

IBatchPresenter is the interface defining the functions that the main
window presenter needs to implement. This interface is used by tab presenters to
request information from other tabs.
*/
class IBatchPresenter {
public:
  /// Destructor
  virtual ~IBatchPresenter() = default;

  virtual void notifyReductionPaused() = 0;
  virtual void notifyReductionResumed() = 0;
  virtual void notifyAutoreductionResumed() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual void notifyAutoreductionCompleted() = 0;
  virtual void notifyInstrumentChanged(const std::string &instName) = 0;
  virtual void notifySettingsChanged() = 0;

  /// Transmission runs for a specific run angle
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(const double angle) const = 0;
  /// Whether there are per-angle transmission runs specified
  virtual bool hasPerAngleOptions() const = 0;
  /// Data processing check for all groups
  virtual bool isProcessing() const = 0;
  virtual bool isAutoreducing() const = 0;
  virtual bool requestClose() const = 0;

  virtual Mantid::Geometry::Instrument_const_sptr instrument() const = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IBATCHPRESENTER_H */
