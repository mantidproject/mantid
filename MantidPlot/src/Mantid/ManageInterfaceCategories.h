// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANAGE_INTERFACE_CATEGORIES_H
#define MANTID_MANAGE_INTERFACE_CATEGORIES_H

#include <QAbstractListModel>
#include <QDialog>
#include <QSet>
#include <QString>

#include "MantidQtWidgets/Common/MantidDialog.h"
#include "ui_ManageInterfaceCategories.h"

class ApplicationWindow;

/**
 * This is a Model class that wraps the available interface categories along
 * with the user's preferences about which categories to display.  It
 * interacts with the View in the ManageInterfaceCategories dialog, and
 * persists its data using the user preferences file via the ConfigService.

 * See the Qt documentation for more information on how this class fits in to
 * their implementation of Model-View programming.
 */
class InterfaceCategoryModel : public QAbstractListModel {
  Q_OBJECT

public:
  /// Constructor.
  explicit InterfaceCategoryModel(
      const QSet<QString> &allCategories = QSet<QString>());

  /// Required overloaded methods for an editable Qt data model:

  /// The total number of categories.
  int rowCount(const QModelIndex &parent) const override;
  /// Returns the information to be displayed in the column header.
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  /// Returns the data used to fill each item in the model.
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  /// Used to set the data for each item in the model.
  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override;
  /// Flagging up the fact that this model contains checkable items.
  Qt::ItemFlags flags(const QModelIndex &index) const override;

public slots:
  /// Persist this model's data to the user preferences file.
  void saveHiddenCategories();

private:
  /// Load this model's data from the user preferences file.
  void loadHiddenCategories();

  /// The set of all categories to be hidden.
  QSet<QString> m_hiddenCategories;
  /// A list of all categories.  (Effectively a set, but implemented as a list
  /// so it can be sorted.
  QList<QString> m_allCategories;
};

/**
 * This class handles the "Manage Interface Categories" dialog in MantidPlot, in
 * which users can add or remove the various interface categories listed in the
 * "Interface" menu.
 */
class ManageInterfaceCategories : public MantidQt::API::MantidDialog {
  Q_OBJECT

public:
  /// Only constructor.  We insist on seeing the ApplicationWindow.
  explicit ManageInterfaceCategories(ApplicationWindow *parent);

private slots:
  /// Slot to open the help web page.
  void helpClicked();

private:
  /// Disallow default constructor.
  ManageInterfaceCategories();

  /// Set up the dialog.
  void initLayout();

  /// The widget form.
  Ui::ManageInterfaceCategories m_uiForm;
  /// The model used by the View of this dialog.
  InterfaceCategoryModel m_model;
};

#endif /* MANTID_MANAGE_INTERFACE_CATEGORIES_H */
