#ifndef PEAKSTABLECOLUMNSDIALOG_H
#define PEAKSTABLECOLUMNSDIALOG_H

#include <QDialog>
#include <set>

namespace Ui {
class PeaksTableColumnsDialog;
}

namespace MantidQt
{
namespace SliceViewer
{

class PeaksTableColumnsDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit PeaksTableColumnsDialog(QWidget *parent = 0);
  ~PeaksTableColumnsDialog();

  void setVisibleColumns(std::set<QString> & cols);
  std::set<QString> getVisibleColumns();
  
private:
  Ui::PeaksTableColumnsDialog *ui;
  std::set<QString> m_origVisible;
};

} // namespace SliceViewer
} // namespace MantidQt

#endif // PEAKSTABLECOLUMNSDIALOG_H
