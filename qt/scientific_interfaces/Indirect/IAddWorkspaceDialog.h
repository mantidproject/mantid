// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_IADDWORKSPACEDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_IADDWORKSPACEDIALOG_H_

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

signals:
  void addData();
  void closeDialog();
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_IADDWORKSPACEDIALOG_H_ */
