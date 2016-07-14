#ifndef MANTIDQTCUSTOMINTERFACES_STRETCH_H_
#define MANTIDQTCUSTOMINTERFACES_STRETCH_H_

#include "ui_Stretch.h"
#include "IndirectBayesTab.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport Stretch : public IndirectBayesTab {
  Q_OBJECT

public:
  Stretch(QWidget *parent = 0);

  // Inherited methods from IndirectBayesTab
  void setup() override;
  bool validate() override;
  void run() override;
  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;
  /// Save the workspaces produces from the algorithm
  void saveWorkspaces(const QString &fitWorkspace,
                      const QString &contourWorkspace);
  /// Plot the workspaces specified by the interface
  void plotWorkspaces(const QString &fitWorkspace,
                      const QString &contourWorkspace);

private slots:
  /// Slot for when the min range on the range selector changes
  void minValueChanged(double min);
  /// Slot for when the min range on the range selector changes
  void maxValueChanged(double max);
  /// Slot to update the guides when the range properties change
  void updateProperties(QtProperty *prop, double val) override;
  /// Slot to handle when a new sample file is available
  void handleSampleInputReady(const QString &filename);
  /// Handle plotting and saving after algorithm completion
  void algorithmComplete(const bool &error);

private:
  // The ui form
  Ui::Stretch m_uiForm;
  // Output Names
  std::string m_fitWorkspaceName;
  std::string m_contourWorkspaceName;
  // state of plot and save when algorithm is run
  std::string m_plotType;
  bool m_save;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
