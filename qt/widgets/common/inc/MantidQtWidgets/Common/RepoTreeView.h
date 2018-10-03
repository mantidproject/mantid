// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_REPOTREEVIEW_H_
#define MANTID_API_REPOTREEVIEW_H_

#include "DllOption.h"
#include <QTreeView>

namespace MantidQt {
namespace API {
/** RepoTreeView : A specialization of QTreeView class that emits signal every
  time
    the selection change. It extends the currentChanged method in order to add
  the
    emition of the signal currentCell.
*/
class EXPORT_OPT_MANTIDQT_COMMON RepoTreeView : public QTreeView {
  Q_OBJECT

public:
  // constuctor
  RepoTreeView(QWidget *parent = nullptr) : QTreeView(parent){};
  // destructor - not virtual, because this is not intended to be base
  ~RepoTreeView() override{};

signals:
  void currentCell(const QModelIndex &);

protected slots:

  void currentChanged(const QModelIndex &current,
                      const QModelIndex &previous) override {
    QTreeView::currentChanged(current, previous);
    emit currentCell(current);
  };
};

} // namespace API
} // namespace MantidQt

#endif /* MANTID_API_REPOTREEVIEW_H_ */
