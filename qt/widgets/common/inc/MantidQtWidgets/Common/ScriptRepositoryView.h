// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/ScriptRepository.h"
#include "MantidQtWidgets/Common/MantidDialog.h"
#include "ui_ScriptRepositoryView.h"
#include <QDialog>
#include <QStyledItemDelegate>

namespace MantidQt {
namespace API {
class RepoModel;
/** ScriptRepositoryView : Provide the User Interface to the ScriptRepository.
  It does so through the Mantid Model View Framework. It is composed by a
  specialized QTreeView (RepoTreeView) and a TextBrowser. The TreeView is
  populated with the RepoModel, wich wrappers the ScriptRepository. Inside this
  class, there are two nested classes that will implement delegates to the
  columns of Status and AutoUpdate in order to improve the User Experience.
*/
class EXPORT_OPT_MANTIDQT_COMMON ScriptRepositoryView : public MantidDialog {
  Q_OBJECT

  /// Delegate to show the icons Download and Upload
  class RepoDelegate : public QStyledItemDelegate {
  public:
    RepoDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QIcon getIcon(const QString &state) const;
  };
  /// Delegate to show the checkbox for configuring the auto update
  class CheckBoxDelegate : public QStyledItemDelegate {
  public:
    CheckBoxDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;
  };
  /// Delegate to show the icon to remove the entry from the local and central
  /// repository
  class RemoveEntryDelegate : public QStyledItemDelegate {
  public:
    RemoveEntryDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;
  };

public:
  // constuctor
  ScriptRepositoryView(QWidget *parent = nullptr);
  // destructor - not virtual, because this is not intended to be base
  ~ScriptRepositoryView() override;

signals:
  // allow Mantid to open a python file to be seen
  void loadScript(const QString /*_t1*/);

protected slots:
  // allow to interact with the cells, in order to update the description of the
  // files
  bool chooseLocationAndInstall(Mantid::API::ScriptRepository_sptr repo_ptr);
  void cell_activated(const QModelIndex & /*in*/);
  void updateModel();
  void currentChanged(const QModelIndex &current);
  void helpClicked();
  void openFolderLink(const QString & /*link*/);

private:
  Ui::ScriptRepositoryView *ui;
  RepoModel *model;
};

} // namespace API
} // namespace MantidQt
