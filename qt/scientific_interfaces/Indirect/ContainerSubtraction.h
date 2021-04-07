// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "CorrectionsTab.h"
#include "ui_ContainerSubtraction.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport ContainerSubtraction : public CorrectionsTab {
  Q_OBJECT

public:
  ContainerSubtraction(QWidget *parent = nullptr);
  ~ContainerSubtraction();

  void setTransformedContainer(Mantid::API::MatrixWorkspace_sptr workspace, const std::string &name);
  void setTransformedContainer(const Mantid::API::MatrixWorkspace_sptr &workspace);

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
  void runClicked();

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;
  void setFileExtensionsByName(bool filter) override;

  void plotInPreview(const QString &curveName, Mantid::API::MatrixWorkspace_sptr &ws, const QColor &curveColor);

  std::string createOutputName();
  void removeOutput();

  Mantid::API::MatrixWorkspace_sptr requestRebinToSample(Mantid::API::MatrixWorkspace_sptr workspace) const;

  Mantid::API::MatrixWorkspace_sptr shiftWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace,
                                                   double shiftValue) const;
  Mantid::API::MatrixWorkspace_sptr scaleWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace,
                                                   double scaleValue) const;
  Mantid::API::MatrixWorkspace_sptr minusWorkspace(const Mantid::API::MatrixWorkspace_sptr &lhsWorkspace,
                                                   const Mantid::API::MatrixWorkspace_sptr &rhsWorkspace) const;
  Mantid::API::MatrixWorkspace_sptr rebinToWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspaceToRebin,
                                                     const Mantid::API::MatrixWorkspace_sptr &workspaceToMatch) const;
  Mantid::API::MatrixWorkspace_sptr convertToHistogram(const Mantid::API::MatrixWorkspace_sptr &workspace) const;

  Mantid::API::IAlgorithm_sptr shiftAlgorithm(const Mantid::API::MatrixWorkspace_sptr &workspace,
                                              double shiftValue) const;
  Mantid::API::IAlgorithm_sptr scaleAlgorithm(const Mantid::API::MatrixWorkspace_sptr &workspace,
                                              double scaleValue) const;
  Mantid::API::IAlgorithm_sptr minusAlgorithm(const Mantid::API::MatrixWorkspace_sptr &lhsWorkspace,
                                              const Mantid::API::MatrixWorkspace_sptr &rhsWorkspace) const;
  Mantid::API::IAlgorithm_sptr
  rebinToWorkspaceAlgorithm(const Mantid::API::MatrixWorkspace_sptr &workspaceToRebin,
                            const Mantid::API::MatrixWorkspace_sptr &workspaceToMatch) const;
  Mantid::API::IAlgorithm_sptr convertToHistogramAlgorithm(const Mantid::API::MatrixWorkspace_sptr &workspace) const;
  Mantid::API::IAlgorithm_sptr addSampleLogAlgorithm(const Mantid::API::MatrixWorkspace_sptr &workspace,
                                                     const std::string &name, const std::string &type,
                                                     const std::string &value) const;

  void setRunEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setRunIsRunning(bool running);

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
