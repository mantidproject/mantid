// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PEAKSTABLECOLUMNSDIALOG_H
#define PEAKSTABLECOLUMNSDIALOG_H

#include <QDialog>
#include <set>

namespace Ui {
class PeaksTableColumnsDialog;
}

namespace MantidQt {
namespace SliceViewer {

class PeaksTableColumnsDialog : public QDialog {
  Q_OBJECT

public:
  explicit PeaksTableColumnsDialog(QWidget *parent = nullptr);
  ~PeaksTableColumnsDialog() override;

  void setVisibleColumns(const std::set<QString> &cols);
  std::set<QString> getVisibleColumns();

private:
  Ui::PeaksTableColumnsDialog *ui;
  std::set<QString> m_origVisible;
};

} // namespace SliceViewer
} // namespace MantidQt

#endif // PEAKSTABLECOLUMNSDIALOG_H
