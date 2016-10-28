#ifndef MANTID_CUSTOMINTERFACES_QTREFLSETTINGSTABVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLSETTINGSTABVIEW_H_

#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabView.h"
#include "ui_ReflSettingsTabWidget.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflSettingsTabPresenter;

/** QtReflSettingsTabView : Provides an interface for the "Settings" tab in the
Reflectometry (Polref) interface.

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
class QtReflSettingsTabView : public QWidget, public IReflSettingsTabView {
  Q_OBJECT
public:
  /// Constructor
  QtReflSettingsTabView(QWidget *parent = 0);
  /// Destructor
  ~QtReflSettingsTabView() override;
  /// Returns the presenter managing this view
  IReflSettingsTabPresenter *getPresenter() const override;
  /// Returns global options for 'Plus' algorithm
  std::string getPlusOptions() const override;
  /// Returns global options for 'CreateTransmissionWorkspaceAuto'
  std::string getTransmissionOptions() const override;
  /// Returns global options for 'ReflectometryReductionOneAuto'
  std::string getReductionOptions() const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions() const override;
  /// Return selected analysis mode
  std::string getAnalysisMode() const override;
  /// Return direct beam
  std::string getDirectBeam() const override;
  /// Return CRho
  std::string getCRho() const override;
  /// Return CAlpha
  std::string getCAlpha() const override;
  /// Return CAp
  std::string getCAp() const override;
  /// Return Cpp
  std::string getCPp() const override;
  /// Return momentum transfer limits
  std::string getMomentumTransferLimits() const override;
  /// Return detector limits
  std::string getDetectorLimits() const override;
  /// Return scale factor
  std::string getScaleFactor() const override;
  /// Return selected polarisation corrections
  std::string getPolarisationCorrections() const override;
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
  /// Set default values for experiment and instrument settings
  void setExpDefaults(std::vector<std::string> defaults) const override;
  void setInstDefaults(std::vector<double> defaults) const override;

  /// Creates hints for 'Plus'
  void
  createPlusHints(const std::map<std::string, std::string> &hints) override;
  /// Creates hints for 'CreateTransmissionWorkspaceAuto'
  void createTransmissionHints(
      const std::map<std::string, std::string> &hints) override;
  /// Creates hints for 'ReflectometryReductionOneAuto'
  void createReductionHints(
      const std::map<std::string, std::string> &hints) override;
  /// Creates hints for 'Stitch1DMany'
  void
  createStitchHints(const std::map<std::string, std::string> &hints) override;

public slots:
  /// Request presenter to obtain default values for settings
  void requestExpDefaults() const override;
  void requestInstDefaults() const override;

private:
  /// Initialise the interface
  void initLayout();

  /// The widget
  Ui::ReflSettingsTabWidget m_ui;
  /// The presenter
  std::unique_ptr<IReflSettingsTabPresenter> m_presenter;
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QTREFLSETTINGSTABVIEW_H_ */
