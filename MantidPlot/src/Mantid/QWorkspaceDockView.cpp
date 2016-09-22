#include "QWorkspaceDockView.h"
#include "FlowLayout.h"
#include "MantidGroupPlotGenerator.h"
#include "MantidMatrix.h"
#include "MantidTreeWidget.h"
#include "MantidTreeWidgetItem.h"
#include "WorkspaceIcons.h"
#include "pixmaps.h"
#include <MantidGeometry/Instrument.h>
#include <MantidKernel/make_unique.h>
#include <MantidQtAPI/AlgorithmDialog.h>
#include <MantidQtAPI/AlgorithmInputHistory.h>
#include <MantidQtAPI/InterfaceManager.h>
#include <MantidQtMantidWidgets/LineEditWithClear.h>
#include <MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h>
#include <MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h>

#include <MantidAPI/IMDEventWorkspace.h>
#include <MantidAPI/IPeaksWorkspace.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/WorkspaceGroup.h>

#include <QFileDialog>
#include <QHash>
#include <QMenu>
#include <QMessageBox>
#include <QSignalMapper>

#ifdef MAKE_VATES
#include "vtkPVDisplayInformation.h"
#endif

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

namespace {
/// static logger for dock widget
Mantid::Kernel::Logger docklog("MantidDockWidget");

WorkspaceIcons WORKSPACE_ICONS = WorkspaceIcons();
}

QWorkspaceDockView::QWorkspaceDockView(MantidUI *mui, ApplicationWindow *parent)
    : QDockWidget(tr("Workspaces"), parent), m_mantidUI(mui), m_updateCount(0),
      m_treeUpdating(false), m_promptDelete(false),
      m_saveFileType(SaveFileType::Nexus), m_sortCriteria(SortCriteria::ByName),
      m_sortDirection(SortDirection::Ascending) {
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

void QWorkspaceDockView::dropEvent(QDropEvent *de) { m_tree->dropEvent(de); }

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
  connect(
      this, SIGNAL(signalUpdateTree(
                const std::map<std::string, Mantid::API::Workspace_sptr> &)),
      this, SLOT(updateTree(
                const std::map<std::string, Mantid::API::Workspace_sptr> &)),
      Qt::QueuedConnection);

  connect(this, SIGNAL(signalClearView()), this, SLOT(handleClearView()),
          Qt::QueuedConnection);
  connect(m_tree, SIGNAL(itemSelectionChanged()), this,
          SLOT(treeSelectionChanged()));
  connect(m_tree, SIGNAL(itemExpanded(QTreeWidgetItem *)), this,
          SLOT(populateChildData(QTreeWidgetItem *)));
}

void QWorkspaceDockView::setTreeUpdating(const bool state) {
  m_treeUpdating = state;
}

void QWorkspaceDockView::incrementUpdateCount() { m_updateCount.ref(); }

void QWorkspaceDockView::init() {
  auto presenter = boost::make_shared<WorkspacePresenter>(shared_from_this());
  m_presenter = boost::dynamic_pointer_cast<ViewNotifiable>(presenter);
  presenter->init();
}

WorkspacePresenterWN_wptr QWorkspaceDockView::getPresenterWeakPtr() {
  return boost::dynamic_pointer_cast<WorkspacePresenter>(m_presenter);
}

StringList QWorkspaceDockView::getSelectedWorkspaceNames() const {
  QList<QTreeWidgetItem *> items = m_tree->selectedItems();
  StringList names;

  for (auto &item : items)
    names.push_back(item->text(0).toStdString());

  return StringList();
}

Mantid::API::Workspace_sptr QWorkspaceDockView::getSelectedWorkspace() const {
  // TODO: is this method really necessary?
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
QWorkspaceDockView::createAlgorithm(const std::string &algName, int version) {
  Mantid::API::IAlgorithm_sptr alg;
  try {
    alg = Mantid::API::AlgorithmManager::Instance().create(algName, version);
  } catch (...) {
    QMessageBox::warning(m_appParent, "MantidPlot",
                         "Cannot create algorithm \"" +
                             QString::fromStdString(algName) + "\"");
    alg = Mantid::API::IAlgorithm_sptr();
  }
  return alg;
}

void QWorkspaceDockView::showAlgorithm(const std::string &algName,
                                       int version) {
  try {
    auto alg = createAlgorithm(algName, version);
    if (!alg)
      return;

    QHash<QString, QString> presets;
    QStringList enabled;
    QString inputWsProp;
    // If a property was explicitly set show it as preset in the dialog
    const std::vector<Mantid::Kernel::Property *> props = alg->getProperties();
    for (const auto &p : props) {
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

void QWorkspaceDockView::showLoadDialog() { showAlgorithm("Load"); }

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

  try {
    auto alg = createAlgorithm("DeleteWorkspace");

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
        alg->setPropertyValue("Workspace", m->workspaceName().toStdString());
        alg->executeAsync();
      }
    }
  } catch (...) {
    QMessageBox::warning(m_appParent, "",
                         "Could not delete selected workspaces.");
  }
}

void QWorkspaceDockView::clearView() { emit signalClearView(); }

QWorkspaceDockView::SortDirection QWorkspaceDockView::getSortDirection() const {
  return m_sortDirection;
}

QWorkspaceDockView::SortCriteria QWorkspaceDockView::getSortCriteria() const {
  return m_sortCriteria;
}

void QWorkspaceDockView::sortAscending() {
  m_sortDirection = SortDirection::Ascending;
  m_presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);
}

void QWorkspaceDockView::sortDescending() {
  m_sortDirection = SortDirection::Descending;
  m_presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);
}

void QWorkspaceDockView::chooseByName() {
  m_sortCriteria = SortCriteria::ByName;
  m_presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);
}

void QWorkspaceDockView::chooseByLastModified() {
  m_sortCriteria = SortCriteria::ByLastModified;
  m_presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);
}

void QWorkspaceDockView::excludeItemFromSort(MantidTreeWidgetItem *item) {
  static int counter = 1;

  item->setSortPos(counter);

  counter++;
}

QWorkspaceDockView::SortDirection QWorkspaceDockView::getSortDirection() const {
  return SortDirection::Ascending;
}

QWorkspaceDockView::SortCriteria QWorkspaceDockView::getSortCriteria() const {
  return SortCriteria::ByName;
}

void QWorkspaceDockView::sortWorkspaces(SortCriteria criteria,
                                        SortDirection direction) {
  if (isTreeUpdating())
    return;
  m_tree->setSortScheme(criteria == SortCriteria::ByName
                            ? MantidItemSortScheme::ByName
                            : MantidItemSortScheme::ByLastModified);
  m_tree->setSortOrder(direction == SortDirection::Ascending
                           ? Qt::AscendingOrder
                           : Qt::DescendingOrder);
  m_tree->sort();
}

QWorkspaceDockView::SaveFileType QWorkspaceDockView::getSaveFileType() const {
  return m_saveFileType;
}

void QWorkspaceDockView::saveWorkspaceCollection() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::SaveWorkspaceCollection);
}

void QWorkspaceDockView::handleShowSaveAlgorithm() {
  QAction *sendingAction = dynamic_cast<QAction *>(sender());

  if (sendingAction) {
    QString actionName = sendingAction->text();

    if (actionName.compare("Nexus") == 0)
      m_saveFileType = SaveFileType::Nexus;
    else if (actionName.compare("ASCII") == 0)
      m_saveFileType = SaveFileType::ASCII;
    else if (actionName.compare("ASCII v1"))
      m_saveFileType = SaveFileType::ASCIIv1;
  }

  m_presenter->notifyFromView(ViewNotifiable::Flag::SaveSingleWorkspace);
}

QWorkspaceDockView::SaveFileType QWorkspaceDockView::getSaveFileType() const {
  return m_saveFileType;
}

void QWorkspaceDockView::saveWorkspace(const std::string &wsName,
                                       SaveFileType type) {
  int version = -1;
  std::string algorithmName;

  switch (type) {
  case SaveFileType::Nexus:
    algorithmName = "SaveNexus";
    break;
  case SaveFileType::ASCIIv1:
    version = 1;
  case SaveFileType::ASCII:
    algorithmName = "SaveAscii";
    break;
  }

  showAlgorithm(algorithmName, version);
}

void QWorkspaceDockView::saveWorkspaces(const StringList &wsNames) {
  QList<QTreeWidgetItem *> items = m_tree->selectedItems();
  if (items.size() < 2)
    return;

  m_saveFolderDialog->setWindowTitle("Select save folder");
  m_saveFolderDialog->setLabelText(QFileDialog::Accept, "Select");
  auto res = m_saveFolderDialog->exec();
  auto folder = m_saveFolderDialog->selectedFiles()[0].toStdString();

  IAlgorithm_sptr saveAlg = AlgorithmManager::Instance().create("SaveNexus");
  saveAlg->initialize();

  if (res == QFileDialog::Accepted) {
    for (auto &wsName : wsNames) {
      std::string filename = folder + "/" + wsName + ".nxs";
      try {
        saveAlg->setProperty("InputWorkspace", wsName);
        saveAlg->setProperty("Filename", filename);
        saveAlg->execute();
      } catch (std::runtime_error &rte) {
        docklog.error() << "Error saving workspace " << wsName << ": "
                        << rte.what() << '\n';
      }
    }
  }
}

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

void QWorkspaceDockView::setItemIcon(QTreeWidgetItem *item,
                                     const std::string &wsID) {
  try {
    item->setIcon(0, QIcon(WORKSPACE_ICONS.getIcon(wsID)));
  } catch (std::runtime_error &) {
    docklog.warning() << "Cannot find icon for workspace ID '" << wsID << "'\n";
  }
}

/**
* Create the action items associated with the dock
*/
void QWorkspaceDockView::createWorkspaceMenuActions() {
  m_showData = new QAction(tr("Show Data"), this);
  connect(m_showData, SIGNAL(triggered()), m_mantidUI, SLOT(importWorkspace()));

  m_showInst = new QAction(tr("Show Instrument"), this);
  connect(m_showInst, SIGNAL(triggered()), m_mantidUI,
          SLOT(showMantidInstrumentSelected()));

  m_plotSpec = new QAction(tr("Plot Spectrum..."), this);
  connect(m_plotSpec, SIGNAL(triggered()), this, SLOT(plotSpectra()));

  m_plotSpecErr = new QAction(tr("Plot Spectrum with Errors..."), this);
  connect(m_plotSpecErr, SIGNAL(triggered()), this, SLOT(plotSpectraErr()));

  m_colorFill = new QAction(tr("Color Fill Plot"), this);
  connect(m_colorFill, SIGNAL(triggered()), this, SLOT(drawColorFillPlot()));

  m_showDetectors = new QAction(tr("Show Detectors"), this);
  connect(m_showDetectors, SIGNAL(triggered()), this,
          SLOT(showDetectorTable()));

  m_showBoxData = new QAction(tr("Show Box Data Table"), this);
  connect(m_showBoxData, SIGNAL(triggered()), m_mantidUI,
          SLOT(importBoxDataTable()));

  m_showVatesGui = new QAction(tr("Show Vates Simple Interface"), this);
  {
    QIcon icon;
    icon.addFile(
        QString::fromUtf8(":/VatesSimpleGuiViewWidgets/icons/pvIcon.png"),
        QSize(), QIcon::Normal, QIcon::Off);
    m_showVatesGui->setIcon(icon);
  }
  connect(m_showVatesGui, SIGNAL(triggered()), m_mantidUI,
          SLOT(showVatesSimpleInterface()));

  m_showMDPlot = new QAction(tr("Plot MD"), this);
  connect(m_showMDPlot, SIGNAL(triggered()), m_mantidUI, SLOT(showMDPlot()));

  m_showListData = new QAction(tr("List Data"), this);
  connect(m_showListData, SIGNAL(triggered()), m_mantidUI,
          SLOT(showListData()));

  m_showSpectrumViewer = new QAction(tr("Show Spectrum Viewer"), this);
  connect(m_showSpectrumViewer, SIGNAL(triggered()), m_mantidUI,
          SLOT(showSpectrumViewer()));

  m_showSliceViewer = new QAction(tr("Show Slice Viewer"), this);
  {
    QIcon icon;
    icon.addFile(
        QString::fromUtf8(":/SliceViewer/icons/SliceViewerWindow_icon.png"),
        QSize(), QIcon::Normal, QIcon::Off);
    m_showSliceViewer->setIcon(icon);
  }
  connect(m_showSliceViewer, SIGNAL(triggered()), m_mantidUI,
          SLOT(showSliceViewer()));

  m_showLogs = new QAction(tr("Sample Logs..."), this);
  connect(m_showLogs, SIGNAL(triggered()), m_mantidUI,
          SLOT(showLogFileWindow()));

  m_showSampleMaterial = new QAction(tr("Sample Material..."), this);
  connect(m_showSampleMaterial, SIGNAL(triggered()), m_mantidUI,
          SLOT(showSampleMaterialWindow()));

  m_showHist = new QAction(tr("Show History"), this);
  connect(m_showHist, SIGNAL(triggered()), m_mantidUI,
          SLOT(showAlgorithmHistory()));

  m_saveNexus = new QAction(tr("Save Nexus"), this);
  connect(m_saveNexus, SIGNAL(triggered()), m_mantidUI,
          SLOT(saveNexusWorkspace()));

  m_rename = new QAction(tr("Rename"), this);
  connect(m_rename, SIGNAL(triggered()), this, SLOT(renameWorkspace()));

  m_delete = new QAction(tr("Delete"), this);
  connect(m_delete, SIGNAL(triggered()), this, SLOT(deleteWorkspaces()));

  m_showTransposed = new QAction(tr("Show Transposed"), this);
  connect(m_showTransposed, SIGNAL(triggered()), m_mantidUI,
          SLOT(importTransposed()));

  m_convertToMatrixWorkspace =
      new QAction(tr("Convert to MatrixWorkspace"), this);
  m_convertToMatrixWorkspace->setIcon(QIcon(getQPixmap("mantid_matrix_xpm")));
  connect(m_convertToMatrixWorkspace, SIGNAL(triggered()), this,
          SLOT(convertToMatrixWorkspace()));

  m_convertMDHistoToMatrixWorkspace =
      new QAction(tr("Convert to MatrixWorkspace"), this);
  m_convertMDHistoToMatrixWorkspace->setIcon(
      QIcon(getQPixmap("mantid_matrix_xpm")));
  connect(m_convertMDHistoToMatrixWorkspace, SIGNAL(triggered()), this,
          SLOT(convertMDHistoToMatrixWorkspace()));

  m_clearUB = new QAction(tr("Clear UB Matrix"), this);
  connect(m_clearUB, SIGNAL(triggered()), this, SLOT(clearUB()));

  m_plotSurface = new QAction(tr("Plot Surface from Group"), this);
  connect(m_plotSurface, SIGNAL(triggered()), this, SLOT(plotSurface()));

  m_plotContour = new QAction(tr("Plot Contour from Group"), this);
  connect(m_plotContour, SIGNAL(triggered()), this, SLOT(plotContour()));
}

/**
* Create actions for sorting.
*/
void QWorkspaceDockView::createSortMenuActions() {
  chooseByName();
  m_sortMenu = new QMenu(this);

  QAction *m_ascendingSortAction = new QAction("Ascending", this);
  QAction *m_descendingSortAction = new QAction("Descending", this);
  QAction *m_byNameChoice = new QAction("Name", this);
  QAction *m_byLastModifiedChoice = new QAction("Last Modified", this);

  m_ascendingSortAction->setCheckable(true);
  m_ascendingSortAction->setEnabled(true);

  m_descendingSortAction->setCheckable(true);
  m_descendingSortAction->setEnabled(true);

  QActionGroup *sortDirectionGroup = new QActionGroup(m_sortMenu);
  sortDirectionGroup->addAction(m_ascendingSortAction);
  sortDirectionGroup->addAction(m_descendingSortAction);
  sortDirectionGroup->setExclusive(true);
  m_ascendingSortAction->setChecked(true);

  m_byNameChoice->setCheckable(true);
  m_byNameChoice->setEnabled(true);

  m_byLastModifiedChoice->setCheckable(true);
  m_byLastModifiedChoice->setEnabled(true);

  m_sortChoiceGroup = new QActionGroup(m_sortMenu);
  m_sortChoiceGroup->addAction(m_byNameChoice);
  m_sortChoiceGroup->addAction(m_byLastModifiedChoice);
  m_sortChoiceGroup->setExclusive(true);
  m_byNameChoice->setChecked(true);

  connect(m_ascendingSortAction, SIGNAL(triggered()), this,
          SLOT(sortAscending()));
  connect(m_descendingSortAction, SIGNAL(triggered()), this,
          SLOT(sortDescending()));
  connect(m_byNameChoice, SIGNAL(triggered()), this, SLOT(chooseByName()));
  connect(m_byLastModifiedChoice, SIGNAL(triggered()), this,
          SLOT(chooseByLastModified()));

  m_sortMenu->addActions(sortDirectionGroup->actions());
  m_sortMenu->addSeparator();
  m_sortMenu->addActions(m_sortChoiceGroup->actions());
  m_sortButton->setMenu(m_sortMenu);
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
  incrementUpdateCount();
  emit signalUpdateTree(items);
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

MantidTreeWidgetItem *QWorkspaceDockView::addTreeEntry(
    const std::pair<std::string, Mantid::API::Workspace_sptr> &item,
    QTreeWidgetItem *parent) {
  MantidTreeWidgetItem *node =
      new MantidTreeWidgetItem(QStringList(item.first.c_str()), m_tree);
  node->setData(0, Qt::UserRole, QVariant::fromValue(item.second));

  // A a child ID item so that it becomes expandable. Using the correct ID is
  // needed when plotting from non-expanded groups.
  const std::string wsID = item.second->id();
  MantidTreeWidgetItem *idNode =
      new MantidTreeWidgetItem(QStringList(wsID.c_str()), m_tree);
  idNode->setFlags(Qt::NoItemFlags);
  node->addChild(idNode);
  setItemIcon(node, wsID);

  if (parent) {
    parent->addChild(node);
  } else {
    m_tree->addTopLevelItem(node);
  }
  return node;
}

bool QWorkspaceDockView::shouldBeSelected(QString name) const {
  QStringList renamed = m_renameMap.keys(name);
  if (!renamed.isEmpty()) {
    foreach (QString oldName, renamed) {
      if (m_selectedNames.contains(oldName)) {
        return true;
      }
    }
  } else if (m_selectedNames.contains(name)) {
    return true;
  }
  return false;
}

/**
* Add the actions that are appropriate for a MatrixWorkspace
* @param menu :: The menu to store the items
* @param matrixWS :: The workspace related to the menu
*/
void QWorkspaceDockView::addMatrixWorkspaceMenuItems(
    QMenu *menu,
    const Mantid::API::MatrixWorkspace_const_sptr &matrixWS) const {
  // Add all options except plot of we only have 1 value
  menu->addAction(m_showData);
  menu->addAction(m_showInst);
  // Disable the 'show instrument' option if a workspace doesn't have an
  // instrument attached
  m_showInst->setEnabled(matrixWS->getInstrument() &&
                         !matrixWS->getInstrument()->getName().empty());
  menu->addSeparator();
  menu->addAction(m_plotSpec);
  menu->addAction(m_plotSpecErr);

  // Don't plot a spectrum if only one X value
  m_plotSpec->setEnabled(matrixWS->blocksize() > 1);
  m_plotSpecErr->setEnabled(matrixWS->blocksize() > 1);

  menu->addAction(m_showSpectrumViewer); // The 2D spectrum viewer

  menu->addAction(m_colorFill);
  // Show the color fill plot if you have more than one histogram
  m_colorFill->setEnabled(
      (matrixWS->axes() > 1 && matrixWS->getNumberHistograms() > 1));
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
void QWorkspaceDockView::addMDEventWorkspaceMenuItems(
    QMenu *menu, const Mantid::API::IMDEventWorkspace_const_sptr &WS) const {
  Q_UNUSED(WS);

  // menu->addAction(m_showBoxData); // Show MD Box data (for debugging only)
  menu->addAction(m_showVatesGui); // Show the Vates simple interface
  if (!MantidQt::API::InterfaceManager::hasVatesLibraries()) {
    m_showVatesGui->setEnabled(false);
#ifdef MAKE_VATES
  } else if (!vtkPVDisplayInformation::SupportsOpenGLLocally()) {
    m_showVatesGui->setEnabled(false);
#endif
  } else {
    std::size_t nDim = WS->getNonIntegratedDimensions().size();
    m_showVatesGui->setEnabled(nDim >= 3 && nDim < 5);
  }
  menu->addAction(m_showSliceViewer); // The 2D slice viewer
  menu->addAction(m_showHist);        // Algorithm history
  menu->addAction(m_showListData);    // Show data in table
  menu->addAction(m_showLogs);
}

void QWorkspaceDockView::addMDHistoWorkspaceMenuItems(
    QMenu *menu, const Mantid::API::IMDWorkspace_const_sptr &WS) const {
  Q_UNUSED(WS);
  menu->addAction(m_showHist);     // Algorithm history
  menu->addAction(m_showVatesGui); // Show the Vates simple interface
  if (!MantidQt::API::InterfaceManager::hasVatesLibraries()) {
    m_showVatesGui->setEnabled(false);
#ifdef MAKE_VATES
  } else if (!vtkPVDisplayInformation::SupportsOpenGLLocally()) {
    m_showVatesGui->setEnabled(false);
#endif
  } else {
    std::size_t nDim = WS->getNonIntegratedDimensions().size();
    m_showVatesGui->setEnabled(nDim >= 3 && nDim < 5);
  }
  menu->addAction(m_showSliceViewer); // The 2D slice viewer
  menu->addAction(m_showMDPlot);      // A plot of intensity vs bins
  menu->addAction(m_showListData);    // Show data in table
  menu->addAction(m_convertMDHistoToMatrixWorkspace);
  menu->addAction(m_showLogs);
}

/** Add the actions that are appropriate for a PeaksWorkspace
* @param menu :: The menu to store the items
* @param WS :: The workspace related to the menu
*/
void QWorkspaceDockView::addPeaksWorkspaceMenuItems(
    QMenu *menu, const Mantid::API::IPeaksWorkspace_const_sptr &WS) const {
  Q_UNUSED(WS);
  menu->addAction(m_showData);
  menu->addSeparator();
  menu->addAction(m_showDetectors);
  menu->addAction(m_showHist);
}

/**
* Add the actions that are appropriate for a WorkspaceGroup
* @param menu :: The menu to store the items
* @param groupWS :: [input] Workspace group related to the menu
*/
void QWorkspaceDockView::addWorkspaceGroupMenuItems(
    QMenu *menu, const WorkspaceGroup_const_sptr &groupWS) const {
  m_plotSpec->setEnabled(true);
  menu->addAction(m_plotSpec);
  m_plotSpecErr->setEnabled(true);
  menu->addAction(m_plotSpecErr);
  menu->addAction(m_colorFill);
  m_colorFill->setEnabled(true);

  // If appropriate, add "plot surface" and "plot contour" options
  // Only add these if:
  // - there are >2 workspaces in group
  // - all are MatrixWorkspaces (otherwise they can't be plotted)
  // - only one group is selected
  if (m_tree->selectedItems().size() == 1) {
    if (groupWS && groupWS->getNumberOfEntries() > 2) {
      if (MantidGroupPlotGenerator::groupIsAllMatrixWorkspaces(groupWS)) {
        menu->addAction(m_plotSurface);
        m_plotSurface->setEnabled(true);
        menu->addAction(m_plotContour);
        m_plotContour->setEnabled(true);
      }
    }
  }

  menu->addSeparator();
  menu->addAction(m_saveNexus);
}

/**
* Add the actions that are appropriate for a MatrixWorkspace
* @param menu :: The menu to store the items
*/
void QWorkspaceDockView::addTableWorkspaceMenuItems(QMenu *menu) const {
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
void QWorkspaceDockView::addClearMenuItems(QMenu *menu, const QString &wsName) {
  QMenu *clearMenu = new QMenu(tr("Clear Options"), this);

  m_clearUB->setEnabled(hasUBMatrix(wsName.toStdString()));

  clearMenu->addAction(m_clearUB);
  menu->addMenu(clearMenu);
}

bool QWorkspaceDockView::hasUBMatrix(const std::string &wsName) {
  bool hasUB = false;
  auto alg = createAlgorithm("HasUB");

  if (alg) {
    alg->setLogging(false);
    alg->setPropertyValue("Workspace", wsName);
    // TODO: may need an executeAsync with a wait as before check MantidUI.cpp
    alg->execute();
    hasUB = alg->getProperty("HasUB");
  }
  return hasUB;
}

/**
* Adds an algorithm to the save menu.
*
* @param algorithmString Algorithm string in format ALGO_NAME.VERSION or
* ALGO_NAME
* @param menuEntryName Text to be shown in menu
*/
void QWorkspaceDockView::addSaveMenuOption(QString algorithmString,
                                           QString menuEntryName) {
  // Default to algo string if no entry name given
  if (menuEntryName.isEmpty())
    menuEntryName = algorithmString;

  // Create the action and add data
  QAction *saveAction = new QAction(menuEntryName, this);
  saveAction->setData(QVariant(algorithmString));

  // Connect the tigger slot to show algorithm dialog
  connect(saveAction, SIGNAL(triggered()), this,
          SLOT(handleShowSaveAlgorithm()));

  // Add it to the menu
  m_saveMenu->addAction(saveAction);
}

/**
* Filter workspaces based on the string provided
* @param text : the string to filter on.
*/
void QWorkspaceDockView::filterWorkspaceTree(const QString &text) {
  m_filteredText = text.toStdString();
  m_presenter->notifyFromView(ViewNotifiable::Flag::FilterWorkspaces);
}

void QWorkspaceDockView::onClickDeleteWorkspaces() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::DeleteWorkspaces);
}

void QWorkspaceDockView::clickedWorkspace(QTreeWidgetItem *item, int) {
  Q_UNUSED(item);
}

void QWorkspaceDockView::workspaceSelected() {
  auto selectedNames = getSelectedWorkspaceNames();
  if (selectedNames.empty())
    return;

  // If there are multiple workspaces selected group and save as Nexus
  if (selectedNames.size() > 1) {
    connect(m_saveButton, SIGNAL(clicked()), this,
            SLOT(saveWorkspaceCollection()));

    // Don't display as a group
    m_saveButton->setMenu(NULL);
  } else {
    // Don't run the save group function when clicked
    disconnect(m_saveButton, SIGNAL(clicked()), this,
               SLOT(saveWorkspaceCollection()));

    // Remove all existing save algorithms from list
    m_saveMenu->clear();

    // Add some save algorithms
    addSaveMenuOption("SaveNexus", "Nexus");
    addSaveMenuOption("SaveAscii", "ASCII");
    addSaveMenuOption("SaveAscii.1", "ASCII v1");

    // Set the button to show the menu
    m_saveButton->setMenu(m_saveMenu);
  }

  auto wsName = selectedNames[0];
  // TODO: Wire signal correctly in applicationwindow
  emit enableSaveNexus(QString::fromStdString(wsName));
}

void QWorkspaceDockView::onClickGroupButton() {
  if (m_groupButton) {
    QString qButtonName = m_groupButton->text();
    if (qButtonName == "Group") {
      m_presenter->notifyFromView(ViewNotifiable::Flag::GroupWorkspaces);
    } else if (qButtonName == "Ungroup") {
      m_presenter->notifyFromView(ViewNotifiable::Flag::UngroupWorkspaces);
    }
  }
}

void QWorkspaceDockView::onClickLoad() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::LoadWorkspace);
}

void QWorkspaceDockView::onClickLiveData() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::LoadLiveDataWorkspace);
}

// Asynchronous signal handlers
void QWorkspaceDockView::handleUpdateTree(
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

void QWorkspaceDockView::handleClearView() { m_tree->clear(); }

// Context Menu Methods
void QWorkspaceDockView::popupMenu(const QPoint &pos) {
  m_menuPosition = pos;
  m_presenter->notifyFromView(
      ViewNotifiable::Flag::PopulateAndShowWorkspaceContextMenu);
}

void QWorkspaceDockView::popupContextMenu() {
  QTreeWidgetItem *treeItem = m_tree->itemAt(m_menuPosition);
  selectedWsName = "";
  if (treeItem)
    selectedWsName = treeItem->text(0);
  else
    m_tree->selectionModel()->clear();

  QMenu *menu(nullptr);

  // If no workspace is here then have load raw and dae
  if (selectedWsName.isEmpty())
    menu = m_loadMenu;
  else { // else show instrument, sample logs and delete
    // Fresh menu
    menu = new QMenu(this);
    menu->setObjectName("WorkspaceContextMenu");
    auto mantidTreeItem = dynamic_cast<MantidTreeWidgetItem *>(treeItem);
    auto ws = mantidTreeItem->data(0, Qt::UserRole)
                  .value<Mantid::API::Workspace_const_sptr>();

    // Add the items that are appropriate for the type
    if (auto matrixWS =
            boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(
                ws)) {
      addMatrixWorkspaceMenuItems(menu, matrixWS);
    } else if (auto mdeventWS =
                   boost::dynamic_pointer_cast<const IMDEventWorkspace>(ws)) {
      addMDEventWorkspaceMenuItems(menu, mdeventWS);
    } else if (auto mdWS =
                   boost::dynamic_pointer_cast<const IMDWorkspace>(ws)) {
      addMDHistoWorkspaceMenuItems(menu, mdWS);
    } else if (auto peaksWS =
                   boost::dynamic_pointer_cast<const IPeaksWorkspace>(ws)) {
      addPeaksWorkspaceMenuItems(menu, peaksWS);
    } else if (auto groupWS =
                   boost::dynamic_pointer_cast<const WorkspaceGroup>(ws)) {
      addWorkspaceGroupMenuItems(menu, groupWS);
    } else if (boost::dynamic_pointer_cast<const Mantid::API::ITableWorkspace>(
                   ws)) {
      addTableWorkspaceMenuItems(menu);
    }
    addClearMenuItems(menu, selectedWsName);

    // Get the names of the programs for the send to option
    std::vector<std::string> programNames =
        (Mantid::Kernel::ConfigService::Instance().getKeys(
            "workspace.sendto.name"));
    bool firstPass(true);
    // Check to see if any options aren't visible
    for (auto &programName : programNames) {
      std::string visible = Mantid::Kernel::ConfigService::Instance().getString(
          "workspace.sendto." + programName + ".visible");
      std::string target = Mantid::Kernel::ConfigService::Instance().getString(
          "workspace.sendto." + programName + ".target");
      if (Mantid::Kernel::ConfigService::Instance().isExecutable(target) &&
          visible == "Yes") {
        bool compatible(true);
        std::string saveUsing(
            Mantid::Kernel::ConfigService::Instance().getString(
                "workspace.sendto." + programName + ".saveusing"));
        try {
          Mantid::API::IAlgorithm_sptr alg =
              Mantid::API::AlgorithmManager::Instance().create(saveUsing);
          alg->setPropertyValue("InputWorkspace", selectedWsName.toStdString());
        } catch (std::exception &) {
          compatible = false;
        }
        if (compatible) {
          if (firstPass) {
            m_saveToProgram = new QMenu(tr("Send to"), this);
            menu->addMenu(m_saveToProgram);

            // Sub-menu for program list
            m_programMapper = new QSignalMapper(this);
          }
          QString name = QString::fromStdString(programName);
          // Setup new menu option for the program
          m_program = new QAction(name, this);
          connect(m_program, SIGNAL(triggered()), m_programMapper, SLOT(map()));
          // Send name of program when clicked
          m_programMapper->setMapping(m_program, name);
          m_saveToProgram->addAction(m_program);

          // Set first pass to false so that it doesn't set up another menu
          // entry for all programs.
          firstPass = false;
        }
      }
    }

    // Tell the button what to listen for and what to do once clicked (if there
    // is anything to connect it will be set to false)
    if (!firstPass)
      connect(m_programMapper, SIGNAL(mapped(const QString &)), this,
              SLOT(saveToProgram(const QString &)));

    // Rename is valid for all workspace types
    menu->addAction(m_rename);
    // separate delete
    menu->addSeparator();
    menu->addAction(m_delete);
  }

  // Show the menu at the cursor's current position
  menu->popup(QCursor::pos());
}

void QWorkspaceDockView::showWorkspaceData() {}
void QWorkspaceDockView::showInstrumentView() {}

/// Plots a single spectrum from each selected workspace
void QWorkspaceDockView::onClickPlotSpectra() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::PlotSpectrum);
}
/// Plots a single spectrum from each selected workspace with errors
void QWorkspaceDockView::onClickPlotSpectraErr() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::PlotSpectrumWithErrors);
}

void QWorkspaceDockView::plotSpectrum(bool showErrors) {
  const auto userInput = m_tree->chooseSpectrumFromSelected();
  // An empty map will be returned if the user clicks cancel in the spectrum
  // selection
  if (userInput.plots.empty())
    return;

  bool spectrumPlot(true), clearWindow(false);
  MultiLayer *window(NULL);
  // TODO: Replace with signal?
  m_mantidUI->plot1D(userInput.plots, spectrumPlot,
                     MantidQt::DistributionDefault, showErrors, window,
                     clearWindow, userInput.waterfall);
}

/**
* Draw a color fill plot of the workspaces that are currently selected.
* NOTE: The drawing of 2D plots is currently intimately linked with MantidMatrix
* meaning
* that one of these must be generated first!
*/
void QWorkspaceDockView::onClickDrawColorFillPlot() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ShowColourFillPlot);
}

void QWorkspaceDockView::showColourFillPlot() {
  // Get the selected workspaces
  auto items = m_tree->selectedItems();
  if (items.empty())
    return;

  // Extract child workspace names from any WorkspaceGroups selected.
  // Use a list to preserve workspace order.
  QStringList allWsNames;

  for (auto &item : items) {
    auto mItem = dynamic_cast<MantidTreeWidgetItem *>(item);
    auto ws = item->data(0, Qt::UserRole).value<Workspace_sptr>();

    if (auto wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(ws)) {
      for (auto &name : wsGroup->getNames())
        allWsNames.append(QString::fromStdString(name));
    } else
      allWsNames.append(item->text(0));
  }

  // remove duplicate workspace entries
  allWsNames.removeDuplicates();

  emit signalDrawColourFillPlot(allWsNames);
}

void QWorkspaceDockView::onClickShowDetectorTable() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ShowDetectorsTable);
}

void QWorkspaceDockView::showDetectorsTable() {
  // get selected workspace
  auto ws = getSelectedWorkspaceNames()[0];
  // TODO: wire this signal with MantidUI
  emit signalCreateDetectorTable(QString::fromStdString(ws), std::vector<int>(),
                                 false);
}

void QWorkspaceDockView::showBoxDataTable() {}
void QWorkspaceDockView::showVatesGUI() {}
void QWorkspaceDockView::showMDPlot() {}
void QWorkspaceDockView::showListData() {}
void QWorkspaceDockView::showSpectrumViewer() {}
void QWorkspaceDockView::showSliceViewer() {}
void QWorkspaceDockView::showLogs() {}
void QWorkspaceDockView::showSampleMaterialWindow() {}
void QWorkspaceDockView::showAlgorithmHistory() {}
void QWorkspaceDockView::showTransposed() {}

/**
* Convert selected TableWorkspace to a MatrixWorkspace.
*/
void QWorkspaceDockView::onClickConvertToMatrixWorkspace() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ConvertToMatrixWorkspace);
}

/**
* Convert selected MDHistoWorkspace to a MatrixWorkspace.
*/
void QWorkspaceDockView::onClickConvertMDHistoToMatrixWorkspace() {
  m_presenter->notifyFromView(
      ViewNotifiable::Flag::ConvertMDHistoToMatrixWorkspace);
}

void QWorkspaceDockView::convertToMatrixWorkspace() {
  showAlgorithm("ConvertTableToMatrixWorkspace");
}

void QWorkspaceDockView::convertMDHistoToMatrixWorkspace() {
  showAlgorithm("ConvertMDHistoToMatrixWorkspace");
}

/**
* Handler for the clear the UB matrix event.
*/
void QWorkspaceDockView::onClickClearUB() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ClearUBMatrix);
}

void QWorkspaceDockView::clearUBMatrix() {
  auto wsNames = getSelectedWorkspaceNames();

  for (auto &ws : wsNames) {
    auto alg = createAlgorithm("ClearUB");
    if (alg) {
      alg->initialize();
      alg->setPropertyValue("Workspace", ws);
      alg->executeAsync();
    } else
      break;
  }
}

/**
* Create a 3D surface plot from the selected workspace group
*/
void QWorkspaceDockView::onClickPlotSurface() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ShowSurfacePlot);
}

void QWorkspaceDockView::showSurfacePlot() {
  // find the workspace group clicked on
  auto items = m_tree->selectedItems();
  if (!items.empty()) {
    auto data = items[0]->data(0, Qt::UserRole).value<Workspace_sptr>();
    const auto wsGroup =
        boost::dynamic_pointer_cast<const WorkspaceGroup>(data);
    if (wsGroup) {
      auto options =
          m_tree->chooseSurfacePlotOptions(wsGroup->getNumberOfEntries());

      // TODO: Figure out how to get rid of MantidUI dependency here.
      auto plotter =
          Mantid::Kernel::make_unique<MantidGroupPlotGenerator>(m_mantidUI);
      plotter->plotSurface(wsGroup, options);
    }
  }
}

/**
* Create a contour plot from the selected workspace group
*/
void QWorkspaceDockView::onClickPlotContour() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ShowContourPlot);
}

void QWorkspaceDockView::showContourPlot() {
  auto items = m_tree->selectedItems();
  if (!items.empty()) {
    auto data = items[0]->data(0, Qt::UserRole).value<Workspace_sptr>();
    const auto wsGroup =
        boost::dynamic_pointer_cast<const WorkspaceGroup>(data);
    if (wsGroup) {
      auto options =
          m_tree->chooseContourPlotOptions(wsGroup->getNumberOfEntries());

      // TODO: Figure out how to remove the MantidUI dependency
      auto plotter =
          Mantid::Kernel::make_unique<MantidGroupPlotGenerator>(m_mantidUI);
      plotter->plotContour(wsGroup, options);
    }
  }
}
