#include "QWorkspaceDockView.h"
#include "FlowLayout.h"
#include "MantidTreeWidget.h"
#include "MantidTreeWidgetItem.h"
#include "MantidMatrix.h"
#include <MantidKernel/make_unique.h>
#include <MantidQtMantidWidgets/LineEditWithClear.h>
#include <MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h>
#include <MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h>
#include <MantidQtAPI/InterfaceManager.h>
#include <MantidQtAPI/AlgorithmDialog.h>
#include <MantidQtAPI/AlgorithmInputHistory.h>

#include <MantidAPI/IMDEventWorkspace.h>
#include <MantidAPI/IPeaksWorkspace.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/WorkspaceGroup.h>

#include <QHash>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSignalMapper>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

QWorkspaceDockView::QWorkspaceDockView(MantidUI *mui, ApplicationWindow *parent)
    : QDockWidget(tr("Workspaces"), parent), m_mantidUI(mui), m_updateCount(0),
      m_treeUpdating(false), m_promptDelete(false) {
  setObjectName(
      "exploreMantid"); // this is needed for QMainWindow::restoreState()
  setMinimumHeight(150);
  setMinimumWidth(200);
  parent->addDockWidget(Qt::RightDockWidgetArea, this);

  m_appParent = parent;

  m_saveMenu = new QMenu(this);

  setupWidgetLayout();
  setupLoadButtonMenu();

  // Dialog box used for user to specify folder to save multiple workspaces into
  m_saveFolderDialog = new QFileDialog;
  m_saveFolderDialog->setFileMode(QFileDialog::DirectoryOnly);
  m_saveFolderDialog->setOption(QFileDialog::ShowDirsOnly);

  // SET UP SORT
  createSortMenuActions();
  createWorkspaceMenuActions();

  setupConnections();

  m_tree->setDragEnabled(true);
}

QWorkspaceDockView::~QWorkspaceDockView() {}

void QWorkspaceDockView::setupWidgetLayout() {
  QFrame *f = new QFrame(this);
  setWidget(f);

  m_tree = new MantidTreeWidget(this, m_mantidUI);
  m_tree->setHeaderLabel("Workspaces");

  FlowLayout *buttonLayout = new FlowLayout();
  m_loadButton = new QPushButton("Load");
  m_saveButton = new QPushButton("Save");
  m_deleteButton = new QPushButton("Delete");
  m_groupButton = new QPushButton("Group");
  m_sortButton = new QPushButton("Sort");

  if (m_groupButton)
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

  QVBoxLayout *layout = new QVBoxLayout();
  f->setLayout(layout);
  layout->setSpacing(0);
  layout->setMargin(0);
  layout->addLayout(buttonLayout);
  layout->addWidget(m_workspaceFilter);
  layout->addWidget(m_tree);
}

void QWorkspaceDockView::setupLoadButtonMenu() {
  m_loadMenu = new QMenu(this);

  QAction *loadFileAction = new QAction("File", this);
  QAction *liveDataAction = new QAction("Live Data", this);
  connect(liveDataAction, SIGNAL(triggered()), this, SLOT(onClickLoad()));
  connect(loadFileAction, SIGNAL(triggered()), m_loadMapper,
          SLOT(onClickLiveData()));

  m_loadMenu->addAction(loadFileAction);
  m_loadMenu->addAction(liveDataAction);
  m_loadButton->setMenu(m_loadMenu);
}

void QWorkspaceDockView::setupConnections() {
  connect(m_workspaceFilter, SIGNAL(textChanged(const QString &)), this,
          SLOT(filterWorkspaceTree(const QString &)));
  connect(m_deleteButton, SIGNAL(clicked()), this,
          SLOT(onClickDeleteWorkspaces()));
  connect(m_tree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this,
          SLOT(clickedWorkspace(QTreeWidgetItem *, int)));
  connect(m_tree, SIGNAL(itemSelectionChanged()), this,
          SLOT(workspaceSelected()));
  connect(m_groupButton, SIGNAL(clicked()), this, SLOT(onClickGroupButton()));

  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(popupMenu(const QPoint &)));

  //// call this slot directly after the signal is received. just increment the
  //// update counter
  //connect(m_mantidUI, SIGNAL(ADS_updated()), this, SLOT(incrementUpdateCount()),
  //        Qt::DirectConnection);
  //// this slot is called when the GUI thread is free. decrement the counter. do
  //// nothing until the counter == 0
  //connect(m_mantidUI, SIGNAL(ADS_updated()), this, SLOT(updateTree()),
  //        Qt::QueuedConnection);

  connect(m_mantidUI, SIGNAL(workspaces_cleared()), m_tree, SLOT(clear()),
          Qt::QueuedConnection);
  connect(m_tree, SIGNAL(itemSelectionChanged()), this,
          SLOT(treeSelectionChanged()));
  connect(m_tree, SIGNAL(itemExpanded(QTreeWidgetItem *)), this,
          SLOT(populateChildData(QTreeWidgetItem *)));
}

void QWorkspaceDockView::init() {
	presenter = boost::make_shared<WorkspacePresenter>(shared_from_this());
	presenter->init();
}

WorkspacePresenter_wptr QWorkspaceDockView::getPresenterWeakPtr() {
  return presenter;
}

WorkspacePresenter_sptr QWorkspaceDockView::getPresenterSharedPtr() {
  return presenter;
}

StringList QWorkspaceDockView::getSelectedWorkspaceNames() const {
  QList<QTreeWidgetItem *> items = m_tree->selectedItems();
  StringList names;

  for (auto &item : items)
    names.push_back(item->text(0).toStdString());

  return StringList();
}

Mantid::API::Workspace_sptr QWorkspaceDockView::getSelectedWorkspace() const {
	//TODO: is this method really necessary?
  return nullptr;
}

bool QWorkspaceDockView::askUserYesNo(const std::string &caption,
                                      const std::string &message) const {
  return QMessageBox::question(m_appParent, QString::fromStdString(caption),
                               QString::fromStdString(message),
                               QMessageBox::Yes,
                               QMessageBox::No) == QMessageBox::Yes;
}

void QWorkspaceDockView::showCriticalUserMessage(
    const std::string &caption, const std::string &message) const {
  QMessageBox::critical(m_appParent, QString::fromStdString(caption),
                        QString::fromStdString(message));
}

Mantid::API::IAlgorithm_sptr
QWorkspaceDockView::createAlgorithm(const std::string &algName) {
  Mantid::API::IAlgorithm_sptr alg;
  try {
    alg = Mantid::API::AlgorithmManager::Instance().create(algName, -1);
  } catch (...) {
    QMessageBox::warning(m_appParent, "MantidPlot",
                         "Cannot create algorithm \"" +
                             QString::fromStdString(algName) + "\"");
    alg = Mantid::API::IAlgorithm_sptr();
  }
  return alg;
}

void QWorkspaceDockView::showAlgorithm(const std::string &algName) {
  try {
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create(algName, -1);
    if (!alg)
      return;

    QHash<QString, QString> presets;
    QStringList enabled;
    QString inputWsProp;
    // If a property was explicitly set show it as preset in the dialog
    const std::vector<Mantid::Kernel::Property *> props = alg->getProperties();
    for (const auto &p: props) {
      if (p->isDefault()) {
        QString property_name = QString::fromStdString(p->name());
        presets.insert(property_name, QString::fromStdString(p->value()));
        enabled.append(property_name);
      }

      const Mantid::API::IWorkspaceProperty *ws_prop =
          dynamic_cast<Mantid::API::IWorkspaceProperty *>(p);
      if (ws_prop) {
        unsigned int direction = p->direction();
        if (direction == Mantid::Kernel::Direction::Input ||
            direction == Mantid::Kernel::Direction::InOut) {
          inputWsProp = QString::fromStdString(p->name());
        }
      }
    }

    // If a workspace is selected in the dock then set this as a preset for the
    // dialog
    QString selected = QString::fromStdString(getSelectedWorkspaceNames()[0]);
    if (!selected.isEmpty()) {
      if (!presets.contains(inputWsProp)) {
        presets.insert(inputWsProp, selected);
        // Keep it enabled
        enabled.append(inputWsProp);
      }
    }

    // Check if a workspace is selected in the dock and set this as a preference
    // for the input workspace
    // This is an optional message displayed at the top of the GUI.
    QString optional_msg(alg->summary().c_str());

    MantidQt::API::InterfaceManager interfaceManager;
    MantidQt::API::AlgorithmDialog *dlg = interfaceManager.createDialog(
        alg, m_appParent, false, presets, optional_msg, enabled);

    if (algName == "Load") {
      // update recent files
      connect(dlg, SIGNAL(accepted()), this, SLOT(onLoadAccept()));
    }

    dlg->show();
    dlg->raise();
    dlg->activateWindow();
  } catch (...) {
    QMessageBox::warning(m_appParent, "MantidPlot",
                         "Cannot create algorithm \"" +
                             QString::fromStdString(algName) + "\"");
  }
}

void QWorkspaceDockView::onLoadAccept() {
	QObject *sender = QObject::sender();
	MantidQt::API::AlgorithmDialog *dlg =
		reinterpret_cast<MantidQt::API::AlgorithmDialog *>(sender);
	if (!dlg)
		return; // should never happen

	QString fn = MantidQt::API::AlgorithmInputHistory::Instance().previousInput(
		"Load", "Filename");
	
	emit updateRecentFiles(fn);
}

void QWorkspaceDockView::showLoadDialog() {
	showAlgorithm("Load");
}

void QWorkspaceDockView::showLiveDataDialog() {
	showAlgorithm("StartLiveData");
}

void QWorkspaceDockView::showRenameDialog(const StringList &wsNames) const {}
void QWorkspaceDockView::recordWorkspaceRename(const std::string &oldName,
                                               const std::string &newName) {
  // check if old_name has been recently a new name
  QList<QString> oldNames = m_renameMap.keys(oldName);
  // non-empty list of oldNames become new_name
  if (!oldNames.isEmpty()) {
    foreach (QString name, oldNames) { m_renameMap[name] = newName; }
  } else {
    // record a new rename pair
    m_renameMap[oldName] = newName;
  }
}

void QWorkspaceDockView::groupWorkspaces(const StringList &wsNames,
                                         const std::string &groupName) const {
  try {
    std::string algName("GroupWorkspaces");
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create(algName, 1);
    alg->initialize();
    alg->setProperty("InputWorkspaces", wsNames);
    alg->setPropertyValue("OutputWorkspace", groupName);
    // execute the algorithm
    bool bStatus = alg->execute();
    if (!bStatus) {
      showCriticalUserMessage("MantidPlot - Algorithm error",
                              " Error in GroupWorkspaces algorithm");
    }
  } catch (std::invalid_argument &) {
    showCriticalUserMessage("MantidPlot - Algorithm error",
                            " Error in GroupWorkspaces algorithm");
  } catch (Mantid::Kernel::Exception::NotFoundError &) // if not a valid object
                                                       // in analysis data
                                                       // service
  {
    showCriticalUserMessage("MantidPlot - Algorithm error",
                            " Error in GroupWorkspaces algorithm");
  } catch (std::runtime_error &) {
    showCriticalUserMessage("MantidPlot - Algorithm error",
                            " Error in GroupWorkspaces algorithm");
  } catch (std::exception &) {
    showCriticalUserMessage("MantidPlot - Algorithm error",
                            " Error in GroupWorkspaces algorithm");
  }
}

void QWorkspaceDockView::ungroupWorkspaces(const StringList &wsNames) const {
  try {
    // workspace name
    std::string wsname = wsNames[0];

    std::string algName("UnGroupWorkspace");
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create(algName, 1);
    alg->initialize();
    alg->setProperty("InputWorkspace", wsname);

    // execute the algorithm
    bool bStatus = alg->execute();
    if (!bStatus) {
      showCriticalUserMessage("MantidPlot - Algorithm error",
                              " Error in UnGroupWorkspace algorithm");
    }
  } catch (std::invalid_argument &) {
    showCriticalUserMessage("MantidPlot - Algorithm error",
                            " Error in UnGroupWorkspace algorithm");
  } catch (std::runtime_error &) {
    showCriticalUserMessage("MantidPlot - Algorithm error",
                            " Error in UnGroupWorkspace algorithm");
  } catch (std::exception &) {
    showCriticalUserMessage("MantidPlot - Algorithm error",
                            " Error in UnGroupWorkspace algorithm");
  }
}

void QWorkspaceDockView::enableDeletePrompt(bool enable) {
  m_promptDelete = enable;
}

bool QWorkspaceDockView::isPromptDelete() const { return m_promptDelete; }

bool QWorkspaceDockView::deleteConfirmation() const {
  return askUserYesNo(
      "Delete Workspaces",
      "Are you sure you want to delete the selected Workspaces?\n\nThis "
      "prompt can be disabled from:\nPreferences->General->Confirmations");
}

void QWorkspaceDockView::deleteWorkspaces(const StringList &wsNames) {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(m_appParent->activeWindow());

  auto alg = createAlgorithm("DeleteWorkspace");

  try {
    if (alg != nullptr) {
      alg->setLogging(false);

      if ((m_deleteButton->hasFocus() || m_tree->hasFocus()) &&
          !wsNames.empty()) {
        for (auto &ws : wsNames) {
          alg->setPropertyValue("Workspace", ws);
          alg->executeAsync();
        }
      } else if ((m && (strcmp(m->metaObject()->className(), "MantidMatrix") ==
                        0)) &&
                 !m->workspaceName().isEmpty()) {
        m_mantidUI->deleteWorkspace(m->workspaceName());
        alg->setPropertyValue("Workspace", m->workspaceName().toStdString());
        alg->executeAsync();
      }
    }
  } catch (...) {
    QMessageBox::warning(m_appParent, "",
                         "Could not delete selected workspaces.");
  }
}

void QWorkspaceDockView::clearView() { m_tree->clear(); }

QWorkspaceDockView::SortDirection QWorkspaceDockView::getSortDirection() const {
  return SortDirection::Ascending;
}

QWorkspaceDockView::SortCriteria QWorkspaceDockView::getSortCriteria() const {
  return SortCriteria::ByName;
}

void QWorkspaceDockView::sortWorkspaces(SortCriteria criteria,
                                        SortDirection direction) {}

QWorkspaceDockView::SaveFileType QWorkspaceDockView::getSaveFileType() const {
  return SaveFileType::Nexus;
}

void QWorkspaceDockView::saveWorkspace(const std::string &wsName,
                                       SaveFileType type) {}

void QWorkspaceDockView::saveWorkspaces(const StringList &wsNames) {}

std::string QWorkspaceDockView::getFilterText() const {
  return m_workspaceFilter->text().toStdString();
}

void QWorkspaceDockView::filterWorkspaces(const std::string &filterText) {
  const QString text = QString::fromStdString(filterText).trimmed();
  QRegExp filterRegEx(text, Qt::CaseInsensitive);

  // show all items
  QTreeWidgetItemIterator it(m_tree);
  while (*it) {
    (*it)->setHidden(false);
    ++it;
  }

  int hiddenCount = 0;
  QList<QTreeWidgetItem *> visibleGroups;
  if (!text.isEmpty()) {
    // Loop over everything (currently loaded) and top level
    // find out what is already expanded
    QStringList expanded;
    int n = m_tree->topLevelItemCount();
    for (int i = 0; i < n; ++i) {
      auto item = m_tree->topLevelItem(i);
      if (item->isExpanded()) {
        expanded << item->text(0);
      } else {
        // expand everything that is at the top level (as we lazy load this is
        // required)
        item->setExpanded(true);
      }
    }

    // filter based on the string
    QTreeWidgetItemIterator it(m_tree, QTreeWidgetItemIterator::All);
    while (*it) {
      QTreeWidgetItem *item = (*it);
      QVariant userData = item->data(0, Qt::UserRole);

      if (!userData.isNull()) {
        Workspace_sptr workspace = userData.value<Workspace_sptr>();
        if (workspace) {
          // I am a workspace
          if (item->text(0).contains(filterRegEx)) {
            // my name does match the filter
            if (auto group =
                    boost::dynamic_pointer_cast<WorkspaceGroup>(workspace)) {
              // I am a group, I will want my children to be visible
              // but I cannot do that until this iterator has finished
              // store this pointer in a list for processing later
              visibleGroups.append(item);
              item->setHidden(false);
            }

            if (item->parent() == NULL) {
              // No parent, I am a top level workspace - show me
              item->setHidden(false);
            } else {
              // I am a child workspace of a group
              // I match, so I want my parent to remain visible as well.
              item->setHidden(false);
              if (item->parent()->isHidden()) {
                // I was previously hidden, show me and set to be expanded
                --hiddenCount;
                item->parent()->setHidden(false);
                expanded << item->parent()->text(0);
              }
            }
          } else {
            // my name does not match the filter - hide me
            item->setHidden(true);
            ++hiddenCount;
          }
        }
      }
      ++it;
    }

    // make children of visible groups visible
    for (auto itGroup = visibleGroups.begin(); itGroup != visibleGroups.end();
         ++itGroup) {
      QTreeWidgetItem *group = (*itGroup);
      for (int i = 0; i < group->childCount(); i++) {
        QTreeWidgetItem *child = group->child(i);
        if (child->isHidden()) {
          // I was previously hidden, show me
          --hiddenCount;
          child->setHidden(false);
        }
      }
    }

    // set the expanded state
    for (int i = 0; i < n; ++i) {
      auto item = m_tree->topLevelItem(i);
      item->setExpanded(expanded.contains(item->text(0)));
    }
  }

  // display a message if items are hidden
  if (hiddenCount > 0) {
    QString headerString =
        QString("Workspaces (%1 filtered)").arg(QString::number(hiddenCount));
    m_tree->headerItem()->setText(0, headerString);
  } else {
    m_tree->headerItem()->setText(0, "Workspaces");
  }
}

void QWorkspaceDockView::populateChildData(QTreeWidgetItem *item) {
  QVariant userData = item->data(0, Qt::UserRole);
  if (userData.isNull())
    return;

  // Clear it first
  while (item->childCount() > 0) {
    auto *widgetItem = item->takeChild(0);
    delete widgetItem;
  }

  Workspace_sptr workspace = userData.value<Workspace_sptr>();

  if (auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace)) {
    const size_t nmembers = group->getNumberOfEntries();
    for (size_t i = 0; i < nmembers; ++i) {
      auto ws = group->getItem(i);
      auto *node = addTreeEntry(std::make_pair(ws->name(), ws), item);
      excludeItemFromSort(node);
      if (shouldBeSelected(node->text(0)))
        node->setSelected(true);
    }
  } else {
    QString details;
    try {
      details = workspace->toString().c_str();
    } catch (std::runtime_error &e) {
      details = QString("Error: %1").arg(e.what());
    }
    QStringList rows =
        details.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    rows.append(QString("Memory used: ") +
                workspace->getMemorySizeAsStr().c_str());

    auto iend = rows.constEnd();
    for (auto itr = rows.constBegin(); itr != iend; ++itr) {
      MantidTreeWidgetItem *data =
          new MantidTreeWidgetItem(QStringList(*itr), m_tree);
      data->setFlags(Qt::NoItemFlags);
      excludeItemFromSort(data);
      item->addChild(data);
    }
  }
}

void QWorkspaceDockView::updateTree(
    const std::map<std::string, Mantid::API::Workspace_sptr> &items) {
  // do not update until the counter is zero
  if (m_updateCount.deref())
    return;

  // find all expanded top-level entries
  QStringList expanded;
  int n = m_tree->topLevelItemCount();
  for (int i = 0; i < n; ++i) {
    auto item = m_tree->topLevelItem(i);
    if (item->isExpanded()) {
      expanded << item->text(0);
    }
  }

  // create a new tree
  setTreeUpdating(true);
  populateTopLevel(items, expanded);
  setTreeUpdating(false);

  // Re-sort
  m_tree->sort();
}

void QWorkspaceDockView::populateTopLevel(
    const std::map<std::string, Mantid::API::Workspace_sptr> &topLevelItems,
    const QStringList &expanded) {
  // collect names of selected workspaces
  QList<QTreeWidgetItem *> selected = m_tree->selectedItems();
  m_selectedNames.clear(); // just in case
  foreach (QTreeWidgetItem *item, selected) {
    m_selectedNames << item->text(0);
  }

  // populate the tree from scratch
  m_tree->clear();
  auto iend = topLevelItems.end();
  for (auto it = topLevelItems.begin(); it != iend; ++it) {
    auto *node = addTreeEntry(*it);
    QString name = node->text(0);
    if (expanded.contains(name))
      node->setExpanded(true);
    // see if item must be selected
    if (shouldBeSelected(name))
      node->setSelected(true);
  }
  m_selectedNames.clear();
  m_renameMap.clear();

  // apply any filtering
  filterWorkspaceTree(m_workspaceFilter->text());
}

/**
* Filter workspaces based on the string provided
* @param text : the string to filter on.
*/
void QWorkspaceDockView::filterWorkspaceTree(const QString &text) {
  m_filteredText = text.toStdString();
  presenter->notifyFromView(ViewNotifiable::Flag::FilterWorkspaces);
}

void QWorkspaceDockView::onClickDeleteWorkspaces() {
  presenter->notifyFromView(ViewNotifiable::Flag::DeleteWorkspaces);
}

void QWorkspaceDockView::clickedWorkspace(QTreeWidgetItem *item, int) {
  Q_UNUSED(item);
}

void QWorkspaceDockView::workspaceSelected() {
  QList<QTreeWidgetItem *> selectedItems = m_tree->selectedItems();
  if (selectedItems.isEmpty())
    return;

  // If there are multiple workspaces selected group and save as Nexus
  if (selectedItems.length() > 1) {
    connect(m_saveButton, SIGNAL(clicked()), this, SLOT(saveWorkspaceGroup()));

    // Don't display as a group
    m_saveButton->setMenu(NULL);
  } else {
    // Don't run the save group function when clicked
    disconnect(m_saveButton, SIGNAL(clicked()), this,
               SLOT(saveWorkspaceGroup()));

    // Remove all existing save algorithms from list
    m_saveMenu->clear();

    // Add some save algorithms
    addSaveMenuOption("SaveNexus", "Nexus");
    addSaveMenuOption("SaveAscii", "ASCII");
    addSaveMenuOption("SaveAscii.1", "ASCII v1");

    // Set the button to show the menu
    m_saveButton->setMenu(m_saveMenu);
  }

  // TODO figure this out later
  /*QString wsName = selectedItems[0]->text(0);
  if (m_ads.doesExist(wsName.toStdString())) {
    m_mantidUI->enableSaveNexus(wsName);
  }*/
}

void QWorkspaceDockView::onClickGroupButton() {
  if (m_groupButton) {
    QString qButtonName = m_groupButton->text();
    if (qButtonName == "Group") {
      presenter->notifyFromView(ViewNotifiable::Flag::GroupWorkspaces);
    } else if (qButtonName == "Ungroup") {
      presenter->notifyFromView(ViewNotifiable::Flag::UngroupWorkspaces);
    }
  }
}

void QWorkspaceDockView::onClickLoad() {
  presenter->notifyFromView(ViewNotifiable::Flag::LoadWorkspace);
}

void QWorkspaceDockView::onClickLiveData() {
  presenter->notifyFromView(ViewNotifiable::Flag::LoadLiveDataWorkspace);
}