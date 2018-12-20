#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_

#include "CorrectionsTab.h"
#include "ui_ContainerSubtraction.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport ContainerSubtraction : public CorrectionsTab {
  Q_OBJECT

public:
  ContainerSubtraction(QWidget *parent = nullptr);
  ~ContainerSubtraction();

  void setTransformedContainer(Mantid::API::MatrixWorkspace_sptr workspace,
                               const std::string &name);
  void setTransformedContainer(Mantid::API::MatrixWorkspace_sptr workspace);

private slots:
  /// Handles a new sample being loaded
  void newSample(const QString &dataName);
  /// Handles a new container being loaded
  void newContainer(const QString &dataName);
  /// Handles a change in the can scale or shift
  void updateCan();
  /// Updates the preview mini plot
  void plotPreview(int wsIndex);
  /// Handle abs. correction algorithm completion
  void containerSubtractionComplete();
  /// Handles saving workspace
  void saveClicked();
  /// Handles mantid plotting
  void plotClicked();
  /// Handles plotting the preview.
  void plotCurrentPreview();

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

  void plotInPreview(const QString &curveName,
                     Mantid::API::MatrixWorkspace_sptr &ws,
                     const QColor &curveColor);

  std::string createOutputName();

  Mantid::API::MatrixWorkspace_sptr
  requestRebinToSample(Mantid::API::MatrixWorkspace_sptr workspace) const;

  Mantid::API::MatrixWorkspace_sptr
  shiftWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                 double shiftValue) const;
  Mantid::API::MatrixWorkspace_sptr
  scaleWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                 double scaleValue) const;
  Mantid::API::MatrixWorkspace_sptr
  minusWorkspace(Mantid::API::MatrixWorkspace_sptr lhsWorkspace,
                 Mantid::API::MatrixWorkspace_sptr rhsWorkspace) const;
  Mantid::API::MatrixWorkspace_sptr
  rebinToWorkspace(Mantid::API::MatrixWorkspace_sptr workspaceToRebin,
                   Mantid::API::MatrixWorkspace_sptr workspaceToMatch) const;
  Mantid::API::MatrixWorkspace_sptr
  convertToHistogram(Mantid::API::MatrixWorkspace_sptr workspace) const;

  Mantid::API::IAlgorithm_sptr
  shiftAlgorithm(Mantid::API::MatrixWorkspace_sptr workspace,
                 double shiftValue) const;
  Mantid::API::IAlgorithm_sptr
  scaleAlgorithm(Mantid::API::MatrixWorkspace_sptr workspace,
                 double scaleValue) const;
  Mantid::API::IAlgorithm_sptr
  minusAlgorithm(Mantid::API::MatrixWorkspace_sptr lhsWorkspace,
                 Mantid::API::MatrixWorkspace_sptr rhsWorkspace) const;
  Mantid::API::IAlgorithm_sptr rebinToWorkspaceAlgorithm(
      Mantid::API::MatrixWorkspace_sptr workspaceToRebin,
      Mantid::API::MatrixWorkspace_sptr workspaceToMatch) const;
  Mantid::API::IAlgorithm_sptr convertToHistogramAlgorithm(
      Mantid::API::MatrixWorkspace_sptr workspace) const;
  Mantid::API::IAlgorithm_sptr
  addSampleLogAlgorithm(Mantid::API::MatrixWorkspace_sptr workspace,
                        const std::string &name, const std::string &type,
                        const std::string &value) const;

  Ui::ContainerSubtraction m_uiForm;
  std::string m_originalSampleUnits;

  /// Loaded workspaces
  Mantid::API::MatrixWorkspace_sptr m_csSampleWS;
  Mantid::API::MatrixWorkspace_sptr m_csContainerWS;
  Mantid::API::MatrixWorkspace_sptr m_csSubtractedWS;
  Mantid::API::MatrixWorkspace_sptr m_transformedContainerWS;

  size_t m_spectra;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_ */
