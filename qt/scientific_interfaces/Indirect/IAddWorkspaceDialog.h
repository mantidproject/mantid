#ifndef MANTIDQTCUSTOMINTERFACES_IADDWORKSPACEDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_IADDWORKSPACEDIALOG_H_

#include <QDialog>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IAddWorkspaceDialog : public QDialog {
public:
  IAddWorkspaceDialog(QWidget *parent) : QDialog(parent) {}
  virtual std::string workspaceName() const = 0;
  virtual void setWSSuffices(const QStringList &suffices) = 0;
  virtual void setFBSuffices(const QStringList &suffices) = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_IADDWORKSPACEDIALOG_H_ */
