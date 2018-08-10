#ifndef UCorrectionDIALOG_H
#define REMOVEERRORSDIALOG_H

#include <QDialog>
#include <QPointF>

namespace Ui {
class UCorrectionDialog;
}

namespace MantidQt {
namespace MantidWidgets {

class UCorrectionDialog : public QDialog {
  Q_OBJECT
public:
  UCorrectionDialog(QWidget *parent, QPointF oldValue, bool isManual);
  ~UCorrectionDialog() override;

  bool applyCorrection() const;
  QPointF getValue() const;

private:
  Ui::UCorrectionDialog *ui;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // REMOVEERRORSDIALOG_H
