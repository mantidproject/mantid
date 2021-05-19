// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QDialog>
#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IAddWorkspaceDialog : public QDialog {
  Q_OBJECT
public:
  IAddWorkspaceDialog(QWidget *parent) : QDialog(parent) {}
  virtual std::string workspaceName() const = 0;
  virtual void setWSSuffices(const QStringList &suffices) = 0;
  virtual void setFBSuffices(const QStringList &suffices) = 0;

  virtual void updateSelectedSpectra() = 0;

  void closeEvent(QCloseEvent *) override { emit closeDialog(); }

signals:
  void addData();
  void closeDialog();
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
