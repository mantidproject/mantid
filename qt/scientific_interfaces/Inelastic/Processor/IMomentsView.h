// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include <QStringList>
#include <memory>
#include <string>
#include <tuple>

namespace MantidQt {
namespace MantidWidgets {
class DataSelector;
}
namespace CustomInterfaces {

class IRunView;
class IOutputPlotOptionsView;
class IMomentsPresenter;

class MANTIDQT_INELASTIC_DLL IMomentsView {

public:
  virtual ~IMomentsView() = default;
  virtual void subscribePresenter(IMomentsPresenter *presenter) = 0;
  virtual void setupProperties() = 0;

  virtual IRunView *getRunView() const = 0;
  virtual IOutputPlotOptionsView *getPlotOptions() const = 0;
  virtual MantidWidgets::DataSelector *getDataSelector() const = 0;

  virtual std::string getDataName() const = 0;
  virtual void showMessageBox(std::string const &message) const = 0;

  virtual void setFBSuffixes(QStringList const &suffix) = 0;
  virtual void setWSSuffixes(QStringList const &suffix) = 0;

  /// Function to set the range limits of the plot
  virtual void setPlotPropertyRange(const QPair<double, double> &bounds) = 0;
  /// Function to set the range selector on the mini plot
  virtual void setRangeSelector(const QPair<double, double> &bounds) = 0;
  /// Sets the min of the range selector if it is less than the max
  virtual void setRangeSelectorMin(double newValue) = 0;
  /// Sets the max of the range selector if it is more than the min
  virtual void setRangeSelectorMax(double newValue) = 0;
  virtual void setSaveResultEnabled(bool enable) = 0;

  virtual void plotNewData(std::string const &filename) = 0;
  virtual void replot() = 0;
  virtual void plotOutput(std::string const &outputWorkspace) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt