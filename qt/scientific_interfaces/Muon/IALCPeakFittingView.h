// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_IALCPEAKFITTINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IALCPEAKFITTINGVIEW_H_

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"

#include "DllConfig.h"

#include <QObject>
#include <boost/optional.hpp>

class QwtData;

namespace MantidQt {
namespace CustomInterfaces {

/** IALCPeakFittingView : Interface for ALC Peak Fitting step view.
 */
class MANTIDQT_MUONINTERFACE_DLL IALCPeakFittingView : public QObject {
  Q_OBJECT

public:
  /// @return If index empty - total function, otherwise - function at index
  virtual Mantid::API::IFunction_const_sptr function(QString index) const = 0;

  /// @return Index of the function currently seleted in the Function Browser
  virtual boost::optional<QString> currentFunctionIndex() const = 0;

  /// @return A peak currently represented by the peak picker
  virtual Mantid::API::IPeakFunction_const_sptr peakPicker() const = 0;

public slots:
  /// Performs any necessary initialization
  virtual void initialize() = 0;

  /// Update the data curve displayed
  /// @param data :: New curve data
  /// @param errors :: New curve errors
  virtual void setDataCurve(const QwtData &data,
                            const std::vector<double> &errors) = 0;

  /// Update the fitted curve displayed
  /// @param data :: New curve data
  virtual void setFittedCurve(const QwtData &data) = 0;

  /// Set function displayed in Function Browser
  /// @param newFunction :: New function to display
  virtual void
  setFunction(const Mantid::API::IFunction_const_sptr &newFunction) = 0;

  /// Update a single parameter in Function Browser
  /// @param funcIndex :: Index of the function where to update parameter
  /// @param paramName :: Name of the parameter to udpate
  /// @param value :: New parameter value
  virtual void setParameter(const QString &funcIndex, const QString &paramName,
                            double value) = 0;

  /// Enabled/disable PeakPicker on the plot
  /// @param enabled :: New enabled status
  virtual void setPeakPickerEnabled(bool enabled) = 0;

  /// Resize/move PeakPicker so that it represents specified peak
  /// @param peak :: A new peak to represent
  virtual void
  setPeakPicker(const Mantid::API::IPeakFunction_const_sptr &peak) = 0;

  /**
   * Pops-up an error box
   * @param message :: Error message to display
   */
  virtual void displayError(const QString &message) = 0;

  /// Opens the Mantid Wiki web page
  virtual void help() = 0;

  /// Emits signal
  virtual void plotGuess() = 0;

  /// Changes button state
  virtual void changePlotGuessState(bool plotted) = 0;

signals:
  /// Request to perform peak fitting
  void fitRequested();

  /// Currently selected function in Function Browser has changed
  void currentFunctionChanged();

  /// PeakPicker was resized/moved _by user_ (not thrown if changed
  /// programmatically)
  void peakPickerChanged();

  /// Parameter value is changed in the Function Browser _either by user or
  /// programmatically_
  void parameterChanged(const QString &funcIndex, const QString &paramName);

  /// Request to plot guess
  void plotGuessClicked();
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_IALCPEAKFITTINGVIEW_H_ */
