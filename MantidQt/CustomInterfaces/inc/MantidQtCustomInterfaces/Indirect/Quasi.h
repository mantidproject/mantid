#ifndef MANTIDQTCUSTOMINTERFACES_QUASI_H_
#define MANTIDQTCUSTOMINTERFACES_QUASI_H_

#include "ui_Quasi.h"
#include "IndirectBayesTab.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport Quasi : public IndirectBayesTab {
  Q_OBJECT

public:
  Quasi(QWidget *parent = 0);

  // Inherited methods from IndirectBayesTab
  void setup() override;
  bool validate() override;
  void run() override;
  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

private slots:
  /// Slot for when the min range on the range selector changes
  void minValueChanged(double min);
  /// Slot for when the min range on the range selector changes
  void maxValueChanged(double max);
  /// Slot to update the guides when the range properties change
  void updateProperties(QtProperty *prop, double val) override;
  /// Slot to handle when a new sample file is available
  void handleSampleInputReady(const QString &filename);
  /// Slot to handle when a new resolution file is available
  void handleResolutionInputReady(const QString &wsName);
  /// slot to handle when the user changes the program to be used
  void handleProgramChange(int index);
  /// Slot to handle setting a new preview spectrum
  void previewSpecChanged(int value);
  /// Handles updating spectra in mini plot
  void updateMiniPlot();
  /// Handles what happen after the algorithm is run
  void algorithmComplete(bool error);
  // Handles saving of workspace
  void saveClicked();
  // Handles plotting
  void plotClicked();
  // Handles plotting current preview
  void plotCurrentPreview();

private:
  /// Current preview spectrum
  int m_previewSpec;
  /// The ui form
  Ui::Quasi m_uiForm;
  /// alg
  Mantid::API::IAlgorithm_sptr m_QuasiAlg;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
