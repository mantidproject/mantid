// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IALCPeakFittingViewSubscriber.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"

#include "DllConfig.h"

#include <QObject>
#include <optional>

namespace MantidQt::CustomInterfaces {

/** IALCPeakFittingView : Interface for ALC Peak Fitting step view.
 */
class MANTIDQT_MUONINTERFACE_DLL IALCPeakFittingView : public QObject {
  Q_OBJECT

public:
  /// @return If index empty - total function, otherwise - function at index
  virtual Mantid::API::IFunction_const_sptr function(std::string const &index) const = 0;

  /// @return Index of the function currently seleted in the Function Browser
  virtual std::optional<std::string> currentFunctionIndex() const = 0;

  /// @return A peak currently represented by the peak picker
  virtual Mantid::API::IPeakFunction_const_sptr peakPicker() const = 0;

  virtual void removePlot(std::string const &plotName) = 0;

  /**
   * Pops-up an error box
   * @param message :: Error message to display
   */
  virtual void displayError(const std::string &message) = 0;

  virtual void subscribe(IALCPeakFittingViewSubscriber *subscriber) = 0;

public slots:
  /// Performs any necessary initialization
  virtual void initialize() = 0;

  /// Update the data curve displayed
  /// @param workspace :: The workspace containing the data
  /// @param workspaceIndex :: The index to plot
  virtual void setDataCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) = 0;

  /// Update the fitted curve displayed
  /// @param workspace :: The workspace containing the data
  /// @param workspaceIndex :: The index to plot
  virtual void setFittedCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) = 0;

  /// Update the guess curve displayed
  /// @param workspace :: The workspace containing the data
  /// @param workspaceIndex :: The index to plot
  virtual void setGuessCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) = 0;

  /// Set function displayed in Function Browser
  /// @param newFunction :: New function to display
  virtual void setFunction(const Mantid::API::IFunction_const_sptr &newFunction) = 0;

  /// Update a single parameter in Function Browser
  /// @param funcIndex :: Index of the function where to update parameter
  /// @param paramName :: Name of the parameter to udpate
  /// @param value :: New parameter value
  virtual void setParameter(std::string const &funcIndex, std::string const &paramName, double value) = 0;

  /// Enabled/disable PeakPicker on the plot
  /// @param enabled :: New enabled status
  virtual void setPeakPickerEnabled(bool enabled) = 0;

  /// Resize/move PeakPicker so that it represents specified peak
  /// @param peak :: A new peak to represent
  virtual void setPeakPicker(const Mantid::API::IPeakFunction_const_sptr &peak) = 0;

  /// Opens the Mantid Wiki web page
  virtual void help() = 0;

  /// Calls out to the subscriber
  virtual void plotGuess() = 0;

  /// Changes button state
  virtual void changePlotGuessState(bool plotted) = 0;

  /// Request to perform peak fitting
  virtual void fitRequested() = 0;

  /// Parameter value is changed in the Function Browser _either by user or
  /// programmatically_
  virtual void onParameterChanged(std::string const &, std::string const &) = 0;

signals:
  /// Currently selected function in Function Browser has changed
  void currentFunctionChanged();

  /// PeakPicker was resized/moved _by user_ (not thrown if changed
  /// programmatically)
  void peakPickerChanged();

  /// Parameter value is changed in the Function Browser _either by user or
  /// programmatically_
  void parameterChanged(std::string const &funcIndex, std::string const &paramName);

  /// Request to plot guess
  void plotGuessClicked();
};

} // namespace MantidQt::CustomInterfaces
