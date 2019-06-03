// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGVIEW_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"

#include "ALCDataLoadingPresenter.h"
#include "DllConfig.h"
#include "IALCDataLoadingView.h"

#include "ui_ALCDataLoadingView.h"

#include <qwt_plot_curve.h>

namespace MantidQt {
namespace MantidWidgets {
class ErrorCurve;
class LogValueSelector;
} // namespace MantidWidgets
} // namespace MantidQt

namespace MantidQt {
namespace CustomInterfaces {

/** ALCDataLoadingView : ALC Data Loading view interface implementation using Qt
  widgets
*/

class MANTIDQT_MUONINTERFACE_DLL ALCDataLoadingView
    : public IALCDataLoadingView {
public:
  ALCDataLoadingView(QWidget *widget);
  ~ALCDataLoadingView();

  // -- IALCDataLoadingView interface
  // ------------------------------------------------------------

  void initialize() override;

  std::string firstRun() const override;
  std::string lastRun() const override;
  std::string log() const override;
  std::string function() const override;
  std::string deadTimeType() const override;
  std::string deadTimeFile() const override;
  std::string detectorGroupingType() const override;
  std::string getForwardGrouping() const override;
  std::string getBackwardGrouping() const override;
  std::string redPeriod() const override;
  std::string greenPeriod() const override;
  bool subtractIsChecked() const override;
  std::string calculationType() const override;
  boost::optional<std::pair<double, double>> timeRange() const override;

  void setPlottedData(Mantid::API::MatrixWorkspace_sptr &workspace,
                      std::size_t const &workspaceIndex = 0) override;
  void displayError(const std::string &error) override;
  void setAvailableLogs(const std::vector<std::string> &logs) override;
  void setAvailablePeriods(const std::vector<std::string> &periods) override;
  void setTimeLimits(double tMin, double tMax) override;
  void setTimeRange(double tMin, double tMax) override;
  void help() override;
  void disableAll() override;
  void enableAll() override;
  void checkBoxAutoChanged(int state) override;
  void handleFirstFileChanged() override;

  /// returns the string "Auto"
  std::string autoString() const override { return g_autoString; }

  /// If Auto mode on, store name of currently loaded file
  /// @param file :: [input] name of file loaded
  void setCurrentAutoFile(const std::string &file) override {
    m_currentAutoFile = file;
  }

  // -- End of IALCDataLoadingView interface
  // -----------------------------------------------------

private:
  /// Common function to set available items in a combo box
  void setAvailableItems(QComboBox *comboBox,
                         const std::vector<std::string> &items);

  /// UI form
  Ui::ALCDataLoadingView m_ui;

  /// The widget used
  QWidget *const m_widget;

  /// Loaded data curve
  QwtPlotCurve *m_dataCurve;

  /// Loaded errors
  MantidQt::MantidWidgets::ErrorCurve *m_dataErrorCurve;

  /// the string "Auto"
  static const std::string g_autoString;

  /// If Auto in use, the file last loaded
  std::string m_currentAutoFile;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGVIEW_H_ */
