#ifndef MANTIDQTCUSTOMINTERFACESIDA_APPLYPAALMANPINGS_H_
#define MANTIDQTCUSTOMINTERFACESIDA_APPLYPAALMANPINGS_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_ApplyPaalmanPings.h"
#include "CorrectionsTab.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport ApplyPaalmanPings : public CorrectionsTab {
  Q_OBJECT

public:
  ApplyPaalmanPings(QWidget *parent = 0);

private slots:
  /// Handles the geometry being changed
  void handleGeometryChange(int index);
  /// Handles a new sample being loaded
  void newSample(const QString &dataName);
  /// Handles a new container being loaded
  void newContainer(const QString &dataName);
  /// Updates the container
  void updateContainer();
  /// Updates the preview mini plot
  void plotPreview(int wsIndex);
  /// Handle abs. correction algorithm completion
  void absCorComplete(bool error);
  /// Handle convert units and save algorithm completion
  void postProcessComplete(bool error);
  /// Handles mantid plot and save
  void saveClicked();
  void plotClicked();

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

  void addRebinStep(QString toRebin, QString toMatch);
  void addInterpolationStep(Mantid::API::MatrixWorkspace_sptr toInterpolate,
                            std::string toMatch);

  Ui::ApplyPaalmanPings m_uiForm;

  std::string m_originalSampleUnits;

  std::string m_sampleWorkspaceName;
  std::string m_containerWorkspaceName;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_APPLYPAALMANPINGS_H_ */
