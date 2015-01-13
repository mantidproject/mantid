#include "MantidDock.h"
#include "MantidUI.h"
#include "MantidMatrix.h"
#include "../ApplicationWindow.h"
#include "../pixmaps.h"
#include "MantidWSIndexDialog.h"
#include "FlowLayout.h"
#include "WorkspaceIcons.h"

#include <MantidAPI/AlgorithmFactory.h>
#include <MantidAPI/FileProperty.h>
#include <MantidAPI/WorkspaceGroup.h>
#include <MantidGeometry/MDGeometry/IMDDimension.h>
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include <MantidQtMantidWidgets/LineEditWithClear.h>
#include <MantidQtAPI/InterfaceManager.h>
#include <MantidQtAPI/Message.h>

#include <boost/assign/list_of.hpp>

#include <Poco/Path.h>

#include <algorithm>
#include <sstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace
{
  /// static logger for dock widget
  Mantid::Kernel::Logger docklog("MantidDockWidget");
  Mantid::Kernel::Logger treelog("MantidTreeWidget");

  WorkspaceIcons WORKSPACE_ICONS = WorkspaceIcons();
}

MantidDockWidget::MantidDockWidget(MantidUI *mui, ApplicationWindow *parent) :
  QDockWidget(tr("Workspaces"),parent), m_mantidUI(mui), m_updateCount( 0 ),
  m_treeUpdating(false), m_ads(Mantid::API::AnalysisDataService::Instance())
{
  setObjectName("exploreMantid"); // this is needed for QMainWindow::restoreState()
  setMinimumHeight(150);
  setMinimumWidth(200);
  parent->addDockWidget( Qt::RightDockWidgetArea, this );

  m_appParent = parent;

  QFrame *f = new QFrame(this);
  setWidget(f);

  m_tree = new MantidTreeWidget(this,m_mantidUI);
  m_tree->setHeaderLabel("Workspaces");

  FlowLayout * buttonLayout = new FlowLayout();
  m_loadButton = new QPushButton("Load");
  m_saveButton = new QPushButton("Save");
  m_deleteButton = new QPushButton("Delete");
  m_groupButton= new QPushButton("Group");
  m_sortButton= new QPushButton("Sort");

  if(m_groupButton)
    m_groupButton->setEnabled(false);
  m_deleteButton->setEnabled(false);
  m_saveButton->setEnabled(false);

  buttonLayout->addWidget(m_loadButton);
  buttonLayout->addWidget(m_deleteButton);
  buttonLayout->addWidget(m_groupButton);
  buttonLayout->addWidget(m_sortButton);
  buttonLayout->addWidget(m_saveButton);

  m_workspaceFilter = new MantidQt::MantidWidgets::LineEditWithClear();
  m_workspaceFilter->setPlaceholderText("Filter Workspaces");  
  m_workspaceFilter->setToolTip("Type here to filter the workspaces");  

  connect(m_workspaceFilter, SIGNAL(textChanged(const QString&)), this, SLOT(filterWorkspaceTree(const QString&)));

  QVBoxLayout * layout = new QVBoxLayout();
  f->setLayout(layout); 
  layout->setSpacing(0);
  layout->setMargin(0);
  layout->addLayout(buttonLayout);
  layout->addWidget(m_workspaceFilter);
  layout->addWidget(m_tree);

  m_loadMenu = new QMenu(this);
  m_saveMenu = new QMenu(this);

  QAction *loadFileAction = new QAction("File",this);
  QAction *liveDataAction = new QAction("Live Data",this);
  m_loadMapper = new QSignalMapper(this);
  m_loadMapper->setMapping(liveDataAction,"StartLiveData");
  m_loadMapper->setMapping(loadFileAction,"Load");
  connect(liveDataAction,SIGNAL(activated()), m_loadMapper, SLOT(map()));
  connect(loadFileAction,SIGNAL(activated()),m_loadMapper,SLOT(map()));
  connect(m_loadMapper, SIGNAL(mapped(const QString &)), m_mantidUI, SLOT(showAlgorithmDialog(const QString&)));
  m_loadMenu->addAction(loadFileAction);
  m_loadMenu->addAction(liveDataAction);
  m_loadButton->setMenu(m_loadMenu);

  // Dialog box used for user to specify folder to save multiple workspaces into
  m_saveFolderDialog = new QFileDialog;
  m_saveFolderDialog->setFileMode(QFileDialog::DirectoryOnly);
  m_saveFolderDialog->setOption(QFileDialog::ShowDirsOnly);

  // SET UP SORT
  createSortMenuActions();
  createWorkspaceMenuActions();

  connect(m_deleteButton,SIGNAL(clicked()),this,SLOT(deleteWorkspaces()));
  connect(m_tree,SIGNAL(itemClicked(QTreeWidgetItem*, int)),this,SLOT(clickedWorkspace(QTreeWidgetItem*, int)));
  connect(m_tree,SIGNAL(itemSelectionChanged()),this,SLOT(workspaceSelected()));
  connect(m_groupButton,SIGNAL(clicked()),this,SLOT(groupingButtonClick()));

  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));

  // call this slot directly after the signal is received. just increment the update counter
  connect(m_mantidUI, SIGNAL(workspace_renamed(QString,QString)), this, SLOT(recordWorkspaceRename(QString,QString)),Qt::DirectConnection);
  // call this slot directly after the signal is received. just increment the update counter
  connect(m_mantidUI, SIGNAL(ADS_updated()), this, SLOT(incrementUpdateCount()), Qt::DirectConnection);
  // this slot is called when the GUI thread is free. decrement the counter. do nothing until the counter == 0
  connect(m_mantidUI, SIGNAL(ADS_updated()), this, SLOT(updateTree()), Qt::QueuedConnection);

  connect(m_mantidUI, SIGNAL(workspaces_cleared()), m_tree, SLOT(clear()),Qt::QueuedConnection);
  connect(m_tree,SIGNAL(itemSelectionChanged()),this,SLOT(treeSelectionChanged()));
  connect(m_tree, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(populateChildData(QTreeWidgetItem*)));
  m_tree->setDragEnabled(true);
}

MantidDockWidget::~MantidDockWidget()
{
}

/** Returns the name of the selected workspace
*  (the first one if more than one is selected)
*/
QString MantidDockWidget::getSelectedWorkspaceName() const
{
  QList<QTreeWidgetItem*> items = m_tree->selectedItems();
  QString str("");
  if( !items.empty() )
  {
    QTreeWidgetItem *item = items[0];
    if (item) str = item->text(0);
  }
  return str;
}

/// Returns a pointer to the selected workspace (the first if multiple workspaces selected)
Mantid::API::Workspace_sptr MantidDockWidget::getSelectedWorkspace() const
{
  QString workspaceName = getSelectedWorkspaceName();
  if (m_ads.doesExist(workspaceName.toStdString()))
  {
    return m_ads.retrieve(workspaceName.toStdString());
  }
  else
  {
    return Mantid::API::Workspace_sptr();
  }
}

/**
* Create the action items associated with the dock
*/
void MantidDockWidget::createWorkspaceMenuActions()
{
  m_showData = new QAction(tr("Show Data"),this);
  connect(m_showData,SIGNAL(triggered()),m_mantidUI,SLOT(importWorkspace()));

  m_showInst = new QAction(tr("Show Instrument"),this);
  connect(m_showInst,SIGNAL(triggered()),m_mantidUI,SLOT(showMantidInstrumentSelected()));

  m_plotSpec = new QAction(tr("Plot Spectrum..."),this);
  connect(m_plotSpec,SIGNAL(triggered()),this,SLOT(plotSpectra()));

  m_plotSpecErr = new QAction(tr("Plot Spectrum with Errors..."),this);
  connect(m_plotSpecErr,SIGNAL(triggered()),this,SLOT(plotSpectraErr()));

  m_colorFill = new QAction(tr("Color Fill Plot"), this);
  connect(m_colorFill, SIGNAL(triggered()), this, SLOT(drawColorFillPlot()));

  m_showDetectors = new QAction(tr("Show Detectors"),this);
  connect(m_showDetectors,SIGNAL(activated()),this,SLOT(showDetectorTable()));

  m_showBoxData = new QAction(tr("Show Box Data Table"),this);
  connect(m_showBoxData,SIGNAL(activated()),m_mantidUI,SLOT(importBoxDataTable()));

  m_showVatesGui = new QAction(tr("Show Vates Simple Interface"), this);
  { QIcon icon; icon.addFile(QString::fromUtf8(":/VatesSimpleGuiViewWidgets/icons/pvIcon.png"), QSize(), QIcon::Normal, QIcon::Off);
  m_showVatesGui->setIcon(icon); }
  connect(m_showVatesGui, SIGNAL(activated()), m_mantidUI, SLOT(showVatesSimpleInterface()));

  m_showMDPlot = new QAction(tr("Plot MD"), this);
  connect(m_showMDPlot, SIGNAL(activated()), m_mantidUI, SLOT(showMDPlot()));

  m_showListData = new QAction(tr("List Data"), this);
  connect(m_showListData, SIGNAL(activated()), m_mantidUI, SLOT(showListData())); 

  m_showSpectrumViewer = new QAction(tr("Show Spectrum Viewer"), this);
  connect(m_showSpectrumViewer, SIGNAL(activated()), m_mantidUI, SLOT(showSpectrumViewer()));

  m_showSliceViewer = new QAction(tr("Show Slice Viewer"), this);
  { QIcon icon; icon.addFile(QString::fromUtf8(":/SliceViewer/icons/SliceViewerWindow_icon.png"), QSize(), QIcon::Normal, QIcon::Off);
  m_showSliceViewer->setIcon(icon); }
  connect(m_showSliceViewer, SIGNAL(activated()), m_mantidUI, SLOT(showSliceViewer()));

  m_showLogs = new QAction(tr("Sample Logs..."), this);
  connect(m_showLogs,SIGNAL(triggered()),m_mantidUI,SLOT(showLogFileWindow()));

  m_showSampleMaterial = new QAction(tr("Sample Material..."), this);
  connect(m_showSampleMaterial,SIGNAL(triggered()),m_mantidUI,SLOT(showSampleMaterialWindow()));

  m_showHist = new QAction(tr("Show History"), this);
  connect(m_showHist,SIGNAL(triggered()),m_mantidUI,SLOT(showAlgorithmHistory()));

  m_saveNexus = new QAction(tr("Save Nexus"),this);
  connect(m_saveNexus,SIGNAL(activated()),m_mantidUI,SLOT(saveNexusWorkspace()));

  m_rename = new QAction(tr("Rename"),this);
  connect(m_rename,SIGNAL(activated()),this,SLOT(renameWorkspace()));

  m_delete = new QAction(tr("Delete"),this);
  connect(m_delete,SIGNAL(triggered()),this,SLOT(deleteWorkspaces()));

  m_showTransposed = new QAction(tr("Show Transposed"),this);
  connect(m_showTransposed,SIGNAL(triggered()),m_mantidUI,SLOT(importTransposed()));

  m_convertToMatrixWorkspace = new QAction(tr("Convert to MatrixWorkspace"),this);
  m_convertToMatrixWorkspace->setIcon(QIcon(getQPixmap("mantid_matrix_xpm")));
  connect(m_convertToMatrixWorkspace,SIGNAL(triggered()),this,SLOT(convertToMatrixWorkspace()));

  m_convertMDHistoToMatrixWorkspace = new QAction(tr("Convert to MatrixWorkspace"),this);
  m_convertMDHistoToMatrixWorkspace->setIcon(QIcon(getQPixmap("mantid_matrix_xpm")));
  connect(m_convertMDHistoToMatrixWorkspace,SIGNAL(triggered()),this,SLOT(convertMDHistoToMatrixWorkspace()));

  m_clearUB = new QAction(tr("Clear UB Matrix"), this);
  connect(m_clearUB, SIGNAL(activated()), this, SLOT(clearUB()));
}

/**
* Create actions for sorting.
*/
void MantidDockWidget::createSortMenuActions()
{
  chooseByName();
  m_sortMenu = new QMenu(this);

  QAction* m_ascendingSortAction = new QAction("Ascending", this);
  QAction* m_descendingSortAction = new QAction("Descending", this);
  QAction* m_byNameChoice = new QAction("Name", this);
  QAction* m_byLastModifiedChoice = new QAction("Last Modified", this);

  m_ascendingSortAction->setCheckable(true);
  m_ascendingSortAction->setEnabled(true);
  m_ascendingSortAction->setToggleAction(true);

  m_descendingSortAction->setCheckable(true);
  m_descendingSortAction->setEnabled(true);
  m_descendingSortAction->setToggleAction(true);

  QActionGroup *sortDirectionGroup = new QActionGroup(m_sortMenu);
  sortDirectionGroup->addAction(m_ascendingSortAction);
  sortDirectionGroup->addAction(m_descendingSortAction);
  sortDirectionGroup->setExclusive(true);
  m_ascendingSortAction->setChecked(true);

  m_byNameChoice->setCheckable(true);
  m_byNameChoice->setEnabled(true);
  m_byNameChoice->setToggleAction(true);

  m_byLastModifiedChoice->setCheckable(true);
  m_byLastModifiedChoice->setEnabled(true);
  m_byLastModifiedChoice->setToggleAction(true);

  m_sortChoiceGroup = new QActionGroup(m_sortMenu);
  m_sortChoiceGroup->addAction(m_byNameChoice);
  m_sortChoiceGroup->addAction(m_byLastModifiedChoice);
  m_sortChoiceGroup->setExclusive(true);
  m_byNameChoice->setChecked(true);

  connect(m_ascendingSortAction, SIGNAL(activated()), this, SLOT(sortAscending()));
  connect(m_descendingSortAction, SIGNAL(activated()), this, SLOT(sortDescending()));
  connect(m_byNameChoice, SIGNAL(activated()), this, SLOT(chooseByName()));
  connect(m_byLastModifiedChoice, SIGNAL(activated()), this, SLOT(chooseByLastModified()));

  m_sortMenu->addActions(sortDirectionGroup->actions());
  m_sortMenu->addSeparator();
  m_sortMenu->addActions(m_sortChoiceGroup->actions());
  m_sortButton->setMenu(m_sortMenu);
}

/**
* When an item is expanded, populate the child data for this item
* @param item :: The item being expanded
*/
void MantidDockWidget::populateChildData(QTreeWidgetItem* item)
{
  QVariant userData = item->data(0, Qt::UserRole);
  if ( userData.isNull() ) return;

  // Clear it first
  while( item->childCount() > 0 )
  {
    auto * widgetItem = item->takeChild(0);
    delete widgetItem;
  }

  Workspace_sptr workspace = userData.value<Workspace_sptr>();

  if(auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace))
  {
    const size_t nmembers = group->getNumberOfEntries();
    for(size_t i = 0; i < nmembers; ++i)
    {
      auto ws = group->getItem(i);
      auto * node = addTreeEntry(std::make_pair(ws->name(), ws), item);
      excludeItemFromSort(node);
      if (shouldBeSelected(node->text(0))) node->setSelected(true);
    }
  }
  else
  {
    QString details;
    try
    {
      details = workspace->toString().c_str();
    }
    catch(std::runtime_error& e)
    {
      details = QString("Error: %1").arg(e.what());
    }
    QStringList rows = details.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    rows.append(QString("Memory used: ") + workspace->getMemorySizeAsStr().c_str());

    auto iend = rows.constEnd();
    for(auto itr = rows.constBegin(); itr != iend; ++itr)
    {
      MantidTreeWidgetItem *data = new MantidTreeWidgetItem(QStringList(*itr), m_tree);
      data->setFlags(Qt::NoItemFlags);
      excludeItemFromSort(data);
      item->addChild(data);
    }
  }
}

/**
* Set tree item's icon based on the ID of the workspace.
* @param item :: A workspace tree item.
* @param wsID :: An icon type code.
*/
void MantidDockWidget::setItemIcon(QTreeWidgetItem *item, const std::string & wsID)
{
  try
  {
    item->setIcon(0, QIcon(WORKSPACE_ICONS.getIcon(wsID)));
  }
  catch(std::runtime_error&)
  {
    docklog.warning() << "Cannot find icon for workspace ID '" << wsID << "'\n";
  }
}

/**
* Update the workspace tree to match the current state of the ADS.
* It is important that the workspace tree is modified only by this method.
*/
void MantidDockWidget::updateTree()
{
  // do not update until the counter is zero
  if ( m_updateCount.deref() ) return;

  // find all expanded top-level entries
  QStringList expanded;
  int n = m_tree->topLevelItemCount();
  for(int i = 0; i < n; ++i)
  {
    auto item = m_tree->topLevelItem(i);
    if ( item->isExpanded() )
    {
      expanded << item->text(0);
    }
  }

  // create a new tree
  setTreeUpdating(true);
  populateTopLevel(m_ads.topLevelItems(), expanded);
  setTreeUpdating(false);

  // Re-sort
  m_tree->sort();
}

/**
* Slot to be connected directly to ADS_updated signal. Increase m_updateCount and return.
*/
void MantidDockWidget::incrementUpdateCount()
{
  m_updateCount.ref();
}

/**
* Save the old and the new name in m_renameMap. This is needed to restore selection
*   of the renamed workspace (if it was selected before renaming).
* @param old_name :: Old name of a renamed workspace.
* @param new_name :: New name of a renamed workspace.
*/
void MantidDockWidget::recordWorkspaceRename(QString old_name, QString new_name)
{
  // check if old_name has been recently a new name
  QList<QString> oldNames = m_renameMap.keys(old_name);
  // non-empty list of oldNames become new_name
  if ( !oldNames.isEmpty() )
  {
    foreach(QString name, oldNames)
    {
      m_renameMap[name] = new_name;
    }
  }
  else
  {
    // record a new rename pair
    m_renameMap[old_name] = new_name;
  }
}

/**
* Flips the flag indicating whether a tree update is in progress. Actions such as sorting
* are disabled while an update is in progress.
* @param state The required state for the flag
*/
void MantidDockWidget::setTreeUpdating(const bool state)
{
  m_treeUpdating = state;
}

/**
* Clears the tree and re-populates it with the given top level items
* @param topLevelItems The map of names to workspaces
* @param expanded Names of items who should expanded after being populated
*/
void MantidDockWidget::populateTopLevel(const std::map<std::string,Mantid::API::Workspace_sptr> & topLevelItems,
                                        const QStringList & expanded)
{
  // collect names of selected workspaces
  QList<QTreeWidgetItem *> selected = m_tree->selectedItems();
  m_selectedNames.clear(); // just in case
  foreach( QTreeWidgetItem *item, selected)
  {
    m_selectedNames << item->text(0);
  }

  // populate the tree from scratch
  m_tree->clear();
  auto iend = topLevelItems.end();
  for(auto it = topLevelItems.begin(); it != iend; ++it)
  {
    auto *node = addTreeEntry(*it);
    QString name = node->text(0);
    if(expanded.contains(name)) node->setExpanded(true);
    // see if item must be selected
    if ( shouldBeSelected(name) ) node->setSelected(true);
  }
  m_selectedNames.clear();
  m_renameMap.clear();

  //apply any filtering
  filterWorkspaceTree(m_workspaceFilter->text());
}

/**
* Adds a node for the given named item, including a single child ID item to make each node have a expandable button
* and allowing plotting to work from non-expanded items
* @param item A name/workspace pair to add.
* @param parent If not null then add the new items as a child of the given item
*/
MantidTreeWidgetItem * MantidDockWidget::addTreeEntry(const std::pair<std::string,Mantid::API::Workspace_sptr> & item, QTreeWidgetItem* parent)
{
  MantidTreeWidgetItem *node = new MantidTreeWidgetItem(QStringList(item.first.c_str()), m_tree);
  node->setData(0,Qt::UserRole, QVariant::fromValue(item.second));

  // A a child ID item so that it becomes expandable. Using the correct ID is needed when plotting from non-expanded groups.
  const std::string wsID = item.second->id();
  MantidTreeWidgetItem *idNode = new MantidTreeWidgetItem(QStringList(wsID.c_str()), m_tree);
  idNode->setFlags(Qt::NoItemFlags);
  node->addChild(idNode);
  setItemIcon(node,wsID);

  if(parent)
  {
    parent->addChild(node);
  }
  else
  {
    m_tree->addTopLevelItem(node);
  }
  return node;
}

/**
* Check if a workspace should be selected after dock update.
* @param name :: Name of a workspace to check.
*/
bool MantidDockWidget::shouldBeSelected(QString name) const
{
  QStringList renamed = m_renameMap.keys(name);
  if ( !renamed.isEmpty() )
  {
    foreach(QString oldName,renamed)
    {
      if ( m_selectedNames.contains(oldName) )
      {
        return true;
      }
    }
  }
  else if(m_selectedNames.contains(name))
  {
    return true;
  }
  return false;
}

/**
* Add the actions that are appropriate for a MatrixWorkspace
* @param menu :: The menu to store the items
* @param matrixWS :: The workspace related to the menu
*/
void MantidDockWidget::addMatrixWorkspaceMenuItems(QMenu *menu, const Mantid::API::MatrixWorkspace_const_sptr & matrixWS) const
{
  // Add all options except plot of we only have 1 value
  menu->addAction(m_showData);
  menu->addAction(m_showInst);
  // Disable the 'show instrument' option if a workspace doesn't have an instrument attached
  m_showInst->setEnabled( matrixWS->getInstrument() && !matrixWS->getInstrument()->getName().empty() );
  menu->addSeparator();
  menu->addAction(m_plotSpec);
  menu->addAction(m_plotSpecErr);

  // Don't plot a spectrum if only one X value
  m_plotSpec->setEnabled ( matrixWS->blocksize() > 1 );
  m_plotSpecErr->setEnabled ( matrixWS->blocksize() > 1 );

  menu->addAction(m_showSpectrumViewer); // The 2D spectrum viewer

  menu->addAction(m_colorFill);
  // Show the color fill plot if you have more than one histogram
  m_colorFill->setEnabled( ( matrixWS->axes() > 1 && matrixWS->getNumberHistograms() > 1) );
  menu->addAction(m_showSliceViewer); // The 2D slice viewer
  menu->addSeparator();
  menu->addAction(m_showDetectors);
  menu->addAction(m_showLogs);
  menu->addAction(m_showSampleMaterial);
  menu->addAction(m_showHist);
  menu->addAction(m_saveNexus);
}

/**
* Add the actions that are appropriate for a MDEventWorkspace
* @param menu :: The menu to store the items
* @param WS :: The workspace related to the menu
*/
void MantidDockWidget::addMDEventWorkspaceMenuItems(QMenu *menu, const Mantid::API::IMDEventWorkspace_const_sptr & WS) const
{
  Q_UNUSED(WS);

  //menu->addAction(m_showBoxData); // Show MD Box data (for debugging only)
  menu->addAction(m_showVatesGui); // Show the Vates simple interface
  if (!MantidQt::API::InterfaceManager::hasVatesLibraries())
  {
    m_showVatesGui->setEnabled(false);
  }
  else
  {
    std::size_t nDim = WS->getNonIntegratedDimensions().size();
    m_showVatesGui->setEnabled(nDim >= 3 && nDim < 5);
  }
  menu->addAction(m_showSliceViewer); // The 2D slice viewer
  menu->addAction(m_showHist);  // Algorithm history
  menu->addAction(m_showListData); // Show data in table
  menu->addAction(m_showLogs);
  menu->addAction(m_showSampleMaterial); //TODO
}

void MantidDockWidget::addMDHistoWorkspaceMenuItems(QMenu *menu, const Mantid::API::IMDWorkspace_const_sptr &WS) const
{
  Q_UNUSED(WS);
  menu->addAction(m_showHist); // Algorithm history
  menu->addAction(m_showVatesGui); // Show the Vates simple interface
  if (!MantidQt::API::InterfaceManager::hasVatesLibraries())
  {
    m_showVatesGui->setEnabled(false);
  }
  else
  {
    std::size_t nDim = WS->getNonIntegratedDimensions().size();
    m_showVatesGui->setEnabled(nDim >= 3 && nDim < 5);
  }
  menu->addAction(m_showSliceViewer); // The 2D slice viewer
  menu->addAction(m_showMDPlot); // A plot of intensity vs bins
  menu->addAction(m_showListData); // Show data in table
  menu->addAction(m_convertMDHistoToMatrixWorkspace);
  menu->addAction(m_showLogs);
  menu->addAction(m_showSampleMaterial); //TODO
}


/** Add the actions that are appropriate for a PeaksWorkspace
* @param menu :: The menu to store the items
* @param WS :: The workspace related to the menu
*/
void MantidDockWidget::addPeaksWorkspaceMenuItems(QMenu *menu, const Mantid::API::IPeaksWorkspace_const_sptr &WS) const
{
  Q_UNUSED(WS);
  menu->addAction(m_showData);
  menu->addAction(m_showVatesGui); // Show the Vates simple interface
  if (!MantidQt::API::InterfaceManager::hasVatesLibraries())
  {
    m_showVatesGui->setEnabled(false);
  }
  menu->addSeparator();
  menu->addAction(m_showDetectors);
  menu->addAction(m_showHist);
}

/**
* Add the actions that are appropriate for a MatrixWorkspace
* @param menu :: The menu to store the items
*/
void MantidDockWidget::addWorkspaceGroupMenuItems(QMenu *menu) const
{
  m_plotSpec->setEnabled(true);
  menu->addAction(m_plotSpec);
  m_plotSpecErr->setEnabled(true);
  menu->addAction(m_plotSpecErr);
  menu->addAction(m_colorFill);
  m_colorFill->setEnabled(true);
  menu->addSeparator();
  menu->addAction(m_saveNexus);
}

/**
* Add the actions that are appropriate for a MatrixWorkspace
* @param menu :: The menu to store the items
*/
void MantidDockWidget::addTableWorkspaceMenuItems(QMenu * menu) const
{
  menu->addAction(m_showData);
  menu->addAction(m_showTransposed);
  menu->addAction(m_showHist);
  menu->addAction(m_saveNexus);
  menu->addAction(m_convertToMatrixWorkspace);
}

/**
* Add menu for clearing workspace items.
* @param menu : Parent menu.
* @param wsName : Name of the selected workspace.
*/
void MantidDockWidget::addClearMenuItems(QMenu* menu, const QString& wsName)
{
  QMenu* clearMenu = new QMenu(tr("Clear Options"), this);

  m_clearUB->setEnabled( m_mantidUI->hasUB(wsName) );

  clearMenu->addAction(m_clearUB);
  menu->addMenu(clearMenu);
}

/**
* Filter workspaces based on the string provided
* @param text : the string to filter on.
*/
void MantidDockWidget::filterWorkspaceTree(const QString &text)
{
  const QString filterText = text.stripWhiteSpace();
  QRegExp filterRegEx (filterText,false);



  //show all items
  QTreeWidgetItemIterator it(m_tree);
  while (*it) 
  {
    (*it)->setHidden(false);
    ++it;
  }

  int hiddenCount = 0;
  QList<QTreeWidgetItem*> visibleGroups;
  if (!filterText.isEmpty())
  {


    //Loop over everything (currently loaded) and top level
    //find out what is already expanded
    QStringList expanded;
    int n = m_tree->topLevelItemCount();
    for(int i = 0; i < n; ++i)
    {
      auto item = m_tree->topLevelItem(i);
      if ( item->isExpanded() )
      {
        expanded << item->text(0);
      }
      else  
      {
        //expand everything that is at the top level (as we lazy load this is required)
        item->setExpanded(true);
      }
    }

    //filter based on the string
    QTreeWidgetItemIterator it(m_tree,QTreeWidgetItemIterator::All);
    while (*it) 
    {
      QTreeWidgetItem *item = (*it);
      QVariant userData = item->data(0, Qt::UserRole);

      if (!userData.isNull() ) 
      {
        Workspace_sptr workspace = userData.value<Workspace_sptr>();
        if (workspace)
        {
          //I am a workspace
          if (item->text(0).contains(filterRegEx))
          {
            //my name does match the filter
            if(auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace))
            {
              //I am a group, I will want my children to be visible
              //but I cannot do that until this iterator has finished
              //store this pointer in a list for processing later
              visibleGroups.append(item);
              item->setHidden(false);
            }

            if (item->parent() == NULL)
            {
              // No parent, I am a top level workspace - show me
              item->setHidden(false);
            }
            else
            {
              // I am a child workspace of a group
              // I match, so I want my parent to remain visible as well.
              item->setHidden(false);
              if (item->parent()->isHidden())
              {
                //I was previously hidden, show me and set to be expanded
                --hiddenCount;
                item->parent()->setHidden(false);
                expanded << item->parent()->text(0);
              }
            }
          }
          else
          {
            //my name does not match the filter - hide me
            item->setHidden(true);
            ++hiddenCount;
          }
        }
      }
      ++it;
    }

    //make children of visible groups visible
    for (auto itGroup = visibleGroups.begin(); itGroup != visibleGroups.end(); ++itGroup)
    {
      QTreeWidgetItem *group = (*itGroup);  
      for (int i = 0; i < group->childCount(); i++)
      {
        QTreeWidgetItem *child = group->child(i); 
        if (child->isHidden())
        {
          //I was previously hidden, show me
          --hiddenCount;
          child->setHidden(false);
        }
      }
    }

    //set the expanded state
    for(int i = 0; i < n; ++i)
    {
      auto item = m_tree->topLevelItem(i);
      item->setExpanded(expanded.contains(item->text(0)));
    }

  }

  //display a message if items are hidden
  if (hiddenCount > 0)
  {
    QString headerString = QString("Workspaces (%1 filtered)").arg(QString::number(hiddenCount));
    m_tree->headerItem()->setText(0,headerString);
  }
  else
  {
    m_tree->headerItem()->setText(0,"Workspaces");
  }
}


void MantidDockWidget::clickedWorkspace(QTreeWidgetItem* item, int)
{
  Q_UNUSED(item);
}

void MantidDockWidget::workspaceSelected()
{ 
  QList<QTreeWidgetItem*> selectedItems=m_tree->selectedItems();
  if(selectedItems.isEmpty()) return;

  // If there are multiple workspaces selected group and save as Nexus
  if(selectedItems.length() > 1)
  {
    connect(m_saveButton, SIGNAL(clicked()), this, SLOT(saveWorkspaceGroup()));

    // Don't display as a group
    m_saveButton->setMenu(NULL);
  }
  else
  {
    // Don't run the save group function when clicked
    disconnect(m_saveButton, SIGNAL(clicked()), this, SLOT(saveWorkspaceGroup()));

    // Remove all existing save algorithms from list
    m_saveMenu->clear();

    // Add some save algorithms
    addSaveMenuOption("SaveNexus", "Nexus");
    addSaveMenuOption("SaveAscii", "ASCII");
    addSaveMenuOption("SaveAscii.1", "ASCII v1");

    // Set the button to show the menu
    m_saveButton->setMenu(m_saveMenu);
  }

  QString wsName=selectedItems[0]->text(0);
  if(m_ads.doesExist(wsName.toStdString()))
  {
    m_mantidUI->enableSaveNexus(wsName);
  }
}

/**
 * Adds an algorithm to the save menu.
 *
 * @param algorithmString Algorithm string in format ALGO_NAME.VERSION or ALGO_NAME
 * @param menuEntryName Text to be shown in menu
 */
void MantidDockWidget::addSaveMenuOption(QString algorithmString, QString menuEntryName)
{
  // Default to algo string if no entry name given
  if(menuEntryName.isEmpty())
    menuEntryName = algorithmString;

  // Create the action and add data
  QAction *saveAction = new QAction(menuEntryName, this);
  saveAction->setData(QVariant(algorithmString));

  // Connect the tigger slot to show algorithm dialog
  connect(saveAction, SIGNAL(triggered()), this, SLOT(handleShowSaveAlgorithm()));

  // Add it to the menu
  m_saveMenu->addAction(saveAction);
}

/**
 * Save all selected workspaces
 */
void MantidDockWidget::saveWorkspaceGroup()
{
  QList<QTreeWidgetItem*> items = m_tree->selectedItems();
  if(items.size() < 2)
    return;

  m_saveFolderDialog->setWindowTitle("Select save folder");
  m_saveFolderDialog->setLabelText(QFileDialog::Accept, "Select");
  m_saveFolderDialog->open(this, SLOT(saveWorkspacesToFolder(const QString &)));
}

/**
 * Handler for the directory browser being closed when selecting save on multiple workspaces
 *
 * @param folder Path to folder to save workspaces in
 */
void MantidDockWidget::saveWorkspacesToFolder(const QString &folder)
{
  QList<QTreeWidgetItem*> items = m_tree->selectedItems();

  // Loop through multiple items selected from the mantid tree
  QList<QTreeWidgetItem*>::iterator itr=items.begin();
  for (itr = items.begin(); itr != items.end(); ++itr)
  {
    QString workspaceName = (*itr)->text(0);
    QString filename = folder + "/" + workspaceName + ".nxs";

    IAlgorithm_sptr saveAlg = AlgorithmManager::Instance().create("SaveNexus");
    saveAlg->initialize();
    try
    {
      saveAlg->setProperty("InputWorkspace", workspaceName.toStdString());
      saveAlg->setProperty("Filename", filename.toStdString());
      saveAlg->execute();
    }
    catch(std::runtime_error &rte)
    {
      docklog.error() << "Error saving workspace " << workspaceName.toStdString()
        << ": " << rte.what() << std::endl;
    }
  }
}

/**
 * Handles a save algorithm being triggered by the Save menu.
 *
 * To select a specific algorithm add a QString to the data of the QAction
 * in the form ALGORITHM_NAME.VERSION or just ALGORITHM_NAME to use the
 * most recent version.
 */
void MantidDockWidget::handleShowSaveAlgorithm()
{
  QAction *sendingAction = dynamic_cast<QAction *>(sender());

  if(sendingAction)
  {
    QString wsName = getSelectedWorkspaceName();
    QVariant data = sendingAction->data();

    if(data.canConvert<QString>())
    {
      QString algorithmName;
      int version = -1;

      QStringList splitData = data.toString().split(".");
      switch(splitData.length())
      {
        case 2:
          version = splitData[1].toInt();
        case 1:
          algorithmName = splitData[0];
          break;
        default:
          m_mantidUI->saveNexusWorkspace();
          return;
      }

      QHash<QString,QString> presets;
      if(!wsName.isEmpty())
        presets["InputWorkspace"] = wsName;

      m_mantidUI->showAlgorithmDialog(algorithmName, presets, NULL, version);
      return;
    }
  }

  // If we can't get the type of algorithm this should be we can always fall back on Nexus
  m_mantidUI->saveNexusWorkspace();
}

/**
deleteWorkspaces
*/
void MantidDockWidget::deleteWorkspaces()
{
  QList<QTreeWidgetItem*> items = m_tree->selectedItems();
  MantidMatrix* m = dynamic_cast<MantidMatrix*>(m_mantidUI->appWindow()->activeWindow());
   
  bool deleteExplorer = false;
  bool deleteActive = false;

  if((m_deleteButton->hasFocus() || m_tree->hasFocus()) && !items.empty())
  {
    deleteExplorer = true;
  }
  if((m && m->isA("MantidMatrix")) && (!m->workspaceName().isEmpty() && m_ads.doesExist(m->workspaceName().toStdString())))
  {
    deleteActive = true;
  }

  if(deleteActive || deleteExplorer)
  {    
    QMessageBox::StandardButton reply;
    
    if(m_appParent->isDeleteWorkspacePromptEnabled())
    {
      reply = QMessageBox::question(this, "Delete Workspaces", "Are you sure you want to delete the selected Workspaces?\n\nThis prompt can be disabled from:\nPreferences->General->Confirmations",
                                    QMessageBox::Yes|QMessageBox::No);
    }
    else
    {
      reply = QMessageBox::Yes;
    }

    if (reply == QMessageBox::Yes)
    {
      if(deleteExplorer)
      { 
        //loop through multiple items selected from the mantid tree
        QList<QTreeWidgetItem*>::iterator itr=items.begin();
        for (itr = items.begin(); itr != items.end(); ++itr)
        {
          //Sometimes we try to delete a workspace that's already been deleted.
          if(m_ads.doesExist((*itr)->text(0).toStdString()))
            m_mantidUI->deleteWorkspace((*itr)->text(0));
        }//end of for loop for selected items
      }
      else if(deleteActive)
      {
        m_mantidUI->deleteWorkspace(m->workspaceName());
      }
    }
  }
}

void MantidDockWidget::sortAscending()
{
  if(isTreeUpdating()) return;
  m_tree->setSortOrder(Qt::Ascending);
  m_tree->sort();
}

void MantidDockWidget::sortDescending()
{
  if(isTreeUpdating()) return;
  m_tree->setSortOrder(Qt::Descending);
  m_tree->sort();
}

void MantidDockWidget::chooseByName()
{
  if(isTreeUpdating()) return;
  m_tree->setSortScheme(ByName);
  m_tree->sort();
}

void MantidDockWidget::chooseByLastModified()
{
  if(isTreeUpdating()) return;
  m_tree->setSortScheme(ByLastModified);
  m_tree->sort();
}

void MantidDockWidget::excludeItemFromSort(MantidTreeWidgetItem *item)
{
  static int counter = 1;

  item->setSortPos( counter );

  counter++;
}

/**
* Saves a workspace based on the program the user chooses to save to.
* @param name :: A string containing the name of the program
*/

void MantidDockWidget::saveToProgram(const QString & name)
{
  //Create a map for the keys and details to go into
  std::map<std::string,std::string> programKeysAndDetails;
  programKeysAndDetails["name"] = name.toStdString();

  //Get a list of the program detail keys (mandatory - target, saveusing) (optional - arguments, save parameters, workspace type)
  std::vector<std::string> programKeys = (Mantid::Kernel::ConfigService::Instance().getKeys(("workspace.sendto." + programKeysAndDetails.find("name")->second)));

  for (size_t i=0; i<programKeys.size(); i++)
  {
    //Assign a key to its value using the map
    programKeysAndDetails[programKeys[i]] = (Mantid::Kernel::ConfigService::Instance().getString(("workspace.sendto." + programKeysAndDetails.find("name")->second + "." + programKeys[i])));
  }

  //Check to see if mandatory information is included
  if ((programKeysAndDetails.count("name") != 0) && (programKeysAndDetails.count("target") != 0) && (programKeysAndDetails.count("saveusing") != 0))    
  {
    std::string expTarget = Poco::Path::expand(programKeysAndDetails.find("target")->second);

    QFileInfo target = QString::fromStdString(expTarget);
    if(target.exists())
    {
      try
      {
        //Setup a shared pointer for the algorithm using the appropriate save type
        Mantid::API::IAlgorithm_sptr alg;

        //Convert to QString and create Algorithm
        QString saveUsing = QString::fromStdString(programKeysAndDetails.find("saveusing")->second);

        //Create a new save based on what files the new program can open
        alg = m_mantidUI->createAlgorithm(saveUsing);

        //Get the file extention based on the workspace
        Property* prop = alg->getProperty("Filename");
        FileProperty *fileProp = dynamic_cast<FileProperty*>(prop);
        std::string ext;
        if(fileProp)
        {
          ext = fileProp->getDefaultExt();
        }

        //Save as.. default save + the file type i.e .nxs
        alg->setPropertyValue("fileName", "auto_save_" + selectedWsName.toStdString() + ext);

        //Save the workspace
        alg->setPropertyValue("InputWorkspace", selectedWsName.toStdString());

        //If there are any save parameters
        if (programKeysAndDetails.count("saveparameters") != 0)
        {
          QString saveParametersGrouped = QString::fromStdString(programKeysAndDetails.find("saveparameters")->second);
          QStringList saveParameters = saveParametersGrouped.split(',');

          //For each one found split it up and assign the parameter
          for (int i = 0; i<saveParameters.size(); i++)
          {
            QStringList sPNameAndDetail = saveParameters[i].split('=');
            std::string saveParameterName = sPNameAndDetail[0].trimmed().toStdString();
            std::string saveParameterDetail = sPNameAndDetail[1].trimmed().toStdString();
            if(saveParameterDetail == "True")
              alg->setProperty(saveParameterName, true);
            else if(saveParameterDetail == "False")
              alg->setProperty(saveParameterName, false);
            else  //if not true or false then must be a value
            {
              alg->setPropertyValue(saveParameterName, saveParameterDetail);
            }
          }
        }

        //Execute the save
        m_mantidUI->executeAlgorithmAsync(alg, true);
        //alg->execute();

        //Get the save location of the file (should be default Mantid folder)
        //std::string savedFile = alg->getProperty("Filename");
        QString savedFile = QString::fromStdString(alg->getProperty("Filename"));
        QStringList arguments;

        //Arguments for the program to take. Default will be the file anyway.
        if (programKeysAndDetails.count("arguments") != 0)
        {
          QString temp = QString::fromStdString(programKeysAndDetails.find("arguments")->second);
          temp.replace(QString("[file]"), savedFile);
          //temp.replace(QString("[user]"), user;
          arguments = temp.split(",");
        }
        else
          arguments.insert(0, savedFile);

        //convert the list into a standard vector for compatibility with Poco
        std::vector<std::string> argumentsV;

        for (int i = 0; i<arguments.size(); i++)
        {
          argumentsV.assign(1, (arguments[i].toStdString()));
        }

        //Execute the program
        try
        {
          Mantid::Kernel::ConfigService::Instance().launchProcess(expTarget, argumentsV);
        }
        catch(std::runtime_error&)
        {
          QMessageBox::information(this, "Error", "User tried to open program from: " + QString::fromStdString(expTarget) + " There was an error opening the program. Please check the target and arguments list to ensure that these are correct");
        }
      }
      catch(std::exception&)
      {
        QMessageBox::information(this, "Mantid - Send to Program", "A file property wasn't found. Please check that the correct"
          + QString("save algorithm was used.\n(View -> Preferences -> Mantid -> SendTo -> Edit -> SaveUsing)") );
      }
    }
    else
      QMessageBox::information(this, "Target Path Error", "User tried to open program from: " + QString::fromStdString(expTarget) + " The target file path for the program can't be found. Please check that the full path is correct");
  }
}


void MantidDockWidget::renameWorkspace()
{
  //get selected workspace
  QList<QTreeWidgetItem*>selectedItems=m_tree->selectedItems();
  QStringList selectedwsNames;
  if(!selectedItems.empty())
  {
    for(int i=0; i < selectedItems.size(); ++i)
    {
      selectedwsNames.append(selectedItems[i]->text(0));
    }
  }
  m_mantidUI->renameWorkspace(selectedwsNames);
}

void MantidDockWidget::showDetectorTable()
{
  //get selected workspace
  QList<QTreeWidgetItem*>selectedItems=m_tree->selectedItems();
  QString selctedwsName;
  if(!selectedItems.empty())
  {
    selctedwsName=selectedItems[0]->text(0);
  }
  m_mantidUI->createDetectorTable(selctedwsName,std::vector<int>());
}

void MantidDockWidget::popupMenu(const QPoint & pos)
{
  QTreeWidgetItem* treeItem = m_tree->itemAt(pos);
  selectedWsName = "";
  if( treeItem ) selectedWsName = treeItem->text(0);
  else m_tree->selectionModel()->clear();
  QMenu *menu(NULL);

  //If no workspace is here then have load raw and dae
  if( selectedWsName.isEmpty() )
  {
    menu = m_loadMenu;
  }
  //else show instrument, sample logs and delete
  else
  {
    // Fresh menu
    menu = new QMenu(this);
    menu->setObjectName("WorkspaceContextMenu");
    Mantid::API::Workspace_const_sptr ws;
    try
    {
      ws = m_ads.retrieve(selectedWsName.toStdString());
    }
    catch(Mantid::Kernel::Exception::NotFoundError &)
    {
      // Nothing to do
      return;
    }

    // Add the items that are appropriate for the type
    if( MatrixWorkspace_const_sptr matrixWS = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(ws) ) 
    {
      addMatrixWorkspaceMenuItems(menu, matrixWS);
    }
    else if( IMDEventWorkspace_const_sptr mdeventWS = boost::dynamic_pointer_cast<const IMDEventWorkspace>(ws) )
    {
      addMDEventWorkspaceMenuItems(menu, mdeventWS);
    }
    else if( IMDWorkspace_const_sptr mdWS = boost::dynamic_pointer_cast<const IMDWorkspace>(ws) )
    {
      addMDHistoWorkspaceMenuItems(menu, mdWS);
    }
    else if( IPeaksWorkspace_const_sptr peaksWS = boost::dynamic_pointer_cast<const IPeaksWorkspace>(ws) )
    {
      addPeaksWorkspaceMenuItems(menu, peaksWS);
    }
    else if( boost::dynamic_pointer_cast<const WorkspaceGroup>(ws) ) 
    {
      addWorkspaceGroupMenuItems(menu);
    }
    else if( boost::dynamic_pointer_cast<const Mantid::API::ITableWorkspace>(ws) )
    {
      addTableWorkspaceMenuItems(menu);
    }
    addClearMenuItems(menu, selectedWsName);

    //Get the names of the programs for the send to option
    std::vector<std::string> programNames = (Mantid::Kernel::ConfigService::Instance().getKeys("workspace.sendto.name"));
    bool firstPass(true);
    //Check to see if any options aren't visible
    for (size_t i = 0; i<programNames.size(); i++)
    {
      std::string visible = Mantid::Kernel::ConfigService::Instance().getString("workspace.sendto." + programNames[i] + ".visible");
      std::string target = Mantid::Kernel::ConfigService::Instance().getString("workspace.sendto." + programNames[i] + ".target");
      if (Mantid::Kernel::ConfigService::Instance().isExecutable(target) && visible == "Yes")
      {
        bool compatible(true);
        std::string saveUsing(Mantid::Kernel::ConfigService::Instance().getString("workspace.sendto." + programNames[i] + ".saveusing") );
        try
        {
          Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create(saveUsing);
          alg->setPropertyValue("InputWorkspace", selectedWsName.toStdString() );
        }
        catch(std::exception&)
        {
          compatible = false;
        }
        if (compatible != false)
        {
          if (firstPass == true)
          {
            m_saveToProgram = new QMenu(tr("Send to"),this);
            menu->addMenu(m_saveToProgram);

            //Sub-menu for program list
            m_programMapper = new QSignalMapper(this);
          }
          QString name = QString::fromStdString(programNames[i]);
          //Setup new menu option for the program
          m_program = new QAction(tr(name),this);
          connect(m_program,SIGNAL(activated()),m_programMapper,SLOT(map()));
          //Send name of program when clicked
          m_programMapper->setMapping(m_program, name);
          m_saveToProgram->addAction(m_program);		

          // Set first pass to false so that it doesn't set up another menu entry for all programs.
          firstPass = false;
        }
      } 
    }

    //Tell the button what to listen for and what to do once clicked (if there is anything to connect it will be set to false)
    if (firstPass == false)    
      connect(m_programMapper, SIGNAL(mapped(const QString &)), this, SLOT(saveToProgram(const QString &)));

    //Rename is valid for all workspace types
    menu->addAction(m_rename);
    //separate delete
    menu->addSeparator();
    menu->addAction(m_delete);
  }

  //Show the menu at the cursor's current position
  menu->popup(QCursor::pos());
}

void MantidDockWidget::groupingButtonClick()
{
  if(m_groupButton)
  {
    QString qButtonName=m_groupButton->text();
    if(qButtonName == "Group")
    {
      m_mantidUI->groupWorkspaces();
    }
    else if(qButtonName == "Ungroup")
    {
      m_mantidUI->ungroupWorkspaces();
    }
  }
}

/// Plots a single spectrum from each selected workspace
void MantidDockWidget::plotSpectra()
{
  const QMultiMap<QString,std::set<int> > toPlot = m_tree->chooseSpectrumFromSelected();
  // An empty map will be returned if the user clicks cancel in the spectrum selection
  if (toPlot.empty()) return;

  m_mantidUI->plot1D(toPlot, true, MantidQt::DistributionDefault, false);
}

/// Plots a single spectrum from each selected workspace with errors
void MantidDockWidget::plotSpectraErr()
{
  const QMultiMap<QString,std::set<int> > toPlot = m_tree->chooseSpectrumFromSelected();
  // An empty map will be returned if the user clicks cancel in the spectrum selection
  if (toPlot.empty()) return;

  m_mantidUI->plot1D(toPlot, true, MantidQt::DistributionDefault, true);
}

/**
* Draw a color fill plot of the workspaces that are currently selected.
* NOTE: The drawing of 2D plots is currently intimately linked with MantidMatrix meaning
* that one of these must be generated first!
*/
void MantidDockWidget::drawColorFillPlot()
{
  // Get the selected workspaces
  const QStringList wsNames = m_tree->getSelectedWorkspaceNames();
  if( wsNames.empty() ) return;

  // Extract child workspace names from any WorkspaceGroups selected.
  QSet<QString> allWsNames;
  foreach( const QString wsName, wsNames )
  {
    const auto wsGroup = boost::dynamic_pointer_cast<const WorkspaceGroup>(m_ads.retrieve(wsName.toStdString()));
    if( wsGroup )
    {
      const auto children = wsGroup->getNames();
      for( auto childWsName = children.begin(); childWsName != children.end(); ++childWsName )
        allWsNames.insert(QString::fromStdString(*childWsName));
    }
    else
      allWsNames.insert(wsName);
  }

  m_mantidUI->drawColorFillPlots(allWsNames.toList());
}

void MantidDockWidget::treeSelectionChanged()
{
  //get selected workspaces
  QList<QTreeWidgetItem*>Items = m_tree->selectedItems();

  if(m_groupButton)
  {
    if(Items.size()==1)
    {
      //check it's group
      QList<QTreeWidgetItem*>::const_iterator itr=Items.begin();
      std::string selectedWSName=(*itr)->text(0).toStdString();
      if(m_ads.doesExist(selectedWSName))
      {
        Workspace_sptr wsSptr=m_ads.retrieve(selectedWSName);
        WorkspaceGroup_sptr grpSptr=boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
        if(grpSptr)
        {
          m_groupButton->setText("Ungroup");
          m_groupButton->setEnabled(true);
        }
        else
          m_groupButton->setEnabled(false);
      }

    }
    else if(Items.size()>=2)
    {
      m_groupButton->setText("Group");
      m_groupButton->setEnabled(true);
    }
    else if(Items.size()==0)
    {
      m_groupButton->setText("Group");
      m_groupButton->setEnabled(false);
    }
  }

  if(m_deleteButton)
    m_deleteButton->setEnabled(Items.size() > 0);

  if(m_saveButton)
    m_saveButton->setEnabled(Items.size() > 0);

  if (Items.size() > 0)
  {
    auto item = *(Items.begin());
    m_mantidUI->enableSaveNexus(item->text(0));
  }
  else
  {
    m_mantidUI->disableSaveNexus();
  }
}

/**
* Convert selected TableWorkspace to a MatrixWorkspace.
*/
void MantidDockWidget::convertToMatrixWorkspace()
{
  m_mantidUI->showAlgorithmDialog(QString("ConvertTableToMatrixWorkspace"),-1);
}

/**
* Convert selected MDHistoWorkspace to a MatrixWorkspace.
*/
void MantidDockWidget::convertMDHistoToMatrixWorkspace()
{
  m_mantidUI->showAlgorithmDialog(QString("ConvertMDHistoToMatrixWorkspace"),-1);
}

/**
* Handler for the clear the UB matrix event.
*/
void MantidDockWidget::clearUB()
{
  QList<QTreeWidgetItem*> selectedItems = m_tree->selectedItems();
  QStringList selctedWSNames;
  if (!selectedItems.empty())
  {
    for (int i = 0; i < selectedItems.size(); ++i)
    {
      selctedWSNames.append(selectedItems[i]->text(0));
    }
  }
  m_mantidUI->clearUB(selctedWSNames);
}

/**
* Accept a drag drop event and process the data appropriately
* @param de :: The drag drop event
*/
void MantidDockWidget::dropEvent(QDropEvent *de)
{
  m_tree->dropEvent(de);
}

//------------ MantidTreeWidget -----------------------//

MantidTreeWidget::MantidTreeWidget(MantidDockWidget *w, MantidUI *mui)
  : QTreeWidget(w),m_dockWidget(w),m_mantidUI(mui),m_ads(Mantid::API::AnalysisDataService::Instance()),m_sortScheme()
{
  setObjectName("WorkspaceTree");
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setAcceptDrops(true);
}

/**
* Accept a drag move event and selects whether to accept the action
* @param de :: The drag move event
*/
void MantidTreeWidget::dragMoveEvent(QDragMoveEvent *de)
{
  // The event needs to be accepted here
  if (de->mimeData()->hasUrls())
    de->accept();
}

/**
* Accept a drag enter event and selects whether to accept the action
* @param de :: The drag enter event
*/
void MantidTreeWidget::dragEnterEvent(QDragEnterEvent *de)
{
  // Set the drop action to be the proposed action.
  if (de->mimeData()->hasUrls())
    de->acceptProposedAction();
}



/**
* Accept a drag drop event and process the data appropriately
* @param de :: The drag drop event
*/
void MantidTreeWidget::dropEvent(QDropEvent *de)
{
  QStringList filenames;
  const QMimeData *mimeData = de->mimeData();  
  if (mimeData->hasUrls()) 
  {
    QList<QUrl> urlList = mimeData->urls();
    for (int i = 0; i < urlList.size(); ++i) 
    {
      QString fName = urlList[i].toLocalFile();
      if (fName.size()>0)
      {
        filenames.append(fName);
      }
    }
  }
  de->acceptProposedAction();

  for (int i = 0; i < filenames.size(); ++i) 
  {
    try
    {
      QFileInfo fi(filenames[i]);
      QString basename = fi.baseName();
      IAlgorithm_sptr alg = m_mantidUI->createAlgorithm("Load");
      alg->initialize();
      alg->setProperty("Filename",filenames[i].toStdString());
      alg->setProperty("OutputWorkspace",basename.toStdString());
      m_mantidUI->executeAlgorithmAsync(alg,true);
    }
    catch (std::runtime_error& error)
    {
      treelog.error()<<"Failed to Load the file "<<filenames[i].toStdString()<<" . The reason for failure is: "<< error.what()<<std::endl;
    }      
    catch (std::logic_error& error)
    {
      treelog.error()<<"Failed to Load the file "<<filenames[i].toStdString()<<" . The reason for failure is: "<< error.what()<<std::endl;
    }
    catch (std::exception& error)
    {
      treelog.error()<<"Failed to Load the file "<<filenames[i].toStdString()<<" . The reason for failure is: "<< error.what()<<std::endl;
    }
  }
}

void MantidTreeWidget::mousePressEvent (QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton)
  {
    if( !itemAt(e->pos()) ) selectionModel()->clear();
    m_dragStartPosition = e->pos();
  }

  QTreeWidget::mousePressEvent(e);
}

void MantidTreeWidget::mouseMoveEvent(QMouseEvent *e)
{
  if (!(e->buttons() & Qt::LeftButton))
    return;
  if ((e->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
    return;

  // Start dragging
  QDrag *drag = new QDrag(this);
  QMimeData *mimeData = new QMimeData;

  QStringList wsnames = getSelectedWorkspaceNames();
  if (wsnames.size() == 0) return;
  QString importStatement = "";
  foreach( const QString wsname, wsnames )
  {
    QString prefix = "";
    if (wsname[0].isDigit()) prefix = "ws";
    if (importStatement.size() > 0) importStatement += "\n";
    importStatement += prefix + wsname + " = mtd[\"" + wsname + "\"]";
  }

  mimeData->setText(importStatement);
  mimeData->setObjectName("MantidWorkspace");

  drag->setMimeData(mimeData);

  Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
  (void) dropAction;
}

void MantidTreeWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
  try
  {
    QString wsName = m_mantidUI->getSelectedWorkspaceName();
    Mantid::API::WorkspaceGroup_sptr grpWSPstr;
    grpWSPstr=boost::dynamic_pointer_cast< WorkspaceGroup>
      (m_ads.retrieve(wsName.toStdString()));
    if(!grpWSPstr)
    {
      if ( ! wsName.isEmpty() )
      {
        m_mantidUI->importWorkspace(wsName,false);
        return;
      }
    }
  }
  catch(Mantid::Kernel::Exception::NotFoundError &)
  {
    return;
  }
  QTreeWidget::mouseDoubleClickEvent(e);
}

/**
* Returns a list of all selected workspaces.  It does NOT
* extract child workspaces from groups - it only returns
* exactly what has been selected.
*/
QStringList MantidTreeWidget::getSelectedWorkspaceNames() const
{
  QStringList names;

  foreach( const auto selectedItem, this->selectedItems() )
  {
    if( selectedItem )
      names.append(selectedItem->text(0));
  }

  return names;
}

/**
* Allows users to choose spectra from the selected workspaces by presenting them
* with a dialog box.  Skips showing the dialog box and automatically chooses
* workspace index 0 for all selected workspaces if one or more of the them are
* single-spectrum workspaces.
*
* We also must filter the list of selected workspace names to account for any
* non-MatrixWorkspaces that may have been selected.  In particular WorkspaceGroups
* (the children of which are to be included if they are MatrixWorkspaces) and
* TableWorkspaces (which are implicitly excluded).  We only want workspaces we
* can actually plot!
*
* @return :: A map of workspace name to spectrum numbers to plot.
*/
QMultiMap<QString,std::set<int> > MantidTreeWidget::chooseSpectrumFromSelected() const
{
  // Check for any selected WorkspaceGroup names and replace with the names of
  // their children.
  QSet<QString> selectedWsNames;
  foreach( const QString wsName, this->getSelectedWorkspaceNames() )
  {
    const auto groupWs = boost::dynamic_pointer_cast<const WorkspaceGroup>(m_ads.retrieve(wsName.toStdString()));
    if( groupWs )
    {
      const auto childWsNames = groupWs->getNames();
      for( auto childWsName = childWsNames.begin(); childWsName != childWsNames.end(); ++childWsName )
      {
        selectedWsNames.insert(QString::fromStdString(*childWsName));
      }
    }
    else
    {
      selectedWsNames.insert(wsName);
    }
  }

  // Get the names of, and pointers to, the MatrixWorkspaces only.
  QList<MatrixWorkspace_const_sptr> selectedMatrixWsList;
  QList<QString> selectedMatrixWsNameList;
  foreach( const auto selectedWsName, selectedWsNames )
  {
    const auto matrixWs = boost::dynamic_pointer_cast<const MatrixWorkspace>(m_ads.retrieve(selectedWsName.toStdString()));
    if( matrixWs )
    {
      selectedMatrixWsList.append(matrixWs);
      selectedMatrixWsNameList.append(QString::fromStdString(matrixWs->name()));
    }
  }

  // Check to see if all workspaces have only a single spectrum ...
  bool allSingleWorkspaces = true;
  foreach( const auto selectedMatrixWs, selectedMatrixWsList )
  {
    if( selectedMatrixWs->getNumberHistograms() != 1 )
    {
      allSingleWorkspaces = false;
      break;
    }
  }

  // ... and if so, just return all workspace names mapped to workspace index 0;
  if( allSingleWorkspaces )
  {
    const std::set<int> SINGLE_SPECTRUM = boost::assign::list_of<int>(0);
    QMultiMap<QString,std::set<int>> spectrumToPlot;
    foreach( const auto selectedMatrixWs, selectedMatrixWsList )
    {
      spectrumToPlot.insert(
        QString::fromStdString(selectedMatrixWs->name()),
        SINGLE_SPECTRUM
        );
    }
    return spectrumToPlot;
  }

  // Else, one or more workspaces 
  MantidWSIndexDialog *dio = new MantidWSIndexDialog(m_mantidUI, 0, selectedMatrixWsNameList);
  dio->exec();
  return dio->getPlots();
}

void MantidTreeWidget::setSortScheme(MantidItemSortScheme sortScheme)
{
  m_sortScheme = sortScheme;
}

void MantidTreeWidget::setSortOrder(Qt::SortOrder sortOrder)
{
  m_sortOrder = sortOrder;
}

Qt::SortOrder MantidTreeWidget::getSortOrder() const
{
  return m_sortOrder;
}

MantidItemSortScheme MantidTreeWidget::getSortScheme() const
{
  return m_sortScheme;
}

/**
* Sort the items according to the current sort scheme and order.
*/
void MantidTreeWidget::sort()
{
  sortItems(sortColumn(), m_sortOrder);
}

/**
* Log a warning message.
* @param msg :: A message to log.
*/
void MantidTreeWidget::logWarningMessage(const std::string& msg)
{
  treelog.warning( msg );
}

//-------------------- MantidTreeWidgetItem ----------------------//
/**Constructor.
* Must be passed its parent MantidTreeWidget, to facilitate correct sorting.
*/
MantidTreeWidgetItem::MantidTreeWidgetItem(MantidTreeWidget* parent)
  :QTreeWidgetItem(parent),m_parent(parent),m_sortPos(0)
{
}

/**Constructor.
* Must be passed its parent MantidTreeWidget, to facilitate correct sorting.
*/
MantidTreeWidgetItem::MantidTreeWidgetItem(QStringList list, MantidTreeWidget* parent):
  QTreeWidgetItem(list),m_parent(parent),m_sortPos(0)
{
}

/**Overidden operator.
* Must be passed its parent MantidTreeWidget, to facilitate correct sorting.
*/
bool MantidTreeWidgetItem::operator<(const QTreeWidgetItem &other)const
{
  // If this and/or other has been set to have a Qt::UserRole, then
  // it has an accompanying sort order that we must maintain, no matter
  // what the user has seletected in terms of order or scheme.

  bool thisShouldBeSorted = m_sortPos == 0;
  const MantidTreeWidgetItem *mantidOther = dynamic_cast<const MantidTreeWidgetItem*>(&other);
  int otherSortPos = mantidOther ? mantidOther->getSortPos() : 0;
  bool otherShouldBeSorted = otherSortPos == 0;

  // just in case m_parent is NULL. I think I saw this once but cannot reproduce.
  if ( !m_parent ) return false;

  if(!thisShouldBeSorted && !otherShouldBeSorted)
  {
    if(m_parent->getSortOrder() == Qt::Ascending)
      return m_sortPos < otherSortPos;
    else
      return m_sortPos >= otherSortPos;
  }
  else if(thisShouldBeSorted && !otherShouldBeSorted)
  {
    if(m_parent->getSortOrder() == Qt::Ascending)
      return false;
    else
      return true;
  }
  else if(!thisShouldBeSorted && otherShouldBeSorted)
  {
    if(m_parent->getSortOrder() == Qt::Ascending)
      return true;
    else
      return false;
  }

  // If both should be sorted, and the scheme is set to ByName ...
  if(m_parent->getSortScheme() == ByName)
  {
    if(QString::compare(text(0), other.text(0), Qt::CaseInsensitive) < 0)
      return true;
    return false;
  }
  // ... else both should be sorted and the scheme is set to ByLastModified.
  else
  {
    try
    {
      if(childCount() > 0 && other.childCount() > 0)
      {
        const QTreeWidgetItem * other_ptr = &other;

        try
        {
          return getLastModified(this) < getLastModified(other_ptr);
        }
        catch(std::out_of_range &e)
        {
          m_parent->logWarningMessage( e.what() );
          return false;
        }
      }
    }
    catch (Mantid::Kernel::Exception::NotFoundError&)
    {
      ;
    }
    return false;
  }
}

/**Finds the date and time of the last modification made to the workspace who's details
* are found in the given QTreeWidgetItem.
*/
DateAndTime MantidTreeWidgetItem::getLastModified(const QTreeWidgetItem* item)
{
  QVariant userData = item->data(0, Qt::UserRole);
  if ( userData.isNull() ) return DateAndTime(); //now

  Workspace_sptr workspace = userData.value<Workspace_sptr>();
  const Mantid::API::WorkspaceHistory & wsHist = workspace->getHistory();
  if(wsHist.empty()) return DateAndTime(); // now

  const size_t indexOfLast = wsHist.size() - 1;
  const auto lastAlgHist = wsHist.getAlgorithmHistory(indexOfLast);
  return lastAlgHist->executionDate();
}

//-------------------- AlgorithmDockWidget ----------------------//
/** Create a QDockWidget containing:
* The AlgorithmSelectorWidget
* The progress bar and Details button
*/
AlgorithmDockWidget::AlgorithmDockWidget(MantidUI *mui, ApplicationWindow *w):
  QDockWidget(w),m_progressBar(NULL),m_algID(),m_mantidUI(mui)
{
  setWindowTitle(tr("Algorithms"));
  setObjectName("exploreAlgorithms"); // this is needed for QMainWindow::restoreState()
  setMinimumHeight(150);
  setMinimumWidth(200);
  w->addDockWidget( Qt::RightDockWidgetArea, this );//*/

  //Add the AlgorithmSelectorWidget
  m_selector = new MantidQt::MantidWidgets::AlgorithmSelectorWidget(this);
  connect(m_selector,SIGNAL(executeAlgorithm(const QString &, const int)),
    m_mantidUI,SLOT(showAlgorithmDialog(const QString &, const int)));

  m_runningLayout = new QHBoxLayout();
  m_runningLayout->setName("testA");

  m_runningButton = new QPushButton("Details");
  m_runningLayout->addStretch();
  m_runningLayout->addWidget(m_runningButton);
  connect(m_runningButton,SIGNAL(clicked()),m_mantidUI,SLOT(showAlgMonitor()));


  QFrame *f = new QFrame(this);
  QVBoxLayout * layout = new QVBoxLayout(f, 4 /*border*/, 4 /*spacing*/);
  f->setLayout(layout);
  layout->setMargin(0);
  layout->addWidget(m_selector);
  layout->addLayout(m_runningLayout);

  setWidget(f);

}

/** Update the list of algorithms in the dock */
void AlgorithmDockWidget::update()
{
  m_selector->update();
}


void AlgorithmDockWidget::updateProgress(void* alg, const double p, const QString& msg, double estimatedTime, int progressPrecision)
{
  if (m_algID.empty()) return;
  if (alg == m_algID.first() && p >= 0 && p <= 100 && m_progressBar)
  {
    m_progressBar->setValue( static_cast<int>(p) );
    // Make the progress string
    std::ostringstream mess;
    mess << msg.toStdString();
    mess.precision(progressPrecision);
    mess << " " << std::fixed << p << "%";
    if (estimatedTime > 0.5)
    {
      mess.precision(0);
      mess << " (~";
      if (estimatedTime < 60)
        mess << static_cast<int>(estimatedTime) << "s";
      else if (estimatedTime < 60*60)
      {
        int min = static_cast<int>(estimatedTime/60);
        int sec = static_cast<int>(estimatedTime - min*60);
        mess << min << "m"
          << std::setfill('0') << std::setw(2) << sec << "s";
      }
      else
      {
        int hours = static_cast<int>(estimatedTime/3600);
        int min = static_cast<int>( (estimatedTime-hours*3600)/60);
        mess << hours << "h"
          << std::setfill('0') << std::setw(2) << min << "h";
      }
      mess << ")";
    }
    QString formatStr = QString::fromStdString(mess.str());
    m_progressBar->setFormat(formatStr);
  }
}

void AlgorithmDockWidget::algorithmStarted(void* alg)
{
  m_algID.push_front(alg);
  hideProgressBar();
  showProgressBar();
}

void AlgorithmDockWidget::algorithmFinished(void* alg)
{
  if (m_algID.empty()) return;
  m_algID.removeAll(alg);
  hideProgressBar();
}

void AlgorithmDockWidget::showProgressBar()
{
  if (m_progressBar == NULL)
  {
    // insert progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setAlignment(Qt::AlignHCenter);
    m_runningLayout->insertWidget(1,m_progressBar);
    // remove the stretch item
    m_runningLayout->removeItem(m_runningLayout->takeAt(0));
  }
}

void AlgorithmDockWidget::hideProgressBar()
{
  if (m_progressBar && m_algID.empty())
  {
    m_runningLayout->insertStretch(0);
    m_runningLayout->removeWidget(m_progressBar);
    m_progressBar->close();
    delete m_progressBar;
    m_progressBar = NULL;
  }
}


//--------------------  ----------------------//

