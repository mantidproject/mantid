#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfaceViewQtGUI.h"

#include <boost/lexical_cast.hpp>
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogSavu.h"
#include "MantidQtCustomInterfaces/Tomography/TomoReconToolsUserSettings.h"

#include <MantidAPI/TableRow.h>
#include <MantidAPI/WorkspaceFactory.h>
#include <Poco/String.h>
#include <json/reader.h>
#include <MantidAPI/NotebookWriter.h>
#include <QFileDialog>
#include <QMessageBox>
#include <MantidAPI/AlgorithmManager.h>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

TomoToolConfigDialogSavu::TomoToolConfigDialogSavu(QWidget *parent)
    : QMainWindow(parent) {
  // TODO init the 545231453t54 variables
  m_availPlugins = Mantid::API::WorkspaceFactory::Instance().createTable();
  m_availPlugins->addColumns("str", "name", 4);
  m_currPlugins = Mantid::API::WorkspaceFactory::Instance().createTable();
  m_currPlugins->addColumns("str", "name", 4);
}

int TomoToolConfigDialogSavu::executeQt() {
  this->show();
  QEventLoop el;
  connect(this, SIGNAL(destroyed()), &el, SLOT(quit()));
  return el.exec();
}

void TomoToolConfigDialogSavu::initialiseDialog() {
  throw Mantid::Kernel::Exception::NotImplementedError(
      "SAVU interface not implemented");
}
void TomoToolConfigDialogSavu::setupMethodSelected() {
  throw Mantid::Kernel::Exception::NotImplementedError(
      "SAVU interface not implemented");
}
void TomoToolConfigDialogSavu::setupToolSettingsFromPaths() {
  throw Mantid::Kernel::Exception::NotImplementedError(
      "SAVU interface not implemented");
}

void TomoToolConfigDialogSavu::setupDialogUi() {
  m_savuUi.setupUi(this);
  initSavuWindow();
  this->setWindowModality(Qt::ApplicationModal);
}

void TomoToolConfigDialogSavu::initSavuWindow() {
  // geometry, etc. niceties
  // on the left (just plugin names) 1/2, right: 2/3
  QList<int> sizes{100, 200};
  m_savuUi.splitterPlugins->setSizes(std::move(sizes));

  // Setup Parameter editor tab
  loadAvailablePlugins();
  m_savuUi.treeCurrentPlugins->setHeaderHidden(true);

  // Connect slots
  // Lists/trees
  connect(m_savuUi.listAvailablePlugins, SIGNAL(itemSelectionChanged()), this,
          SLOT(availablePluginSelected()));
  connect(m_savuUi.treeCurrentPlugins, SIGNAL(itemSelectionChanged()), this,
          SLOT(currentPluginSelected()));
  connect(m_savuUi.treeCurrentPlugins, SIGNAL(itemExpanded(QTreeWidgetItem *)),
          this, SLOT(expandedItem(QTreeWidgetItem *)));

  // Buttons
  connect(m_savuUi.btnTransfer, SIGNAL(released()), this,
          SLOT(transferClicked()));
  connect(m_savuUi.btnMoveUp, SIGNAL(released()), this, SLOT(moveUpClicked()));
  connect(m_savuUi.btnMoveDown, SIGNAL(released()), this,
          SLOT(moveDownClicked()));
  connect(m_savuUi.btnRemove, SIGNAL(released()), this, SLOT(removeClicked()));
}

// TODO: what's in this file should become a class of its own,
// 'SavuConfigDialog' or similar

void TomoToolConfigDialogSavu::loadAvailablePlugins() {
  // TODO:: load actual plugins when we know them
  // creating a few relatively realistic choices for now (should crossh check
  //  with the savu api when finalized).
  // - Should also verify the param string is valid json when setting

  // Create plugin table
  Mantid::API::TableRow row = m_availPlugins->appendRow();
  row << "savu.plugins.timeseries_field_corrections"
      << "{}"
      << "Time Series Field Corrections"
      << "Citation info";

  row = m_availPlugins->appendRow();
  row << "savu.plugins.median_filter"
      << "{\"kernel_size\":[1, 3, 3]}"
      << "Median Filter"
      << "Citation info";

  row = m_availPlugins->appendRow();
  row << "savu.plugins.vo_centering"
      << "{}"
      << "Vo Centering"
      << "Citation info";

  row = m_availPlugins->appendRow();
  row << "savu.plugins.simple_recon"
      << "{\"center_of_rotation\":86}"
      << "Simple Reconstruction"
      << "Citation info";

  row = m_availPlugins->appendRow();
  row << "savu.plugins.astra_recon"
      << "{\"center_of_rotation\":\"86\", "
         "\"reconsturction_type\":\"SIRT\", \"number_of_iterations\":5}"
      << "Simple Reconstruction"
      << "Citation info";

  // Update the UI
  refreshAvailablePluginListUI();
}

// Reloads the GUI list of available plugins from the data object ::
// Populating only through this ensures correct indexing.
void TomoToolConfigDialogSavu::refreshAvailablePluginListUI() {
  // Table WS structure, id/params/name/cite
  m_savuUi.listAvailablePlugins->clear();
  const size_t rowcount = m_availPlugins->rowCount();
  for (size_t i = 0; i < rowcount; ++i) {
    const QString str =
        QString::fromStdString(m_availPlugins->cell<std::string>(i, 2));
    m_savuUi.listAvailablePlugins->addItem(std::move(str));
  }
}

// Reloads the GUI list of current plugins from the data object ::
// Populating only through this ensures correct indexing.
void TomoToolConfigDialogSavu::refreshCurrentPluginListUI() {
  // Table WS structure, id/params/name/cite
  m_savuUi.treeCurrentPlugins->clear();
  createPluginTreeEntries(m_currPlugins);
}

// Updates the selected plugin info from Available plugins list.
void TomoToolConfigDialogSavu::availablePluginSelected() {
  if (m_savuUi.listAvailablePlugins->selectedItems().count() != 0) {
    const size_t idx = static_cast<size_t>(
        m_savuUi.listAvailablePlugins->currentIndex().row());
    if (idx < m_availPlugins->rowCount()) {
      m_savuUi.availablePluginDesc->setText(
          tableWSRowToString(m_availPlugins, idx));
    }
  }
}

// Updates the selected plugin info from Current plugins list.
void TomoToolConfigDialogSavu::currentPluginSelected() {
  if (m_savuUi.treeCurrentPlugins->selectedItems().count() != 0) {
    auto currItem = m_savuUi.treeCurrentPlugins->selectedItems()[0];

    while (currItem->parent() != NULL)
      currItem = currItem->parent();

    int topLevelIndex =
        m_savuUi.treeCurrentPlugins->indexOfTopLevelItem(currItem);

    m_savuUi.currentPluginDesc->setText(
        tableWSRowToString(m_currPlugins, topLevelIndex));
  }
}

class OwnTreeWidgetItem : public QTreeWidgetItem {
public:
  OwnTreeWidgetItem(QTreeWidgetItem *parent,
                    QTreeWidgetItem *logicalParent = NULL,
                    const std::string &key = "")
      : QTreeWidgetItem(parent), m_rootParent(logicalParent), m_key(key) {}

  OwnTreeWidgetItem(QStringList list, QTreeWidgetItem *logicalParent = NULL,
                    const std::string &key = "")
      : QTreeWidgetItem(list), m_rootParent(logicalParent), m_key(key) {}

  OwnTreeWidgetItem(QTreeWidgetItem *parent, QStringList list,
                    QTreeWidgetItem *logicalParent = NULL,
                    const std::string &key = "")
      : QTreeWidgetItem(parent, list), m_rootParent(logicalParent), m_key(key) {
  }

  QTreeWidgetItem *getRootParent() { return m_rootParent; }

  std::string getKey() { return m_key; }

private:
  QTreeWidgetItem *m_rootParent;
  std::string m_key;
};

// On user editing a parameter tree item, update the data object to match.
void TomoToolConfigDialogSavu::paramValModified(QTreeWidgetItem *item,
                                                int /*column*/) {
  OwnTreeWidgetItem *ownItem = dynamic_cast<OwnTreeWidgetItem *>(item);
  if (!ownItem)
    return;

  int topLevelIndex = -1;
  if (ownItem->getRootParent() != NULL) {
    topLevelIndex = m_savuUi.treeCurrentPlugins->indexOfTopLevelItem(
        ownItem->getRootParent());
  }
  if (-1 == topLevelIndex)
    return;

  // Recreate the json string from the nodes and write back
  std::string json = m_currPlugins->cell<std::string>(topLevelIndex, 1);
  // potential new line out, and trim spaces
  json.erase(std::remove(json.begin(), json.end(), '\n'), json.end());
  json.erase(std::remove(json.begin(), json.end(), '\r'), json.end());
  json = Poco::trimInPlace(json);

  ::Json::Reader r;
  ::Json::Value root;
  if (r.parse(json, root)) {
    // Look for the key and replace it
    root[ownItem->getKey()] = ownItem->text(0).toStdString();
  }

  m_currPlugins->cell<std::string>(topLevelIndex, 1) =
      ::Json::FastWriter().write(root);
  currentPluginSelected();
}

// When a top level item is expanded, also expand its child items - if tree
// items
void TomoToolConfigDialogSavu::expandedItem(QTreeWidgetItem *item) {
  if (item->parent() == NULL) {
    for (int i = 0; i < item->childCount(); ++i) {
      item->child(i)->setExpanded(true);
    }
  }
}

// Adds one plugin from the available plugins list into the list of
// current plugins
void TomoToolConfigDialogSavu::transferClicked() {
  if (0 == m_savuUi.listAvailablePlugins->selectedItems().count())
    return;

  const int idx = m_savuUi.listAvailablePlugins->currentIndex().row();
  const size_t columnCount = m_currPlugins->columnCount();

  Mantid::API::TableRow row = m_currPlugins->appendRow();
  for (size_t j = 0; j < columnCount; ++j) {
    row << m_availPlugins->cell<std::string>(idx, j);
  }
  createPluginTreeEntry(row);
}

void TomoToolConfigDialogSavu::moveUpClicked() {
  if (0 == m_savuUi.treeCurrentPlugins->selectedItems().count())
    return;

  const size_t idx =
      static_cast<size_t>(m_savuUi.treeCurrentPlugins->currentIndex().row());
  if (idx > 0 && idx < m_currPlugins->rowCount()) {
    // swap row, all columns
    const size_t columnCount = m_currPlugins->columnCount();
    for (size_t j = 0; j < columnCount; ++j) {
      const std::string swap = m_currPlugins->cell<std::string>(idx, j);
      m_currPlugins->cell<std::string>(idx, j) =
          m_currPlugins->cell<std::string>(idx - 1, j);
      m_currPlugins->cell<std::string>(idx - 1, j) = swap;
    }
    refreshCurrentPluginListUI();
  }
}

void TomoToolConfigDialogSavu::moveDownClicked() {
  // TODO: this can be done with the same function as above...
  if (0 == m_savuUi.treeCurrentPlugins->selectedItems().count())
    return;

  const size_t idx =
      static_cast<size_t>(m_savuUi.treeCurrentPlugins->currentIndex().row());
  if (idx < m_currPlugins->rowCount() - 1) {
    // swap all columns

    const size_t columnCount = m_currPlugins->columnCount();
    for (size_t j = 0; j < columnCount; ++j) {
      const std::string swap = m_currPlugins->cell<std::string>(idx, j);
      m_currPlugins->cell<std::string>(idx, j) =
          m_currPlugins->cell<std::string>(idx + 1, j);
      m_currPlugins->cell<std::string>(idx + 1, j) = swap;
    }
    refreshCurrentPluginListUI();
  }
}

void TomoToolConfigDialogSavu::removeClicked() {
  // Also clear ADS entries
  if (0 == m_savuUi.treeCurrentPlugins->selectedItems().count())
    return;

  const int idx = m_savuUi.treeCurrentPlugins->currentIndex().row();
  m_currPlugins->removeRow(idx);

  refreshCurrentPluginListUI();
}

void TomoToolConfigDialogSavu::menuOpenClicked() {
  QString s = QFileDialog::getOpenFileName(0, "Open file", QDir::currentPath(),
                                           "NeXus files (*.nxs);;All files (*)",
                                           new QString("NeXus files (*.nxs)"));
  std::string name = s.toStdString();

  if ("" == name)
    return;

  bool opening = true;
  if (m_currPlugins->rowCount() > 0) {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Open file confirmation",
        "Opening the configuration file will clear the current list."
        "\nWould you like to continue?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
      opening = false;
    }
  }

  if (opening) {
    loadSavuTomoConfig(name, m_currPlugins);

    m_currentParamPath = name;
    refreshCurrentPluginListUI();
  }
}

/**
* Load a savu tomo config file into the current plugin list, overwriting it.
* Uses the algorithm LoadSavuTomoConfig
*/
void TomoToolConfigDialogSavu::loadSavuTomoConfig(
    std::string &filePath, Mantid::API::ITableWorkspace_sptr &currentPlugins) {
  // try to load tomo reconstruction parametereization file
  auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
      "LoadSavuTomoConfig");
  alg->initialize();
  alg->setPropertyValue("Filename", filePath);
  alg->setPropertyValue("OutputWorkspace", createUniqueNameHidden());
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        std::string("Error when trying to load tomographic reconstruction "
                    "parameter file: ") +
        e.what());
  }

  // new processing plugins list
  try {
    currentPlugins = alg->getProperty("OutputWorkspace");
  } catch (std::exception &e) {
    userError("Could not load config file", "Failed to load the file "
                                            "with the following error: " +
                                                std::string(e.what()));
  }
}
void TomoToolConfigDialogSavu::menuSaveClicked() {
  if (m_currentParamPath.empty()) {
    menuSaveAsClicked();
    return;
  }

  if (0 == m_currPlugins->rowCount()) {
    // Alert that the plugin list is empty
    QMessageBox::information(this, tr("Unable to save file"),
                             "The current plugin list is empty, please add one "
                             "or more to the list.");
  } else {
    AnalysisDataService::Instance().add(createUniqueNameHidden(),
                                        m_currPlugins);
    std::string csvWorkspaceNames = m_currPlugins->name();

    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "SaveTomoConfig");
    alg->initialize();
    alg->setPropertyValue("Filename", m_currentParamPath);
    alg->setPropertyValue("InputWorkspaces", csvWorkspaceNames);
    alg->execute();

    if (!alg->isExecuted()) {
      throw std::runtime_error("Error when trying to save config file");
    }
  }
}
size_t TomoToolConfigDialogSavu::g_nameSeqNo = 0;

// Build a unique (and hidden) name for the table ws
std::string TomoToolConfigDialogSavu::createUniqueNameHidden() {
  std::string name;
  do {
    // with __ prefix => hidden
    name = "__TomoConfigTableWS_Seq_" +
           boost::lexical_cast<std::string>(g_nameSeqNo++);
  } while (AnalysisDataService::Instance().doesExist(name));

  return name;
}

void TomoToolConfigDialogSavu::menuSaveAsClicked() {
  QString s = QFileDialog::getSaveFileName(0, "Save file", QDir::currentPath(),
                                           "NeXus files (*.nxs);;All files (*)",
                                           new QString("NeXus files (*.nxs)"));
  std::string name = s.toStdString();
  if ("" == name)
    return;

  m_currentParamPath = name;
  menuSaveClicked();
}

QString TomoToolConfigDialogSavu::tableWSRowToString(ITableWorkspace_sptr table,
                                                     size_t i) {
  std::stringstream msg;
  msg << "ID: " << table->cell<std::string>(i, 0)
      << "\nParams: " << table->cell<std::string>(i, 1)
      << "\nName: " << table->cell<std::string>(i, 2)
      << "\nCite: " << table->cell<std::string>(i, 3);
  return QString::fromStdString(msg.str());
}

/**
* Creates a treewidget item for a row of a table workspace.
*
* @param row Row from a table workspace with each row specfying a savu plugin
*/
void TomoToolConfigDialogSavu::createPluginTreeEntry(TableRow &row) {
  QStringList idStr, nameStr, citeStr, paramsStr;
  idStr.push_back(QString::fromStdString("ID: " + row.cell<std::string>(0)));
  nameStr.push_back(
      QString::fromStdString("Name: " + row.cell<std::string>(2)));
  citeStr.push_back(
      QString::fromStdString("Cite: " + row.cell<std::string>(3)));
  paramsStr.push_back(QString::fromStdString("Params:"));

  // Setup editable tree items
  QList<QTreeWidgetItem *> items;
  OwnTreeWidgetItem *pluginBaseItem = new OwnTreeWidgetItem(nameStr);
  OwnTreeWidgetItem *pluginParamsItem =
      new OwnTreeWidgetItem(pluginBaseItem, paramsStr, pluginBaseItem);

  // Add to the tree list. Adding now to build hierarchy for later setItemWidget
  // call
  items.push_back(new OwnTreeWidgetItem(pluginBaseItem, idStr, pluginBaseItem));
  items.push_back(
      new OwnTreeWidgetItem(pluginBaseItem, nameStr, pluginBaseItem));
  items.push_back(
      new OwnTreeWidgetItem(pluginBaseItem, citeStr, pluginBaseItem));
  items.push_back(pluginParamsItem);

  // Params will be a json string which needs splitting into child tree items
  // [key/value]
  ::Json::Value root;
  std::string paramString = row.cell<std::string>(1);
  ::Json::Reader r;
  if (r.parse(paramString, root)) {
    auto members = root.getMemberNames();
    for (auto it = members.begin(); it != members.end(); ++it) {
      OwnTreeWidgetItem *container =
          new OwnTreeWidgetItem(pluginParamsItem, pluginBaseItem);

      QWidget *w = new QWidget();
      w->setAutoFillBackground(true);

      QHBoxLayout *layout = new QHBoxLayout(w);
      layout->setMargin(1);
      QLabel *label1 = new QLabel(QString::fromStdString((*it) + ": "));

      QTreeWidget *paramContainerTree = new QTreeWidget(w);
      connect(paramContainerTree, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
              this, SLOT(paramValModified(QTreeWidgetItem *, int)));
      paramContainerTree->setHeaderHidden(true);
      paramContainerTree->setIndentation(0);

      auto jsonVal = root.get(*it, "");
      std::string valStr = pluginParamValString(jsonVal, *it);

      QStringList paramVal(QString::fromStdString(valStr));
      OwnTreeWidgetItem *paramValueItem =
          new OwnTreeWidgetItem(paramVal, pluginBaseItem, *it);
      paramValueItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);

      paramContainerTree->addTopLevelItem(paramValueItem);
      QRect rect = paramContainerTree->visualItemRect(paramValueItem);
      paramContainerTree->setMaximumHeight(rect.height());
      paramContainerTree->setFrameShape(QFrame::NoFrame);

      layout->addWidget(label1);
      layout->addWidget(paramContainerTree);

      pluginParamsItem->addChild(container);
      m_savuUi.treeCurrentPlugins->setItemWidget(container, 0, w);
    }
  }

  pluginBaseItem->addChildren(items);
  m_savuUi.treeCurrentPlugins->addTopLevelItem(pluginBaseItem);
}

/**
* This is a kind of .asString() method for arrays. It iterates
* through the array elements and builds the string enclosed by [].
*
* @param jsonVal Value of a parameter that seems to be an array
*(isArray()==true)
* @param name Name of the parameter (to give informative messages)
*
* @return String with a parameter value(s), enclosed by [] and
* separated by commas
*/
std::string
TomoToolConfigDialogSavu::paramValStringFromArray(const Json::Value &jsonVal,
                                                  const std::string &name) {
  std::string s;
  s = "[";
  for (Json::ArrayIndex i = 0; i < jsonVal.size(); ++i) {
    if (jsonVal[i].isArray()) {
      userWarning(
          "Could not recognize parameter value in list/array",
          "The value of parameter '" + name +
              "' could not be interpreted "
              "as a string. It does not seem to be well formed or supported. "
              "For example, parameter values given as lists of lists are not "
              "supported.");
    } else {
      try {
        s += jsonVal[i].asString() + " ,";
      } catch (std::exception &e) {
        userWarning(
            "Could not recognize value in list/array of values",
            "The " + boost::lexical_cast<std::string>(i) +
                "-th value of the list/array could not be interpreted "
                "as a text string. It will be empty in the list of current "
                "plugins. You can still edit it. Error details: " +
                std::string(e.what()));
      }
    }
  }
  // this could be s.back() with C++11
  s[s.length() - 1] = ']'; // and last comma becomes closing ]
  return s;
}

/**
* Build a string with the value of a parameter in a json
* string. Works for scalar and list/array values.
*
* @param jsonVal Value of a parameter that seems to be an array
* @param name Name of the parameter (to give informative messages)
*
* @return String with a parameter value
*/
std::string
TomoToolConfigDialogSavu::pluginParamValString(const Json::Value &jsonVal,
                                               const std::string &name) {
  std::string s;
  // string and numeric values can (normally) be converted to string but arrays
  // cannot
  if (!jsonVal.isArray()) {
    try {
      s = jsonVal.asString();
    } catch (std::exception &e) {
      userWarning(
          "Could not recognize parameter value",
          "The value of parameter '" + name +
              "' could not be interpreted "
              "as a string. It will be empty in the list of current plugins. "
              "You can still edit it. Error details: " +
              std::string(e.what()));
    }
  } else {
    s = paramValStringFromArray(jsonVal, name);
  }
  return s;
}

void TomoToolConfigDialogSavu::createPluginTreeEntries(
    Mantid::API::ITableWorkspace_sptr table) {
  for (size_t i = 0; i < table->rowCount(); ++i) {
    Mantid::API::TableRow r = table->getRow(i);
    createPluginTreeEntry(r);
  }
}

/** TODO move into a class, extract from here and QtView interface
* Show an error (serious) message to the user (pop up)
*
* @param err Basic error title
* @param description More detailed explanation, hints, additional
* information, etc.
*/
void TomoToolConfigDialogSavu::userError(const std::string &err,
                                         const std::string &description) {
  QMessageBox::critical(this, QString::fromStdString(err),
                        QString::fromStdString(description), QMessageBox::Ok,
                        QMessageBox::Ok);
}

/**
* Show a warning message to the user (pop up)
*
* @param err Basic error title
* @param description More detailed explanation, hints, additional
* information, etc.
*/
void TomoToolConfigDialogSavu::userWarning(const std::string &err,
                                           const std::string &description) {
  QMessageBox::warning(this, QString::fromStdString(err),
                       QString::fromStdString(description), QMessageBox::Ok,
                       QMessageBox::Ok);
}

} // namespace CustomInterfaces
} // namespace MantidQt
