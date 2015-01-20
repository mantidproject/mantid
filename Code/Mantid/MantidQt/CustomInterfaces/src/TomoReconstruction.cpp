#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtCustomInterfaces/TomoReconstruction.h"
#include "MantidAPI/TableRow.h"

#include "QFileDialog"
#include "QMessageBox"

#include <boost/lexical_cast.hpp>
#include <jsoncpp/json/json.h>

using namespace Mantid::API;

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(TomoReconstruction);
  }
}

class OwnTreeWidgetItem : public QTreeWidgetItem
{
public:
  OwnTreeWidgetItem(QTreeWidgetItem *parent, QTreeWidgetItem *logicalParent = NULL,
                    const std::string key = ""):
    QTreeWidgetItem(parent), m_rootParent(logicalParent), m_key(key)
  {
  }

  OwnTreeWidgetItem(QStringList list, QTreeWidgetItem *logicalParent = NULL, const std::string key = ""):
    QTreeWidgetItem(list), m_rootParent(logicalParent), m_key(key)
  {
  }

  OwnTreeWidgetItem(QTreeWidgetItem *parent, QStringList list, QTreeWidgetItem *logicalParent = NULL,
                    const std::string key = ""):
    QTreeWidgetItem(parent, list), m_rootParent(logicalParent), m_key(key)
  {
  }

  QTreeWidgetItem* getRootParent()
  {
    return m_rootParent;
  }

  std::string getKey()
  {
    return m_key;
  }

private:
  QTreeWidgetItem* m_rootParent;
  std::string m_key;
};

using namespace MantidQt::CustomInterfaces;

size_t TomoReconstruction::nameSeqNo = 0;

TomoReconstruction::TomoReconstruction(QWidget *parent) : UserSubWindow(parent)
{
  m_currentParamPath = "";
}

void TomoReconstruction::initLayout()
{
  // TODO: should split the tabs out into their own files
  m_uiForm.setupUi(this);  

  // geometry, etc. niceties
  // on the left (just plugin names) 1/2, right: 2/3
  QList<int> sizes;
  sizes.push_back(100);
  sizes.push_back(200);
  m_uiForm.splitterPlugins->setSizes(sizes);

  // Setup Parameter editor tab  
  loadAvailablePlugins();
  m_uiForm.treeCurrentPlugins->setHeaderHidden(true);

  // Setup the setup tab

  // Setup Run tab
  loadSettings();

  // Connect slots  
  // Menu Items
  connect(m_uiForm.actionOpen, SIGNAL(triggered()), this, SLOT(menuOpenClicked()));
  connect(m_uiForm.actionSave, SIGNAL(triggered()), this, SLOT(menuSaveClicked()));
  connect(m_uiForm.actionSaveAs, SIGNAL(triggered()), this, SLOT(menuSaveAsClicked()));  

  // Lists/trees
  connect(m_uiForm.listAvailablePlugins, SIGNAL(itemSelectionChanged()), this, SLOT(availablePluginSelected()));  
  connect(m_uiForm.treeCurrentPlugins, SIGNAL(itemSelectionChanged()), this, SLOT(currentPluginSelected())); 
  connect(m_uiForm.treeCurrentPlugins, SIGNAL(itemExpanded(QTreeWidgetItem*)), this,
          SLOT(expandedItem(QTreeWidgetItem*)));
  
  // Buttons    
  connect(m_uiForm.btnTransfer, SIGNAL(released()), this, SLOT(transferClicked()));  
  connect(m_uiForm.btnMoveUp, SIGNAL(released()), this, SLOT(moveUpClicked()));  
  connect(m_uiForm.btnMoveDown, SIGNAL(released()), this, SLOT(moveDownClicked()));  
  connect(m_uiForm.btnRemove, SIGNAL(released()), this, SLOT(removeClicked()));  
}


/**
 * Load the setting for each tab on the interface.
 *
 * This includes setting the default browsing directory to be the default save directory.
 */
void TomoReconstruction::loadSettings()
{
  // TODO:
}

void TomoReconstruction::loadAvailablePlugins()
{
  // TODO:: load actual plugins -
  // creating a couple of test choices for now (should fetch from remote api when implemented)
  // - Should also verify the param string is valid json when setting
  // Create plugin tables
 
  auto plug1 = Mantid::API::WorkspaceFactory::Instance().createTable();
  auto plug2 = Mantid::API::WorkspaceFactory::Instance().createTable();
  plug1->addColumns("str","name",4);
  plug2->addColumns("str","name",4);
  Mantid::API::TableRow plug1row = plug1->appendRow();
  Mantid::API::TableRow plug2row = plug2->appendRow();
  plug1row << "10001" << "{\"key\":\"val\",\"key2\":\"val2\"}" << "Plugin #1" << "Citation info";
  plug2row << "10002" << "{\"key\":\"val\",\"key2\":\"val2\"}" << "Plugin #2" << "Citation info";
  
  m_availPlugins.push_back(plug1);
  m_availPlugins.push_back(plug2);

  // Update the UI
  refreshAvailablePluginListUI();
}

// Reloads the GUI list of available plugins from the data object ::
// Populating only through this ensures correct indexing.
void TomoReconstruction::refreshAvailablePluginListUI()
{
  // Table WS structure, id/params/name/cite
  m_uiForm.listAvailablePlugins->clear();
  for(auto it=m_availPlugins.begin();it!=m_availPlugins.end();++it)
  {
    QString str = QString::fromStdString((*it)->cell<std::string>(0,2));
    m_uiForm.listAvailablePlugins->addItem(str);
  }
}

// Reloads the GUI list of current plugins from the data object ::
// Populating only through this ensures correct indexing.
void TomoReconstruction::refreshCurrentPluginListUI()
{
  // Table WS structure, id/params/name/cite
  m_uiForm.treeCurrentPlugins->clear();
  for(auto it=m_currPlugins.begin();it!=m_currPlugins.end();++it)
  {
    createPluginTreeEntry(*it);
  }
}

// Updates the selected plugin info from Available plugins list.
void TomoReconstruction::availablePluginSelected()
{
  if(m_uiForm.listAvailablePlugins->selectedItems().count() != 0)
  {  
    int currInd = m_uiForm.listAvailablePlugins->currentIndex().row();
    m_uiForm.availablePluginDesc->setText(tableWSToString(m_availPlugins[currInd]));
  }
}

// Updates the selected plugin info from Current plugins list.
void TomoReconstruction::currentPluginSelected()
{
  if(m_uiForm.treeCurrentPlugins->selectedItems().count() != 0 )
  { 
    auto currItem = m_uiForm.treeCurrentPlugins->selectedItems()[0];

    while(currItem->parent() != NULL)
      currItem = currItem->parent();

    int topLevelIndex = m_uiForm.treeCurrentPlugins->indexOfTopLevelItem(currItem);

    m_uiForm.currentPluginDesc->setText(tableWSToString(m_currPlugins[topLevelIndex]));
  }
}

// On user editing a parameter tree item, update the data object to match.
void TomoReconstruction::paramValModified(QTreeWidgetItem* item, int /*column*/)
{  
  OwnTreeWidgetItem *ownItem = dynamic_cast<OwnTreeWidgetItem*>(item);
  int topLevelIndex = -1;

  if(ownItem->getRootParent() != NULL)
  {
    topLevelIndex = m_uiForm.treeCurrentPlugins->indexOfTopLevelItem(ownItem->getRootParent());
  }
  
  if(topLevelIndex != -1)
  {
    // Recreate the json string from the nodes and write back
    ::Json::Value root;
    std::string json = m_currPlugins[topLevelIndex]->cell<std::string>(0,1);
    ::Json::Reader r;

    if(r.parse(json,root))
    {
      // Look for the key and replace it
      root[ownItem->getKey()] = ownItem->text(0).toStdString();
    }
    
    m_currPlugins[topLevelIndex]->cell<std::string>(0,1) = ::Json::FastWriter().write(root);
    currentPluginSelected();
  }
}

// When a top level item is expanded, also expand its child items - if tree items
void TomoReconstruction::expandedItem(QTreeWidgetItem* item)
{
  if(item->parent() == NULL)
  {
    for(int i=0; i<item->childCount();++i)
    {
      item->child(i)->setExpanded(true); 
    }
  }
}



// Clones the selected available plugin object into the current plugin vector and refreshes the UI.
void TomoReconstruction::transferClicked()
{
  if(m_uiForm.listAvailablePlugins->selectedItems().count() != 0)
  {  
    int currInd = m_uiForm.listAvailablePlugins->currentIndex().row();
    
    ITableWorkspace_sptr newPlugin(m_availPlugins.at(currInd)->clone());

    // Creates a hidden ws entry (with name) in the ADS    
    AnalysisDataService::Instance().add(createUniqueNameHidden(), newPlugin);
   
    m_currPlugins.push_back(newPlugin);
    
    createPluginTreeEntry(newPlugin);
  }
}

void TomoReconstruction::moveUpClicked()
{
  if(m_uiForm.treeCurrentPlugins->selectedItems().count() != 0)
  {      
    int currInd = m_uiForm.treeCurrentPlugins->currentIndex().row();
    if(currInd > 0)
    {
      std::iter_swap(m_currPlugins.begin()+currInd,m_currPlugins.begin()+currInd-1);    
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::moveDownClicked()
{
  if(m_uiForm.treeCurrentPlugins->selectedItems().count() != 0)
  {      
    unsigned int currInd = m_uiForm.treeCurrentPlugins->currentIndex().row();
    if(currInd < m_currPlugins.size()-1 )
    {
      std::iter_swap(m_currPlugins.begin()+currInd,m_currPlugins.begin()+currInd+1);    
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::removeClicked()
{
  // Also clear ADS entries
  if(m_uiForm.treeCurrentPlugins->selectedItems().count() != 0)
  {  
    int currInd = m_uiForm.treeCurrentPlugins->currentIndex().row();
    auto curr = *(m_currPlugins.begin()+currInd);    

    if(AnalysisDataService::Instance().doesExist(curr->getName()))
    {
        AnalysisDataService::Instance().remove(curr->getName());
    }
    m_currPlugins.erase(m_currPlugins.begin()+currInd);
    
    refreshCurrentPluginListUI();
  }
}

void TomoReconstruction::menuOpenClicked()
{ 
  QString s = QFileDialog::getOpenFileName(0, "Open file", QDir::currentPath(),
                                           "NeXus files (*.nxs);;All files (*.*)",
                                           new QString("NeXus files (*.nxs)"));
  std::string returned = s.toStdString();
  if(returned != "")
  {
    bool opening = true;
    
    if(m_currPlugins.size() > 0)
    {
      QMessageBox::StandardButton reply = QMessageBox::question(this,
          "Open file confirmation", "Opening the configuration file will clear the current list."
                                                                "\nWould you like to continue?",
          QMessageBox::Yes|QMessageBox::No);
      if (reply == QMessageBox::No) 
      {
        opening = false;
      }     
    } 

    if(opening)
    {
      loadTomoConfig(returned, m_currPlugins);

      m_currentParamPath = returned;
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::menuSaveClicked()
{
  if(m_currentParamPath == "")
  {
    menuSaveAsClicked();
    return;
  }

  if(m_currPlugins.size() != 0)
  {
    std::string csvWorkspaceNames = "";
    for(auto it=m_currPlugins.begin();it!=m_currPlugins.end();++it)
    {
      csvWorkspaceNames = csvWorkspaceNames + (*it)->name();
      if(it!=m_currPlugins.end()-1)
        csvWorkspaceNames = csvWorkspaceNames + ",";
    }
  
    auto alg = Algorithm::fromString("SaveTomoConfig");
    alg->initialize();
    alg->setPropertyValue("Filename", m_currentParamPath);
    alg->setPropertyValue("InputWorkspaces", csvWorkspaceNames);
    alg->execute();

    if (!alg->isExecuted())
    {
      throw std::runtime_error("Error when trying to save config file");
    }
  }
  else
  {
    // Alert that the plugin list is empty
    QMessageBox::information(this, tr("Unable to save file"),
                             "The current plugin list is empty, please add one or more to the list.");
  }
}

void TomoReconstruction::menuSaveAsClicked()
{
  QString s = QFileDialog::getSaveFileName(0,"Save file",QDir::currentPath(),
                                           "NeXus files (*.nxs);;All files (*.*)",
                                           new QString("NeXus files (*.nxs)"));
  std::string returned = s.toStdString();
  if(returned != "")
  {
    m_currentParamPath = returned;
    menuSaveClicked();
  }
}

QString TomoReconstruction::tableWSToString(ITableWorkspace_sptr table)
{
  std::stringstream msg;
  TableRow row = table->getFirstRow();
  msg << "ID: " << 
    table->cell<std::string>(0,0) << std::endl << "Params: " <<
    table->cell<std::string>(0,1) << std::endl << "Name: " <<
    table->cell<std::string>(0,2) << std::endl << "Cite: " <<
    table->cell<std::string>(0,3);
  return QString::fromStdString(msg.str());
}

/// Load a tomo config file into the current plugin list, overwriting it.
/// Uses the algorithm LoadTomoConfig
void TomoReconstruction::loadTomoConfig(std::string &filePath,
                                        std::vector<Mantid::API::ITableWorkspace_sptr> &currentPlugins)
{
  // try to load tomo reconstruction parametereization file
  auto alg = Algorithm::fromString("LoadTomoConfig");
  alg->initialize();
  alg->setPropertyValue("Filename", filePath);
  alg->setPropertyValue("OutputWorkspaces", createUniqueNameHidden());
  try {
    alg->execute();
  } catch(std::runtime_error& e) {
    throw std::runtime_error(std::string("Error when trying to save tomographic reconstruction parameter file: ")
                             + e.what());
  }

  // Clear the plugin list and remove any item in the ADS entries
  for(auto it = currentPlugins.begin(); it!=currentPlugins.end();++it)
  {
    ITableWorkspace_sptr curr = boost::dynamic_pointer_cast<ITableWorkspace>((*it));
    if(AnalysisDataService::Instance().doesExist(curr->getName()))
    {
      AnalysisDataService::Instance().remove(curr->getName());
    }
  }
  currentPlugins.clear();

  // new processing plugins list
  ITableWorkspace_sptr ws = alg->getProperty("OutputWorkspace");
  currentPlugins.push_back(ws);
}

// Build a unique (and hidden) name for the table ws
std::string TomoReconstruction::createUniqueNameHidden()
{
  std::string name;
  do 
  {
    // with __ prefix => hidden
    name = "__TomoConfigTableWS_Seq_" +  boost::lexical_cast<std::string>(nameSeqNo++);
  } while(AnalysisDataService::Instance().doesExist(name));

  return name;
}

// Creates a treewidget item for a table workspace
void TomoReconstruction::createPluginTreeEntry(Mantid::API::ITableWorkspace_sptr table)
{ 
  QStringList idStr, nameStr, citeStr, paramsStr;
  idStr.push_back(QString::fromStdString("ID: " + table->cell<std::string>(0,0)));
  nameStr.push_back(QString::fromStdString("Name: " + table->cell<std::string>(0,2)));
  citeStr.push_back(QString::fromStdString("Cite: " + table->cell<std::string>(0,3)));
  paramsStr.push_back(QString::fromStdString("Params:"));

  // Setup editable tree items
  QList<QTreeWidgetItem*> items;
  OwnTreeWidgetItem *pluginBaseItem = new OwnTreeWidgetItem(nameStr);
  OwnTreeWidgetItem *pluginParamsItem = new OwnTreeWidgetItem(pluginBaseItem, paramsStr, pluginBaseItem);

  // Add to the tree list. Adding now to build hierarchy for later setItemWidget call
  items.push_back(new OwnTreeWidgetItem(pluginBaseItem, idStr, pluginBaseItem));
  items.push_back(new OwnTreeWidgetItem(pluginBaseItem, nameStr, pluginBaseItem));
  items.push_back(new OwnTreeWidgetItem(pluginBaseItem, citeStr, pluginBaseItem));
  items.push_back(pluginParamsItem);

  // Params will be a json string which needs splitting into child tree items [key/value]
  ::Json::Value root;
  std::string json = table->cell<std::string>(0,1);
  ::Json::Reader r;
  if(r.parse(json,root))
  {
    auto members = root.getMemberNames();
    for(auto it=members.begin();it!=members.end();++it)
    {
      OwnTreeWidgetItem *container = new OwnTreeWidgetItem(pluginParamsItem, pluginBaseItem);
      
      QWidget *w = new QWidget();
      w->setAutoFillBackground(true);
    
      QHBoxLayout *layout = new QHBoxLayout(w);
      layout->setMargin(1);
      QLabel* label1 = new QLabel(QString::fromStdString((*it) + ": ")); 

      QTreeWidget *paramContainerTree = new QTreeWidget(w);    
      connect(paramContainerTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this,
              SLOT(paramValModified(QTreeWidgetItem*,int)));
      paramContainerTree->setHeaderHidden(true);
      paramContainerTree->setIndentation(0);
      
      QStringList paramVal(QString::fromStdString(root[*it].asString()));
      OwnTreeWidgetItem *paramValueItem = new OwnTreeWidgetItem(paramVal, pluginBaseItem, *it);
      paramValueItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled );

      paramContainerTree->addTopLevelItem(paramValueItem);
      QRect rect = paramContainerTree->visualItemRect(paramValueItem);    
      paramContainerTree->setMaximumHeight(rect.height());
      paramContainerTree->setFrameShape(QFrame::NoFrame);

      layout->addWidget(label1); 
      layout->addWidget(paramContainerTree);

      pluginParamsItem->addChild(container); 
      m_uiForm.treeCurrentPlugins->setItemWidget(container,0,w);
    }     
  }  

  pluginBaseItem->addChildren(items);
  m_uiForm.treeCurrentPlugins->addTopLevelItem(pluginBaseItem);
}