#ifndef MANTID_CUSTOMINTERFACES_QTREFLSETTINGSVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLSETTINGSVIEW_H_

#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsView.h"
#include "ui_ReflSettingsWidget.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflSettingsPresenter;

/** QtReflSettingsView : Provides an interface for the "Settings" widget in the
ISIS Reflectometry interface.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class QtReflSettingsView : public QWidget, public IReflSettingsView {
  Q_OBJECT
public:
  /// Constructor
  QtReflSettingsView(QWidget *parent = 0);
  /// Destructor
  ~QtReflSettingsView() override;
  /// Returns the presenter managing this view
  IReflSettingsPresenter *getPresenter() const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions() const override;
  /// Return selected analysis mode
  std::string getAnalysisMode() const override;
  /// Return direct beam
  std::string getDirectBeam() const override;
  /// Return transmission runs
  std::string getTransmissionRuns() const override;
  /// Return start overlap for transmission runs
  std::string getStartOverlap() const override;
  /// Return end overlap for transmission runs
  std::string getEndOverlap() const override;
  /// Return selected polarisation corrections
  std::string getPolarisationCorrections() const override;
  /// Return CRho
  std::string getCRho() const override;
  /// Return CAlpha
  std::string getCAlpha() const override;
  /// Return CAp
  std::string getCAp() const override;
  /// Return Cpp
  std::string getCPp() const override;
  /// Return momentum transfer limits
  std::string getMomentumTransferStep() const override;
  /// Return scale factor
  std::string getScaleFactor() const override;
  /// Return integrated monitors option
  std::string getIntMonCheck() const override;
  /// Return monitor integral wavelength min
  std::string getMonitorIntegralMin() const override;
  /// Return monitor integral wavelength max
  std::string getMonitorIntegralMax() const override;
  /// Return monitor background wavelength min
  std::string getMonitorBackgroundMin() const override;
  /// Return monitor background wavelength max
  std::string getMonitorBackgroundMax() const override;
  /// Return wavelength min
  std::string getLambdaMin() const override;
  /// Return wavelength max
  std::string getLambdaMax() const override;
  /// Return I0MonitorIndex
  std::string getI0MonitorIndex() const override;
  /// Return processing instructions
  std::string getProcessingInstructions() const override;
  /// Return selected detector correction type
  std::string getDetectorCorrectionType() const override;
  /// Set the status of whether polarisation corrections should be enabled
  void setIsPolCorrEnabled(bool enable) const override;
  /// Set default values for experiment and instrument settings
  void setExpDefaults(const std::vector<std::string> &) const override;
  void setInstDefaults(const std::vector<double> &,
                       const std::vector<std::string> &) const override;
  /// Check if experiment settings are enabled
  bool experimentSettingsEnabled() const override;
  /// Check if instrument settings are enabled
  bool instrumentSettingsEnabled() const override;
  /// Creates hints for 'Stitch1DMany'
  void
  createStitchHints(const std::map<std::string, std::string> &hints) override;

public slots:
  /// Request presenter to obtain default values for settings
  void requestExpDefaults() const;
  void requestInstDefaults() const;
  /// Sets enabled status for polarisation corrections and parameters
  void setPolarisationOptionsEnabled(bool enable) const override;

private:
  /// Initialise the interface
  void initLayout();

  /// The widget
  Ui::ReflSettingsWidget m_ui;
  /// The presenter
  std::unique_ptr<IReflSettingsPresenter> m_presenter;
  /// Whether or not polarisation corrections should be enabled
  mutable bool m_isPolCorrEnabled;
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QTREFLSETTINGSVIEW_H_ */
