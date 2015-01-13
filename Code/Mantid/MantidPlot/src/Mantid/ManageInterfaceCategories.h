#ifndef MANTID_MANAGE_INTERFACE_CATEGORIES_H
#define MANTID_MANAGE_INTERFACE_CATEGORIES_H

#include <QAbstractListModel>
#include <QDialog>
#include <QSet>
#include <QString>

#include "ui_ManageInterfaceCategories.h"

class ApplicationWindow;

/**
 * Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source
 * 
 * This file is part of Mantid.
 * 
 * Mantid is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Mantid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * File change history is stored at: <https://github.com/mantidproject/mantid>
 * Code Documentation is available at: <http://doxygen.mantidproject.org>    
 */

/**
 * This is a Model class that wraps the available interface categories along
 * with the user's preferences about which categories to display.  It
 * interacts with the View in the ManageInterfaceCategories dialog, and
 * persists its data using the user preferences file via the ConfigService.
 
 * See the Qt documentation for more information on how this class fits in to
 * their implementation of Model-View programming.
 */
class InterfaceCategoryModel : public QAbstractListModel
{
	Q_OBJECT

public:
  /// Constructor.
  InterfaceCategoryModel(const QSet<QString> & allCategories = QSet<QString>());

  /// Required overloaded methods for an editable Qt data model:

  /// The total number of categories.
  virtual int rowCount(const QModelIndex & parent) const;
  /// Returns the information to be displayed in the column header.
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  /// Returns the data used to fill each item in the model.
  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  /// Used to set the data for each item in the model.
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
  /// Flagging up the fact that this model contains checkable items.
  virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;

public slots:
  /// Persist this model's data to the user preferences file.
  void saveHiddenCategories();

private:
  /// Load this model's data from the user preferences file.
  void loadHiddenCategories();

  /// The set of all categories to be hidden.
  QSet<QString> m_hiddenCategories;
  /// A list of all categories.  (Effectively a set, but implemented as a list so it can be sorted.
  QList<QString> m_allCategories;
};

/**
 * This class handles the "Manage Interface Categories" dialog in MantidPlot, in 
 * which users can add or remove the various interface categories listed in the
 * "Interface" menu.
 */
class ManageInterfaceCategories : public QDialog
{
	Q_OBJECT

public:
	/// Only constructor.  We insist on seeing the ApplicationWindow.
  ManageInterfaceCategories(ApplicationWindow * parent);

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
