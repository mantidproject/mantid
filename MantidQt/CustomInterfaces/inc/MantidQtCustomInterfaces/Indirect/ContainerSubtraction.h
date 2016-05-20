#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_

#include "ui_ContainerSubtraction.h"
#include "CorrectionsTab.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport ContainerSubtraction : public CorrectionsTab {
  Q_OBJECT

public:
  ContainerSubtraction(QWidget *parent = 0);

private slots:
  /// Handles a new sample being loaded
  void newSample(const QString &dataName);
  /// Handles a new container being loaded 
  void newContainer(const QString &dataName);
  /// Handles a change in the can scale
  void updateCanScale(double canScale);
  /// Handles a change in the can shift
  void updateCanShift(double canShift);
  /// Updates the preview mini plot
  void plotPreview(int wsIndex);
  /// Handle abs. correction algorithm completion
  void absCorComplete(bool error);
  /// Handle convert units and save algorithm completion
  void postProcessComplete(bool error);

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

  void addRebinStep(QString toRebin, QString toMatch);

  Ui::ContainerSubtraction m_uiForm;
  std::string m_originalSampleUnits;

  std::string m_sampleWorkspaceName;
  std::string m_containerWorkspaceName;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_ */