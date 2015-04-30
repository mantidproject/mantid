#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtCustomInterfaces/TomoReconstruction/TomoReconstruction.h"

#include <boost/lexical_cast.hpp>

#include <Poco/String.h>

#include <QFileDialog>
#include <QMessageBox>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

// TODO: what's in this file should become a class of its own,
// 'SavuConfigDialog' or similar

void TomoReconstruction::loadAvailablePlugins() {
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
void TomoReconstruction::refreshAvailablePluginListUI() {
  // Table WS structure, id/params/name/cite
  m_uiSavu.listAvailablePlugins->clear();
  for (size_t i = 0; i < m_availPlugins->rowCount(); ++i) {
    QString str =
        QString::fromStdString(m_availPlugins->cell<std::string>(i, 2));
    m_uiSavu.listAvailablePlugins->addItem(str);
  }
}

// Reloads the GUI list of current plugins from the data object ::
// Populating only through this ensures correct indexing.
void TomoReconstruction::refreshCurrentPluginListUI() {
  // Table WS structure, id/params/name/cite
  m_uiSavu.treeCurrentPlugins->clear();
  createPluginTreeEntries(m_currPlugins);
}

// Updates the selected plugin info from Available plugins list.
void TomoReconstruction::availablePluginSelected() {
  if (m_uiSavu.listAvailablePlugins->selectedItems().count() != 0) {
    size_t idx = static_cast<size_t>(
        m_uiSavu.listAvailablePlugins->currentIndex().row());
    if (idx < m_availPlugins->rowCount()) {
      m_uiSavu.availablePluginDesc->setText(
          tableWSRowToString(m_availPlugins, idx));
    }
  }
}

// Updates the selected plugin info from Current plugins list.
void TomoReconstruction::currentPluginSelected() {
  if (m_uiSavu.treeCurrentPlugins->selectedItems().count() != 0) {
    auto currItem = m_uiSavu.treeCurrentPlugins->selectedItems()[0];

    while (currItem->parent() != NULL)
      currItem = currItem->parent();

    int topLevelIndex =
        m_uiSavu.treeCurrentPlugins->indexOfTopLevelItem(currItem);

    m_uiSavu.currentPluginDesc->setText(
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
void TomoReconstruction::paramValModified(QTreeWidgetItem *item,
                                          int /*column*/) {
  OwnTreeWidgetItem *ownItem = dynamic_cast<OwnTreeWidgetItem *>(item);
  int topLevelIndex = -1;

  if (ownItem->getRootParent() != NULL) {
    topLevelIndex = m_uiSavu.treeCurrentPlugins->indexOfTopLevelItem(
        ownItem->getRootParent());
  }

  if (topLevelIndex != -1) {
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
}

// When a top level item is expanded, also expand its child items - if tree
// items
void TomoReconstruction::expandedItem(QTreeWidgetItem *item) {
  if (item->parent() == NULL) {
    for (int i = 0; i < item->childCount(); ++i) {
      item->child(i)->setExpanded(true);
    }
  }
}

// Adds one plugin from the available plugins list into the list of
// current plugins
void TomoReconstruction::transferClicked() {
  if (m_uiSavu.listAvailablePlugins->selectedItems().count() != 0) {
    int idx = m_uiSavu.listAvailablePlugins->currentIndex().row();
    Mantid::API::TableRow row = m_currPlugins->appendRow();
    for (size_t j = 0; j < m_currPlugins->columnCount(); ++j) {
      row << m_availPlugins->cell<std::string>(idx, j);
    }
    createPluginTreeEntry(row);
  }
}

void TomoReconstruction::moveUpClicked() {
  if (m_uiSavu.treeCurrentPlugins->selectedItems().count() != 0) {
    size_t idx =
        static_cast<size_t>(m_uiSavu.treeCurrentPlugins->currentIndex().row());
    if (idx > 0 && idx < m_currPlugins->rowCount()) {
      // swap row, all columns
      for (size_t j = 0; j < m_currPlugins->columnCount(); ++j) {
        std::string swap = m_currPlugins->cell<std::string>(idx, j);
        m_currPlugins->cell<std::string>(idx, j) =
            m_currPlugins->cell<std::string>(idx - 1, j);
        m_currPlugins->cell<std::string>(idx - 1, j) = swap;
      }
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::moveDownClicked() {
  // TODO: this can be done with the same function as above...
  if (m_uiSavu.treeCurrentPlugins->selectedItems().count() != 0) {
    size_t idx =
        static_cast<size_t>(m_uiSavu.treeCurrentPlugins->currentIndex().row());
    if (idx < m_currPlugins->rowCount() - 1) {
      // swap all columns
      for (size_t j = 0; j < m_currPlugins->columnCount(); ++j) {
        std::string swap = m_currPlugins->cell<std::string>(idx, j);
        m_currPlugins->cell<std::string>(idx, j) =
            m_currPlugins->cell<std::string>(idx + 1, j);
        m_currPlugins->cell<std::string>(idx + 1, j) = swap;
      }
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::removeClicked() {
  // Also clear ADS entries
  if (m_uiSavu.treeCurrentPlugins->selectedItems().count() != 0) {
    int idx = m_uiSavu.treeCurrentPlugins->currentIndex().row();
    m_currPlugins->removeRow(idx);

    refreshCurrentPluginListUI();
  }
}

void TomoReconstruction::menuOpenClicked() {
  QString s =
      QFileDialog::getOpenFileName(0, "Open file", QDir::currentPath(),
                                   "NeXus files (*.nxs);;All files (*.*)",
                                   new QString("NeXus files (*.nxs)"));
  std::string returned = s.toStdString();
  if (returned != "") {
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
      loadSavuTomoConfig(returned, m_currPlugins);

      m_currentParamPath = returned;
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::menuSaveClicked() {
  if (m_currentParamPath == "") {
    menuSaveAsClicked();
    return;
  }

  if (m_currPlugins->rowCount() != 0) {
    AnalysisDataService::Instance().add(createUniqueNameHidden(),
                                        m_currPlugins);
    std::string csvWorkspaceNames = m_currPlugins->name();

    auto alg = Algorithm::fromString("SaveTomoConfig");
    alg->initialize();
    alg->setPropertyValue("Filename", m_currentParamPath);
    alg->setPropertyValue("InputWorkspaces", csvWorkspaceNames);
    alg->execute();

    if (!alg->isExecuted()) {
      throw std::runtime_error("Error when trying to save config file");
    }
  } else {
    // Alert that the plugin list is empty
    QMessageBox::information(this, tr("Unable to save file"),
                             "The current plugin list is empty, please add one "
                             "or more to the list.");
  }
}

void TomoReconstruction::menuSaveAsClicked() {
  QString s =
      QFileDialog::getSaveFileName(0, "Save file", QDir::currentPath(),
                                   "NeXus files (*.nxs);;All files (*.*)",
                                   new QString("NeXus files (*.nxs)"));
  std::string returned = s.toStdString();
  if (returned != "") {
    m_currentParamPath = returned;
    menuSaveClicked();
  }
}

QString TomoReconstruction::tableWSRowToString(ITableWorkspace_sptr table,
                                               size_t i) {
  std::stringstream msg;
  msg << "ID: " << table->cell<std::string>(i, 0) << std::endl
      << "Params: " << table->cell<std::string>(i, 1) << std::endl
      << "Name: " << table->cell<std::string>(i, 2) << std::endl
      << "Cite: " << table->cell<std::string>(i, 3);
  return QString::fromStdString(msg.str());
}

/**
 * Creates a treewidget item for a row of a table workspace.
 *
 * @param row Row from a table workspace with each row specfying a savu plugin
 */
void TomoReconstruction::createPluginTreeEntry(TableRow &row) {
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
      m_uiSavu.treeCurrentPlugins->setItemWidget(container, 0, w);
    }
  }

  pluginBaseItem->addChildren(items);
  m_uiSavu.treeCurrentPlugins->addTopLevelItem(pluginBaseItem);
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
TomoReconstruction::paramValStringFromArray(const Json::Value &jsonVal,
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
std::string TomoReconstruction::pluginParamValString(const Json::Value &jsonVal,
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

void TomoReconstruction::createPluginTreeEntries(ITableWorkspace_sptr table) {
  for (size_t i = 0; i < table->rowCount(); ++i) {
    TableRow r = table->getRow(i);
    createPluginTreeEntry(r);
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
