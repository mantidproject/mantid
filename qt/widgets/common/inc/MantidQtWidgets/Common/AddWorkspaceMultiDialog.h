// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/IAddWorkspaceDialog.h"
#include "QtJobRunner.h"
#include "ui_AddWorkspaceMultiDialog.h"

#include <vector>

#include <QDialog>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON AddWorkspaceMultiDialog : public QDialog,
                                                           public IAddWorkspaceDialog,
                                                           public API::JobRunnerSubscriber {
  Q_OBJECT
public:
  explicit AddWorkspaceMultiDialog(QWidget *parent);

  std::string workspaceName() const override;
  stringPairVec selectedNameIndexPairs() const;

  bool isEmpty() const;
  void setWSSuffices(const QStringList &suffices) override;
  void setFBSuffices(const QStringList &suffices) override;
  void setup();

  void notifyBatchComplete(bool error) override;
  void notifyBatchCancelled() override{};
  void notifyAlgorithmStarted(API::IConfiguredAlgorithm_sptr &algorithm) override { UNUSED_ARG(algorithm); };
  void notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr &algorithm) override { UNUSED_ARG(algorithm); };
  void notifyAlgorithmError(API::IConfiguredAlgorithm_sptr &algorithm, std::string const &message) override;

signals:
  void addData(MantidWidgets::IAddWorkspaceDialog *dialog);

private slots:
  void updateSelectedSpectra() override;
  void selectAllSpectra();
  void unifyRange();
  void handleFilesFound();
  void emitAddData();

private:
  void updateAddButtonState(bool enabled) const;
  Ui::AddWorkspaceMultiDialog m_uiForm;
  /// Algorithm Runner used to run the load algorithm
  std::unique_ptr<MantidQt::API::QtJobRunner> m_algRunner;
};

} // namespace MantidWidgets
} // namespace MantidQt
