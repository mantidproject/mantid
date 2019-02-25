// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
  /// Handles plotting the preview.
  void plotCurrentPreview();

  void saveClicked();
  void plotSpectrumClicked();
  void plotContourClicked();
  void runClicked();

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;
  void setFileExtensionsByName(bool filter) override;

  void plotInPreview(const QString &curveName,
                     Mantid::API::MatrixWorkspace_sptr &ws,
                     const QColor &curveColor);

  std::size_t getOutWsNumberOfSpectra() const;
  Mantid::API::MatrixWorkspace_const_sptr
  getADSWorkspace(std::string const &name) const;

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

  void setPlotSpectrumIndexMax(int maximum);
  int getPlotSpectrumIndex();

  void setRunEnabled(bool enabled);
  void setPlotSpectrumEnabled(bool enabled);
  void setPlotContourEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setRunIsRunning(bool running);
  void setPlotSpectrumIsPlotting(bool plotting);
  void setPlotContourIsPlotting(bool plotting);

  QStringList m_sampleFBExtensions;
  QStringList m_sampleWSExtensions;
  QStringList m_containerFBExtensions;
  QStringList m_containerWSExtensions;

  Ui::ContainerSubtraction m_uiForm;
  std::string m_originalSampleUnits;

  /// Loaded workspaces
  Mantid::API::MatrixWorkspace_sptr m_csSampleWS;
  Mantid::API::MatrixWorkspace_sptr m_csContainerWS;
  Mantid::API::MatrixWorkspace_sptr m_csSubtractedWS;
  Mantid::API::MatrixWorkspace_sptr m_transformedContainerWS;

  std::size_t m_spectra;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_ */
