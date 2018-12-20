#ifndef MANTIDQTCUSTOMINTERFACES_EXCITATIONSDIAGRESULTS_H_
#define MANTIDQTCUSTOMINTERFACES_EXCITATIONSDIAGRESULTS_H_

#include "DllOption.h"
#include "MantidQtWidgets/Common/MantidDialog.h"
#include <QGridLayout>
#include <QHash>
#include <QSignalMapper>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON DiagResults : public API::MantidDialog {
  Q_OBJECT

public:
  DiagResults(QWidget *parent);
  void updateResults(const QString &testSummary);

signals:
  /// is emitted just before the window dies to let the window that created this
  /// know the pointer it has is invalid
  void died();

private:
  void updateRow(int row, QString text);
  int addRow(QString firstColumn, QString secondColumn);
  void closeEvent(QCloseEvent *event) override;

private:
  /// the layout that widgets are added to
  QGridLayout *m_Grid;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_EXCITATIONSDIAGRESULTS_H_
