#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTMOLDYN_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTMOLDYN_H_

#include "ui_IndirectMolDyn.h"
#include "IndirectSimulationTab.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport IndirectMolDyn : public IndirectSimulationTab {
  Q_OBJECT

public:
  IndirectMolDyn(QWidget *parent = 0);

  // Inherited methods from IndirectTab
  void setup() override;
  bool validate() override;
  void run() override;

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

private slots:
  void versionSelected(const QString &);
  // Handle plotting and saving
  void plotClicked();
  void saveClicked();

private:
  // The ui form
  Ui::IndirectMolDyn m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
