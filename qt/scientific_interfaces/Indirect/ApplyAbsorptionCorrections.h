// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_APPLYABSORPTIONCORRECTIONS_H_
#define MANTIDQTCUSTOMINTERFACESIDA_APPLYABSORPTIONCORRECTIONS_H_

#include "CorrectionsTab.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_ApplyAbsorptionCorrections.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport ApplyAbsorptionCorrections : public CorrectionsTab {
  Q_OBJECT

public:
  ApplyAbsorptionCorrections(QWidget *parent = nullptr);
  ~ApplyAbsorptionCorrections();

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
  void plotCurrentPreview();

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

  void addInterpolationStep(Mantid::API::MatrixWorkspace_sptr toInterpolate,
                            std::string toMatch);
  void plotInPreview(const QString &curveName,
                     Mantid::API::MatrixWorkspace_sptr &ws,
                     const QColor &curveColor);

  Ui::ApplyAbsorptionCorrections m_uiForm;

  std::string m_originalSampleUnits;

  std::string m_sampleWorkspaceName;
  std::string m_containerWorkspaceName;
  /// Loaded workspaces
  Mantid::API::MatrixWorkspace_sptr m_ppSampleWS;
  Mantid::API::MatrixWorkspace_sptr m_ppContainerWS;

  size_t m_spectra;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_APPLYABSORPTIONCORRECTIONS_H_ */
