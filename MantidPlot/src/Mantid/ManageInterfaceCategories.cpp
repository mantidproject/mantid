#include "ManageInterfaceCategories.h"
#include "../ApplicationWindow.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"

#include <QtAlgorithms>
#include <QtGui>

using MantidQt::API::MantidDesktopServices;

/////////////////////////////////////////////////////
// InterfaceCategoryModel
/////////////////////////////////////////////////////

/**
 * Constructor.
 *
 * @param allCategories :: the set of all categories to be used in the model.
 */
InterfaceCategoryModel::InterfaceCategoryModel(
    const QSet<QString> &allCategories)
    : QAbstractListModel(), m_allCategories(allCategories.toList()) {
  qSort(m_allCategories);
  loadHiddenCategories();
}

/**
 * The total number of categories.  Overrides virtual method of parent class.
 *
 * @param parent :: unused.
 */
int InterfaceCategoryModel::rowCount(const QModelIndex &parent) const {
  UNUSED_ARG(parent);

  return m_allCategories.size();
}

/**
 * The data to be put in the given header item.  Overrides virtual method of
 *parent class.
 *
 * @param section :: unused
 * @param orientation :: either the top (horizontal) or left-hand (vertical)
 *header.
 * @param role :: the "role" of the data, which can be one of several enum
 *values.  See Qt docs for more info.
 */
QVariant InterfaceCategoryModel::headerData(int section,
                                            Qt::Orientation orientation,
                                            int role) const {
  UNUSED_ARG(section);

  if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    return QString("Show Interface Categories");

  return QVariant();
}

/**
 * The data assigned to a given item.  Overrides virtual method of parent class.
 *
 * @param index :: the index of the item.
 * @param role :: the "role" of the data, which can be one of several enum
 *values.  See Qt docs for more info.
 */
QVariant InterfaceCategoryModel::data(const QModelIndex &index,
                                      int role) const {
  if (!index.isValid())
    return QVariant();

  if (role == Qt::DisplayRole) {
    return m_allCategories[index.row()];
  }
  if (role == Qt::CheckStateRole) {
    QString category = m_allCategories[index.row()];
    if (m_hiddenCategories.contains(category))
      return Qt::Unchecked;
    else
      return Qt::Checked;
  }

  return QVariant();
}

/**
 * Assigns data to the given item.  Overrides virtual method of parent class.
 *
 * @param index :: the index of the item.
 * @param value :: the data to assign to the item.
 * @param role :: the "role" of the data, which can be one of several enum
 *values.  See Qt docs for more info.
 */
bool InterfaceCategoryModel::setData(const QModelIndex &index,
                                     const QVariant &value, int role) {
  if (!index.isValid())
    return false;

  if (index.isValid() && role == Qt::CheckStateRole) {
    auto category = m_allCategories[index.row()];

    if (value == Qt::Unchecked)
      m_hiddenCategories.insert(category);
    else
      m_hiddenCategories.remove(category);

    emit dataChanged(index, index);
    return true;
  }

  return false;
}

/**
 * The total number of categories.  Overrides virtual method of parent class.
 */
Qt::ItemFlags InterfaceCategoryModel::flags(const QModelIndex &index) const {
  return Qt::ItemIsUserCheckable | QAbstractListModel::flags(index);
}

/**
 * Persist this model's data to the user preferences file.
 */
void InterfaceCategoryModel::saveHiddenCategories() {
  QString propValue =
      static_cast<QStringList>(m_hiddenCategories.toList()).join(";");

  auto &config = Mantid::Kernel::ConfigService::Instance();
  config.setString("interfaces.categories.hidden", propValue.toStdString());
  config.saveConfig(config.getUserFilename());
}

/**
 * Load this model's data from the user preferences file.
 */
void InterfaceCategoryModel::loadHiddenCategories() {
  const QString propValue = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "interfaces.categories.hidden"));
  m_hiddenCategories = propValue.split(";", QString::SkipEmptyParts).toSet();
}

/////////////////////////////////////////////////////
// ManageInterfaceCategories
/////////////////////////////////////////////////////

/**
 * Constructor.
 *
 * @param parent :: the main MantidPlot ApplicationWindow object.
 */
ManageInterfaceCategories::ManageInterfaceCategories(ApplicationWindow *parent)
    : QDialog(parent), m_model(parent->allCategories()) {
  initLayout();
}

/**
 * Set up the dialog.
 */
void ManageInterfaceCategories::initLayout() {
  m_uiForm.setupUi(this);

  m_uiForm.categoryTreeView->setModel(&m_model);
  m_uiForm.categoryTreeView->show();

  // OK button should save any changes and then exit.
  connect(m_uiForm.okButton, SIGNAL(pressed()), &m_model,
          SLOT(saveHiddenCategories()));
  connect(m_uiForm.okButton, SIGNAL(pressed()), this, SLOT(close()));

  // Cancel should just exit.
  connect(m_uiForm.cancelButton, SIGNAL(pressed()), this, SLOT(close()));

  connect(m_uiForm.helpButton, SIGNAL(pressed()), this, SLOT(helpClicked()));
}

/**
 * Opens a web browser showing the wiki page for this dialog.
 */
void ManageInterfaceCategories::helpClicked() {
  QUrl helpUrl("http://www.mantidproject.org/ManageInterfaceCategories");
  MantidDesktopServices::openUrl(helpUrl);
}
