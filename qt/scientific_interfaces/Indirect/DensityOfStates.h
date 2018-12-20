#ifndef MANTIDQTCUSTOMINTERFACES_DENSITYOFSTATES_H_
#define MANTIDQTCUSTOMINTERFACES_DENSITYOFSTATES_H_

#include "IndirectSimulationTab.h"
#include "ui_DensityOfStates.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport DensityOfStates : public IndirectSimulationTab {
  Q_OBJECT

public:
  DensityOfStates(QWidget *parent = nullptr);

  QString help() { return "DensityOfStates"; };

  void setup() override;
  bool validate() override;
  void run() override;

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

private slots:
  void dosAlgoComplete(bool error);
  void handleFileChange();
  void ionLoadComplete(bool error);
  /// Handle plotting and saving
  void plotClicked();
  void saveClicked();

private:
  /// The ui form
  Ui::DensityOfStates m_uiForm;
  /// Name of output workspace
  QString m_outputWsName;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_DENSITYOFSTATES_H_
