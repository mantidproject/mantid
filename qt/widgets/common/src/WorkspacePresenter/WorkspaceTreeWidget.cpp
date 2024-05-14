// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidget.h"
#include "MantidGeometry/Instrument.h"

#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidQtWidgets/Common/FlowLayout.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/LineEditWithClear.h"
#include "MantidQtWidgets/Common/MantidDisplayBase.h"
#include "MantidQtWidgets/Common/MantidTreeWidget.h"
#include "MantidQtWidgets/Common/MantidTreeWidgetItem.h"
#include "MantidQtWidgets/Common/WorkspaceIcons.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/ADSAdapter.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspacePresenter.h"
#include "MantidQtWidgets/Common/pixmaps.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <Poco/Path.h>

#include <memory>

#include <QFileDialog>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalMapper>

using namespace MantidQt::API;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
/// static logger for dock widget
Mantid::Kernel::Logger docklog("MantidDockWidget");

WorkspaceIcons WORKSPACE_ICONS = WorkspaceIcons();
} // namespace

namespace MantidQt::MantidWidgets {

WorkspaceTreeWidget::WorkspaceTreeWidget(MantidDisplayBase *mdb, bool viewOnly, QWidget *parent)
    : QWidget(parent), m_mantidDisplayModel(mdb), m_viewOnly(viewOnly), m_updateCount(0), m_treeUpdating(false),
      m_promptDelete(false), m_saveFileType(SaveFileType::Nexus), m_sortCriteria(SortCriteria::ByName),
      m_sortDirection(SortDirection::Ascending), m_mutex(QMutex::Recursive) {
  setObjectName("exploreMantid"); // this is needed for QMainWindow::restoreState()
  m_saveMenu = new QMenu(this);

  setupWidgetLayout();

  setupLoadButtonMenu();

  // Dialog box used for user to specify folder to save multiple workspaces into
  m_saveFolderDialog = new QFileDialog(this);
  m_saveFolderDialog->setFileMode(QFileDialog::Directory);
  m_saveFolderDialog->setOption(QFileDialog::ShowDirsOnly);

  // To be able to use them in queued signals they need to be registered
  static bool registered_addtional_types = false;
  if (!registered_addtional_types) {
    registered_addtional_types = true;
    qRegisterMetaType<TopLevelItems>();
  }

  // SET UP SORT
  createSortMenuActions();
  createWorkspaceMenuActions();

  setupConnections();

  m_tree->setDragEnabled(true);

  auto presenter = std::make_shared<WorkspacePresenter>(this);
  m_presenter = std::dynamic_pointer_cast<ViewNotifiable>(presenter);
  presenter->init();

  if (m_viewOnly)
    hideButtonToolbar();
}

WorkspaceTreeWidget::~WorkspaceTreeWidget() = default;

/**
 * Accept a drag drop event and process the data appropriately
 * @param de :: The drag drop event
 */
void WorkspaceTreeWidget::dropEvent(QDropEvent *de) { m_tree->dropEvent(de); }

void WorkspaceTreeWidget::setupWidgetLayout() {
  m_tree = new MantidTreeWidget(m_mantidDisplayModel, this);
  m_tree->setHeaderLabel("Workspaces");

  auto *buttonLayout = new FlowLayout();
  m_loadButton = new QPushButton("Load");
  m_loadButton->setToolTip("Load a file or live data");
  m_saveButton = new QPushButton("Save");
  m_saveButton->setToolTip("Save the selected workspaces");
  m_deleteButton = new QPushButton("Delete");
  m_deleteButton->setToolTip("Delete the selected workspaces");
  m_clearButton = new QPushButton("Clear");
  m_clearButton->setToolTip("Delete all workspaces");
  m_groupButton = new QPushButton("Group");
  m_groupButton->setToolTip("Group together two or more selected workspaces");
  m_sortButton = new QPushButton("Sort");
  m_sortButton->setToolTip("Sort all workspaces by name, size, or the last time they were modified");

  m_groupButton->setEnabled(false);
  m_deleteButton->setEnabled(false);
  m_clearButton->setEnabled(false);
  m_saveButton->setEnabled(false);

  buttonLayout->addWidget(m_loadButton);
  buttonLayout->addWidget(m_deleteButton);
  buttonLayout->addWidget(m_clearButton);
  buttonLayout->addWidget(m_groupButton);
  buttonLayout->addWidget(m_sortButton);
  buttonLayout->addWidget(m_saveButton);

  m_workspaceFilter = new MantidQt::MantidWidgets::LineEditWithClear();
  m_workspaceFilter->setPlaceholderText("Filter Workspaces");
  m_workspaceFilter->setToolTip("Type here to filter the workspaces");

  auto *layout = new QVBoxLayout();
  layout->setMargin(0);
  layout->addLayout(buttonLayout);
  layout->addWidget(m_workspaceFilter);
  layout->addWidget(m_tree);
  this->setLayout(layout);
}

void WorkspaceTreeWidget::setupLoadButtonMenu() {
  m_loadMenu = new QMenu(this);

  QAction *loadFileAction = new QAction("File", this);
  QAction *liveDataAction = new QAction("Live Data", this);
  connect(loadFileAction, SIGNAL(triggered()), this, SLOT(onClickLoad()));
  connect(liveDataAction, SIGNAL(triggered()), this, SLOT(onClickLiveData()));

  m_loadMenu->addAction(loadFileAction);
  m_loadMenu->addAction(liveDataAction);
  m_loadButton->setMenu(m_loadMenu);
}

void WorkspaceTreeWidget::setupConnections() {
  connect(m_workspaceFilter, SIGNAL(textChanged(const QString &)), this, SLOT(filterWorkspaceTree(const QString &)));
  connect(m_deleteButton, SIGNAL(clicked()), this, SLOT(onClickDeleteWorkspaces()));
  connect(m_clearButton, SIGNAL(clicked()), this, SLOT(onClickClearWorkspaces()));
  connect(m_tree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(clickedWorkspace(QTreeWidgetItem *, int)));
  connect(m_tree, SIGNAL(itemSelectionChanged()), this, SLOT(workspaceSelected()));
  connect(m_groupButton, SIGNAL(clicked()), this, SLOT(onClickGroupButton()));

  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));
  connect(this, SIGNAL(signalUpdateTree(const TopLevelItems &)), this, SLOT(handleUpdateTree(const TopLevelItems &)),
          Qt::QueuedConnection);

  connect(this, SIGNAL(signalClearView()), this, SLOT(handleClearView()), Qt::QueuedConnection);
  connect(m_tree, SIGNAL(itemSelectionChanged()), this, SLOT(onTreeSelectionChanged()));
  connect(m_tree, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(populateChildData(QTreeWidgetItem *)));
}

/**
 * Flips the flag indicating whether a tree update is in progress. Actions such
 * as sorting
 * are disabled while an update is in progress.
 * @param state The required state for the flag
 */
void WorkspaceTreeWidget::setTreeUpdating(const bool state) { m_treeUpdating = state; }

void WorkspaceTreeWidget::incrementUpdateCount() { m_updateCount.ref(); }

WorkspacePresenterWN_wptr WorkspaceTreeWidget::getPresenterWeakPtr() {
  return std::dynamic_pointer_cast<WorkspacePresenter>(m_presenter);
}

/** Returns the names of the selected workspaces
 *   in the dock.
 */
StringList WorkspaceTreeWidget::getSelectedWorkspaceNames() const {
  auto items = m_tree->selectedItems();
  StringList names;
  names.reserve(static_cast<size_t>(items.size()));
  std::transform(items.cbegin(), items.cend(), std::back_inserter(names),
                 [](auto const &item) { return item->text(0).toStdString(); });

  return names;
}

QStringList WorkspaceTreeWidget::getSelectedWorkspaceNamesAsQList() const {
  auto items = m_tree->selectedItems();
  QStringList names;

  for (auto &item : items) {
    names.append(item->text(0));
  }
  return names;
}

/** Returns a pointer to the selected workspace (the first if multiple
 *   workspaces selected)
 */
Mantid::API::Workspace_sptr WorkspaceTreeWidget::getSelectedWorkspace() const {
  auto items = m_tree->selectedItems();
  auto data = items[0]->data(0, Qt::UserRole).value<Workspace_sptr>();

  return data;
}

bool WorkspaceTreeWidget::askUserYesNo(const std::string &caption, const std::string &message) const {
  return QMessageBox::question(parentWidget(), QString::fromStdString(caption), QString::fromStdString(message),
                               QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
}

void WorkspaceTreeWidget::showCriticalUserMessage(const std::string &caption, const std::string &message) const {
  QMessageBox::critical(parentWidget(), QString::fromStdString(caption), QString::fromStdString(message));
}

void WorkspaceTreeWidget::onLoadAccept() {
  QObject *sender = QObject::sender();
  const auto *dlg = reinterpret_cast<MantidQt::API::AlgorithmDialog *>(sender);
  if (!dlg)
    return; // should never happen

  QString fn = MantidQt::API::AlgorithmInputHistory::Instance().previousInput("Load", "Filename");

  m_mantidDisplayModel->updateRecentFilesList(fn);
}

void WorkspaceTreeWidget::showLoadDialog() {
  QMetaObject::invokeMethod(dynamic_cast<QObject *>(m_mantidDisplayModel), "showAlgorithmDialog", Qt::QueuedConnection,
                            Q_ARG(QString, "Load"));
}

void WorkspaceTreeWidget::showLiveDataDialog() {
  QMetaObject::invokeMethod(dynamic_cast<QObject *>(m_mantidDisplayModel), "showAlgorithmDialog", Qt::QueuedConnection,
                            Q_ARG(QString, "StartLiveData"));
}

void WorkspaceTreeWidget::renameWorkspace() { m_presenter->notifyFromView(ViewNotifiable::Flag::RenameWorkspace); }

void WorkspaceTreeWidget::showRenameDialog(const StringList &wsNames) {
  QStringList names;

  for (const auto &ws : wsNames)
    names.append(QString::fromStdString(ws));

  m_mantidDisplayModel->renameWorkspace(names);
}

/**
 * Save the old and the new name in m_renameMap. This is needed to restore
 * selection
 *   of the renamed workspace (if it was selected before renaming).
 * @param oldName :: Old name of a renamed workspace.
 * @param newName :: New name of a renamed workspace.
 */
void WorkspaceTreeWidget::recordWorkspaceRename(const std::string &oldName, const std::string &newName) {
  QString qs_oldName = QString::fromStdString(oldName);
  QString qs_newName = QString::fromStdString(newName);

  QMutexLocker renameMapLock(&m_mutex);
  // check if old_name has been recently a new name
  QList<QString> oldNames = m_renameMap.keys(qs_oldName);
  // non-empty list of oldNames become new_name
  if (!oldNames.isEmpty()) {
    for (const auto &name : oldNames)
      m_renameMap[name] = qs_newName;
  } else {
    // record a new rename pair
    m_renameMap[qs_oldName] = qs_newName;
  }
}

void WorkspaceTreeWidget::refreshWorkspaces() { m_presenter->notifyFromView(ViewNotifiable::Flag::RefreshWorkspaces); }

void WorkspaceTreeWidget::enableDeletePrompt(bool enable) { m_promptDelete = enable; }

bool WorkspaceTreeWidget::isPromptDelete() const { return m_promptDelete; }

bool WorkspaceTreeWidget::deleteConfirmation() const {
  return askUserYesNo("Delete Workspaces", "Are you sure you want to delete the selected Workspaces?\n\nThis prompt "
                                           "can be disabled from:\nFile->Settings->General");
}

void WorkspaceTreeWidget::deleteWorkspaces(const StringList &wsNames) {
  QStringList names;
  for (const auto &ws : wsNames)
    names.append(QString::fromStdString(ws));
  m_mantidDisplayModel->deleteWorkspaces(names);
}

void WorkspaceTreeWidget::clearView() { emit signalClearView(); }

void WorkspaceTreeWidget::sortAscending() {
  m_sortDirection = SortDirection::Ascending;
  m_presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);
}

void WorkspaceTreeWidget::sortDescending() {
  m_sortDirection = SortDirection::Descending;
  m_presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);
}

void WorkspaceTreeWidget::chooseByName() {
  m_sortCriteria = SortCriteria::ByName;
  m_presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);
}

void WorkspaceTreeWidget::chooseByLastModified() {
  m_sortCriteria = SortCriteria::ByLastModified;
  m_presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);
}

void WorkspaceTreeWidget::chooseByMemorySize() {
  m_sortCriteria = SortCriteria::ByMemorySize;
  m_presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);
}

void WorkspaceTreeWidget::excludeItemFromSort(MantidTreeWidgetItem *item) {
  static int counter = 1;

  item->setSortPos(counter);

  counter++;
}

WorkspaceTreeWidget::SortDirection WorkspaceTreeWidget::getSortDirection() const { return m_sortDirection; }

WorkspaceTreeWidget::SortCriteria WorkspaceTreeWidget::getSortCriteria() const { return m_sortCriteria; }

void WorkspaceTreeWidget::sortWorkspaces(SortCriteria criteria, SortDirection direction) {
  if (isTreeUpdating())
    return;
  m_tree->setSortScheme(whichCriteria(criteria));
  m_tree->setSortOrder(direction == SortDirection::Ascending ? Qt::AscendingOrder : Qt::DescendingOrder);
  m_tree->sort();
}

MantidQt::MantidWidgets::MantidItemSortScheme WorkspaceTreeWidget::whichCriteria(SortCriteria criteria) {
  switch (criteria) {
  case SortCriteria::ByName:
    return MantidItemSortScheme::ByName;
  case SortCriteria::ByLastModified:
    return MantidItemSortScheme::ByLastModified;
  case SortCriteria::ByMemorySize:
    return MantidItemSortScheme::ByMemorySize;
  default:
    // Handle if someone adds a new Enum and it falls through by defaulting to
    // name
    return MantidItemSortScheme::ByName;
  }
}

void WorkspaceTreeWidget::saveWorkspaceCollection() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::SaveWorkspaceCollection);
}

void WorkspaceTreeWidget::handleShowSaveAlgorithm() {
  const QAction *sendingAction = dynamic_cast<QAction *>(sender());

  if (sendingAction) {
    QString actionName = sendingAction->text();

    if (actionName.compare("Nexus") == 0)
      m_saveFileType = SaveFileType::Nexus;
    else if (actionName.compare("ASCII") == 0)
      m_saveFileType = SaveFileType::ASCII;
  }

  m_presenter->notifyFromView(ViewNotifiable::Flag::SaveSingleWorkspace);
}

WorkspaceTreeWidget::SaveFileType WorkspaceTreeWidget::getSaveFileType() const { return m_saveFileType; }

void WorkspaceTreeWidget::saveWorkspace(const std::string &wsName, SaveFileType type) {
  QHash<QString, QString> presets;
  if (!wsName.empty()) {
    presets["InputWorkspace"] = QString::fromStdString(wsName);
  }
  int version = -1;
  std::string algorithmName;

  switch (type) {
  case SaveFileType::Nexus:
    algorithmName = "SaveNexus";
    break;
  case SaveFileType::ASCII:
    algorithmName = "SaveAscii";
    break;
  }

  m_mantidDisplayModel->showAlgorithmDialog(QString::fromStdString(algorithmName), presets, nullptr, version);
}

void WorkspaceTreeWidget::saveWorkspaces(const StringList &wsNames) {
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
      } catch (std::exception &ex) {
        docklog.error() << "Error saving workspace " << wsName << ": " << ex.what() << '\n';
      }
    }
  }
}

std::string WorkspaceTreeWidget::getFilterText() const { return m_workspaceFilter->text().toStdString(); }

void WorkspaceTreeWidget::filterWorkspaces(const std::string &filterText) {
  const QString text = QString::fromStdString(filterText).trimmed();
  QRegExp filterRegEx(text, Qt::CaseInsensitive);

  // show all items
  QTreeWidgetItemIterator unhideIter(m_tree);
  while (*unhideIter) {
    (*unhideIter)->setHidden(false);
    ++unhideIter;
  }

  int hiddenCount = 0;
  if (!text.isEmpty()) {
    QList<QTreeWidgetItem *> visibleGroups;
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
            if (workspace->isGroup()) {
              // I am a group, I will want my children to be visible
              // but I cannot do that until this iterator has finished
              // store this pointer in a list for processing later
              visibleGroups.append(item);
              item->setHidden(false);
            }

            if (item->parent() == nullptr) {
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
    for (auto group : visibleGroups) {
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
    QString headerString = QString("Workspaces (%1 filtered)").arg(QString::number(hiddenCount));
    m_tree->headerItem()->setText(0, headerString);
  } else {
    m_tree->headerItem()->setText(0, "Workspaces");
  }
}

/**
 * Set tree item's icon based on the ID of the workspace.
 * @param item :: A workspace tree item.
 * @param wsID :: An icon type code.
 */
void WorkspaceTreeWidget::setItemIcon(QTreeWidgetItem *item, const std::string &wsID) {
  try {
    item->setIcon(0, QIcon(WORKSPACE_ICONS.getIcon(wsID)));
  } catch (std::runtime_error &) {
    docklog.warning() << "Cannot find icon for workspace ID '" << wsID << "'\n";
  }
}

/**
 * Create the action items associated with the dock
 */
void WorkspaceTreeWidget::createWorkspaceMenuActions() {
  m_showData = new QAction(tr("Show Data"), this);
  connect(m_showData, SIGNAL(triggered()), this, SLOT(onClickShowData()));

  m_showInst = new QAction(tr("Show Instrument"), this);
  connect(m_showInst, SIGNAL(triggered()), this, SLOT(onClickShowInstrument()));

  m_plotSpec = new QAction(tr("Plot Spectrum..."), this);
  connect(m_plotSpec, SIGNAL(triggered()), this, SLOT(onClickPlotSpectra()));

  m_plotSpecErr = new QAction(tr("Plot Spectrum with Errors..."), this);
  connect(m_plotSpecErr, SIGNAL(triggered()), this, SLOT(onClickPlotSpectraErr()));

  m_plotAdvanced = new QAction(tr("Plot Advanced..."), this);
  connect(m_plotAdvanced, SIGNAL(triggered()), this, SLOT(onClickPlotAdvanced()));

  m_colorFill = new QAction(tr("Color Fill Plot"), this);
  connect(m_colorFill, SIGNAL(triggered()), this, SLOT(onClickDrawColorFillPlot()));

  m_showDetectors = new QAction(tr("Show Detectors"), this);
  connect(m_showDetectors, SIGNAL(triggered()), this, SLOT(onClickShowDetectorTable()));

  m_showBoxData = new QAction(tr("Show Box Data Table"), this);
  connect(m_showBoxData, SIGNAL(triggered()), this, SLOT(onClickShowBoxData()));

  m_showMDPlot = new QAction(tr("Plot MD"), this);
  connect(m_showMDPlot, SIGNAL(triggered()), this, SLOT(onClickShowMDPlot()));

  m_showListData = new QAction(tr("List Data"), this);
  connect(m_showListData, SIGNAL(triggered()), this, SLOT(onClickShowListData()));

  m_showSpectrumViewer = new QAction(tr("Show Spectrum Viewer"), this);
  connect(m_showSpectrumViewer, SIGNAL(triggered()), this, SLOT(onClickShowSpectrumViewer()));

  m_showSliceViewer = new QAction(tr("Show Slice Viewer"), this);
  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/SliceViewer/icons/SliceViewerWindow_icon.png"), QSize(), QIcon::Normal,
                 QIcon::Off);
    m_showSliceViewer->setIcon(icon);
  }
  connect(m_showSliceViewer, SIGNAL(triggered()), this, SLOT(onClickShowSliceViewer()));

  m_showLogs = new QAction(tr("Sample Logs..."), this);
  connect(m_showLogs, SIGNAL(triggered()), this, SLOT(onClickShowFileLog()));

  m_showSampleMaterial = new QAction(tr("Sample Material..."), this);
  connect(m_showSampleMaterial, SIGNAL(triggered()), this, SLOT(onClickShowSampleMaterial()));

  m_showHist = new QAction(tr("Show History"), this);
  connect(m_showHist, SIGNAL(triggered()), this, SLOT(onClickShowAlgHistory()));

  m_saveNexus = new QAction(tr("Save NeXus"), this);
  connect(m_saveNexus, SIGNAL(triggered()), this, SLOT(onClickSaveNexusWorkspace()));

  m_rename = new QAction(tr("Rename"), this);
  connect(m_rename, SIGNAL(triggered()), this, SLOT(renameWorkspace()));

  m_delete = new QAction(tr("Delete"), this);
  connect(m_delete, SIGNAL(triggered()), this, SLOT(onClickDeleteWorkspaces()));

  m_showTransposed = new QAction(tr("Show Transposed"), this);
  connect(m_showTransposed, SIGNAL(triggered()), this, SLOT(onClickShowTransposed()));

  m_convertToMatrixWorkspace = new QAction(tr("Convert to MatrixWorkspace"), this);
  m_convertToMatrixWorkspace->setIcon(QIcon(getQPixmap("mantid_matrix_xpm")));
  connect(m_convertToMatrixWorkspace, SIGNAL(triggered()), this, SLOT(onClickConvertToMatrixWorkspace()));

  m_convertMDHistoToMatrixWorkspace = new QAction(tr("Convert to MatrixWorkspace"), this);
  m_convertMDHistoToMatrixWorkspace->setIcon(QIcon(getQPixmap("mantid_matrix_xpm")));
  connect(m_convertMDHistoToMatrixWorkspace, SIGNAL(triggered()), this, SLOT(onClickConvertMDHistoToMatrixWorkspace()));

  m_clearUB = new QAction(tr("Clear UB Matrix"), this);
  connect(m_clearUB, SIGNAL(triggered()), this, SLOT(onClickClearUB()));
}

/**
 * Create actions for sorting.
 */
void WorkspaceTreeWidget::createSortMenuActions() {
  m_sortCriteria = SortCriteria::ByName;
  QMenu *sortMenu = new QMenu(this);

  QAction *ascendingSortAction = new QAction("Ascending", this);
  QAction *descendingSortAction = new QAction("Descending", this);
  QAction *byNameChoice = new QAction("Name", this);
  QAction *byLastModifiedChoice = new QAction("Last Modified", this);
  QAction *byMemorySize = new QAction("Size", this);

  ascendingSortAction->setCheckable(true);
  ascendingSortAction->setEnabled(true);

  descendingSortAction->setCheckable(true);
  descendingSortAction->setEnabled(true);

  QActionGroup *sortDirectionGroup = new QActionGroup(sortMenu);
  sortDirectionGroup->addAction(ascendingSortAction);
  sortDirectionGroup->addAction(descendingSortAction);
  sortDirectionGroup->setExclusive(true);
  ascendingSortAction->setChecked(true);

  byNameChoice->setCheckable(true);
  byNameChoice->setEnabled(true);

  byLastModifiedChoice->setCheckable(true);
  byLastModifiedChoice->setEnabled(true);

  byMemorySize->setCheckable(true);
  byMemorySize->setEnabled(true);

  QActionGroup *sortChoiceGroup = new QActionGroup(sortMenu);
  sortChoiceGroup->addAction(byNameChoice);
  sortChoiceGroup->addAction(byLastModifiedChoice);
  sortChoiceGroup->addAction(byMemorySize);
  sortChoiceGroup->setExclusive(true);
  byNameChoice->setChecked(true);

  connect(ascendingSortAction, SIGNAL(triggered()), this, SLOT(sortAscending()));
  connect(descendingSortAction, SIGNAL(triggered()), this, SLOT(sortDescending()));
  connect(byNameChoice, SIGNAL(triggered()), this, SLOT(chooseByName()));
  connect(byLastModifiedChoice, SIGNAL(triggered()), this, SLOT(chooseByLastModified()));
  connect(byMemorySize, SIGNAL(triggered()), this, SLOT(chooseByMemorySize()));

  sortMenu->addActions(sortDirectionGroup->actions());
  sortMenu->addSeparator();
  sortMenu->addActions(sortChoiceGroup->actions());
  m_sortButton->setMenu(sortMenu);
}

/**
 * When an item is expanded, populate the child data for this item
 * @param item :: The tree item being expanded
 */
void WorkspaceTreeWidget::populateChildData(QTreeWidgetItem *item) {
  QVariant userData = item->data(0, Qt::UserRole);
  if (userData.isNull())
    return;

  // Clear it first
  while (item->childCount() > 0) {
    auto *widgetItem = item->takeChild(0);
    delete widgetItem;
  }

  Workspace_sptr workspace = userData.value<Workspace_sptr>();

  if (auto group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace)) {
    auto members = group->getAllItems();
    for (const auto &ws : members) {
      auto *node = addTreeEntry(std::make_pair(ws->getName(), ws), item);
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
    QStringList rows = details.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    rows.append(QString("Memory used: ") + workspace->getMemorySizeAsStr().c_str());

    auto iend = rows.constEnd();
    for (auto itr = rows.constBegin(); itr != iend; ++itr) {
      MantidTreeWidgetItem *data = new MantidTreeWidgetItem(QStringList(*itr), m_tree);
      data->setFlags(Qt::NoItemFlags);
      excludeItemFromSort(data);
      item->addChild(data);
    }
  }
}

/**
 * Update the workspace tree to match the current state of the ADS.
 * It is important that the workspace tree is modified only by this method.
 * @param items Items which are currently in the ADS.
 */
void WorkspaceTreeWidget::updateTree(const TopLevelItems &items) {
  incrementUpdateCount();
  emit signalUpdateTree(items);
}

/**
 * Clears the tree and re-populates it with the given top level items
 * @param topLevelItems The map of names to workspaces
 * @param expanded Names of items who should expanded after being populated
 */
void WorkspaceTreeWidget::populateTopLevel(const TopLevelItems &topLevelItems, const QStringList &expanded) {
  {
    QMutexLocker lock(&m_mutex);
    // collect names of selected workspaces
    QList<QTreeWidgetItem *> selected = m_tree->selectedItems();
    m_selectedNames.clear(); // just in case
    foreach (QTreeWidgetItem *item, selected) { m_selectedNames << item->text(0); }

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
  }
  // apply any filtering
  filterWorkspaceTree(m_workspaceFilter->text());
}

/**
 * Adds a node for the given named item, including a single child ID item to
 * make each node have a expandable button and allowing plotting to work from
 * non-expanded items
 * @param item A name/workspace pair to add.
 * @param parent If not null then add the new items as a child of the given item
 */
MantidTreeWidgetItem *WorkspaceTreeWidget::addTreeEntry(const std::pair<std::string, Mantid::API::Workspace_sptr> &item,
                                                        QTreeWidgetItem *parent) {
  MantidTreeWidgetItem *node = new MantidTreeWidgetItem(QStringList(item.first.c_str()), m_tree);
  node->setData(0, Qt::UserRole, QVariant::fromValue(item.second));

  // A a child ID item so that it becomes expandable. Using the correct ID is
  // needed when plotting from non-expanded groups.
  const std::string wsID = item.second->id();
  auto *idNode = new MantidTreeWidgetItem(QStringList(wsID.c_str()), m_tree);
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

/**
 * Check if a workspace should be selected after dock update.
 * @param name :: Name of a workspace to check.
 */
bool WorkspaceTreeWidget::shouldBeSelected(const QString &name) const {
  QMutexLocker lock(&m_mutex);
  QStringList renamed = m_renameMap.keys(name);
  if (!renamed.isEmpty()) {
    return std::any_of(renamed.cbegin(), renamed.cend(),
                       [&](const auto &oldName) { return m_selectedNames.contains(oldName); });
  } else if (m_selectedNames.contains(name)) {
    return true;
  }
  return false;
}

void WorkspaceTreeWidget::onTreeSelectionChanged() {
  // get selected workspaces
  auto items = m_tree->selectedItems();

  if (m_groupButton) {
    if (items.size() == 1) {
      // check it's group
      auto wsSptr = items.first()->data(0, Qt::UserRole).value<Workspace_sptr>();
      auto grpSptr = std::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
      if (grpSptr) {
        m_groupButton->setText("Ungroup");
        m_groupButton->setToolTip("Ungroup selected workspace");
        m_groupButton->setEnabled(true);
      } else
        m_groupButton->setEnabled(false);

    } else if (items.size() >= 2) {
      m_groupButton->setText("Group");
      m_groupButton->setEnabled(true);
      m_groupButton->setToolTip("Group together two or more selected workspaces");
    } else if (items.size() == 0) {
      m_groupButton->setText("Group");
      m_groupButton->setEnabled(false);
      m_groupButton->setToolTip("Group together two or more selected workspaces");
    }
  }

  if (m_deleteButton)
    m_deleteButton->setEnabled(items.size() > 0);

  if (m_saveButton)
    m_saveButton->setEnabled(items.size() > 0);

  if (items.size() > 0) {
    auto item = *(items.begin());
    m_mantidDisplayModel->enableSaveNexus(item->text(0));
  } else {
    m_mantidDisplayModel->disableSaveNexus();
  }
}

/**
 * Add the actions that are appropriate for a MatrixWorkspace
 * @param menu :: The menu to store the items
 * @param matrixWS :: The workspace related to the menu
 */
void WorkspaceTreeWidget::addMatrixWorkspaceMenuItems(QMenu *menu,
                                                      const Mantid::API::MatrixWorkspace_const_sptr &matrixWS) const {
  // Add all options except plot of we only have 1 value
  menu->addAction(m_showData);
  menu->addAction(m_showInst);
  // Disable the 'show instrument' option if a workspace doesn't have an
  // instrument attached or if it does not have a spectra axis
  m_showInst->setEnabled(matrixWS->getInstrument() && !matrixWS->getInstrument()->getName().empty() &&
                         matrixWS->getAxis(1)->isSpectra());
  menu->addSeparator();
  menu->addAction(m_plotSpec);
  menu->addAction(m_plotSpecErr);
  menu->addAction(m_plotAdvanced);

  // Don't plot a spectrum if only one X value
  bool multipleBins = false;
  try {
    multipleBins = (matrixWS->blocksize() > 1);
  } catch (...) {
    const size_t numHist = matrixWS->getNumberHistograms();
    for (size_t i = 0; i < numHist; ++i) {
      if (matrixWS->y(i).size() > 1) {
        multipleBins = true;
        break;
      }
    }
  }
  m_plotSpec->setEnabled(multipleBins);
  m_plotSpecErr->setEnabled(multipleBins);
  m_plotAdvanced->setEnabled(multipleBins);

  menu->addAction(m_showSpectrumViewer); // The 2D spectrum viewer

  menu->addAction(m_colorFill);
  // Show the color fill plot if you have more than one histogram
  m_colorFill->setEnabled((matrixWS->axes() > 1 && matrixWS->getNumberHistograms() > 1));
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
void WorkspaceTreeWidget::addMDEventWorkspaceMenuItems(QMenu *menu,
                                                       const Mantid::API::IMDEventWorkspace_const_sptr &WS) const {
  Q_UNUSED(WS);

  // menu->addAction(m_showBoxData); // Show MD Box data (for debugging only)
  menu->addAction(m_showSliceViewer); // The 2D slice viewer
  menu->addAction(m_showHist);        // Algorithm history
  menu->addAction(m_showListData);    // Show data in table
  menu->addAction(m_showLogs);
}

void WorkspaceTreeWidget::addMDHistoWorkspaceMenuItems(QMenu *menu,
                                                       const Mantid::API::IMDWorkspace_const_sptr &WS) const {
  Q_UNUSED(WS);
  menu->addAction(m_showHist);        // Algorithm history
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
void WorkspaceTreeWidget::addPeaksWorkspaceMenuItems(QMenu *menu,
                                                     const Mantid::API::IPeaksWorkspace_const_sptr &WS) const {
  Q_UNUSED(WS);
  menu->addAction(m_showData);
  menu->addSeparator();
  menu->addAction(m_showDetectors);
  menu->addAction(m_showHist);
}

/**
 * Add the actions that are appropriate for a WorkspaceGroup
 * @param menu :: The menu to store the items
 */
void WorkspaceTreeWidget::addWorkspaceGroupMenuItems(QMenu *menu) const {
  m_plotSpec->setEnabled(true);
  menu->addAction(m_plotSpec);
  m_plotSpecErr->setEnabled(true);
  menu->addAction(m_plotSpecErr);
  m_plotAdvanced->setEnabled(true);
  menu->addAction(m_plotAdvanced);
  menu->addAction(m_colorFill);
  m_colorFill->setEnabled(true);

  menu->addSeparator();
  menu->addAction(m_saveNexus);
}

/**
 * Add the actions that are appropriate for a MatrixWorkspace
 * @param menu :: The menu to store the items
 */
void WorkspaceTreeWidget::addTableWorkspaceMenuItems(QMenu *menu) const {
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
void WorkspaceTreeWidget::addClearMenuItems(QMenu *menu, const QString &wsName) {
  QMenu *clearMenu = new QMenu(tr("Clear Options"), this);

  m_clearUB->setEnabled(hasUBMatrix(wsName.toStdString()));

  clearMenu->addAction(m_clearUB);
  menu->addMenu(clearMenu);
}

bool WorkspaceTreeWidget::hasUBMatrix(const std::string &wsName) {
  bool hasUB = false;
  Workspace_sptr ws = AnalysisDataService::Instance().retrieve(wsName);
  IMDWorkspace_sptr wsIMD = std::dynamic_pointer_cast<IMDWorkspace>(ws);
  if (ws && wsIMD) {
    hasUB = wsIMD->hasOrientedLattice();
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
void WorkspaceTreeWidget::addSaveMenuOption(const QString &algorithmString, QString menuEntryName) {
  // Default to algo string if no entry name given
  if (menuEntryName.isEmpty())
    menuEntryName = algorithmString;

  // Create the action and add data
  QAction *saveAction = new QAction(menuEntryName, this);
  saveAction->setData(QVariant(algorithmString));

  // Connect the trigger slot to show algorithm dialog
  connect(saveAction, SIGNAL(triggered()), this, SLOT(handleShowSaveAlgorithm()));

  // Add it to the menu
  m_saveMenu->addAction(saveAction);
}

/**
 * Filter workspaces based on the string provided
 * @param text : the string to filter on.
 */
void WorkspaceTreeWidget::filterWorkspaceTree(const QString &text) {
  m_filteredText = text.toStdString();
  m_presenter->notifyFromView(ViewNotifiable::Flag::FilterWorkspaces);
}

/// Handles delete button/menu item triggers
void WorkspaceTreeWidget::onClickDeleteWorkspaces() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::DeleteWorkspaces);
}

/**
 * Gets confirmation from user that they meant to press clear workspaces button
 * @return True if yes is pressed, false if no is pressed
 */
bool WorkspaceTreeWidget::clearWorkspacesConfirmation() const {
  return askUserYesNo("Clear Workspaces", "This will delete all the workspaces, are you sure?");
}

/**
 * Enables and disables the Clear Workspaces Button
 * @param enable : true for enable and false for disable
 */
void WorkspaceTreeWidget::enableClearButton(bool enable) { m_clearButton->setEnabled(enable); }

/// Handles clear button trigger
void WorkspaceTreeWidget::onClickClearWorkspaces() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ClearWorkspaces);
}

void WorkspaceTreeWidget::clickedWorkspace(QTreeWidgetItem *item, int /*unused*/) { Q_UNUSED(item); }

void WorkspaceTreeWidget::workspaceSelected() {
  auto selectedNames = getSelectedWorkspaceNames();
  if (selectedNames.empty())
    return;

  // If there are multiple workspaces selected group and save as Nexus
  if (selectedNames.size() > 1) {
    connect(m_saveButton, SIGNAL(clicked()), this, SLOT(saveWorkspaceCollection()));

    // Don't display as a group
    m_saveButton->setMenu(nullptr);
  } else {
    // Don't run the save group function when clicked
    disconnect(m_saveButton, SIGNAL(clicked()), this, SLOT(saveWorkspaceCollection()));

    // Remove all existing save algorithms from list
    m_saveMenu->clear();

    // Add some save algorithms
    addSaveMenuOption("SaveNexus", "Nexus");
    addSaveMenuOption("SaveAscii", "ASCII");

    // Set the button to show the menu
    m_saveButton->setMenu(m_saveMenu);
  }

  auto wsName = selectedNames[0];
  // TODO: Wire signal correctly in applicationwindow
  m_mantidDisplayModel->enableSaveNexus(QString::fromStdString(wsName));
}

/// Handles group button clicks
void WorkspaceTreeWidget::onClickGroupButton() {
  if (m_groupButton) {
    QString qButtonName = m_groupButton->text();
    if (qButtonName == "Group") {
      m_presenter->notifyFromView(ViewNotifiable::Flag::GroupWorkspaces);
    } else if (qButtonName == "Ungroup") {
      m_presenter->notifyFromView(ViewNotifiable::Flag::UngroupWorkspaces);
    }
  }
}

/// Handles Load File menu trigger
void WorkspaceTreeWidget::onClickLoad() { m_presenter->notifyFromView(ViewNotifiable::Flag::LoadWorkspace); }

/// handles Live Data menu trigger
void WorkspaceTreeWidget::onClickLiveData() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::LoadLiveDataWorkspace);
}

// Asynchronous signal handlers
/// Handle asynchronous tree update.
void WorkspaceTreeWidget::handleUpdateTree(const TopLevelItems &items) {
  m_mantidDisplayModel->updateProject();
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

  // enable clear button here if any items in tree
  enableClearButton(!items.empty());

  // Re-sort
  m_tree->sort();
}

void WorkspaceTreeWidget::handleClearView() {
  m_mantidDisplayModel->updateProject();
  m_tree->clear();
}

// Context Menu Methods

/// Handles display of the workspace context menu.
void WorkspaceTreeWidget::popupMenu(const QPoint &pos) {
  if (!m_viewOnly) {
    m_menuPosition = pos;
    m_presenter->notifyFromView(ViewNotifiable::Flag::PopulateAndShowWorkspaceContextMenu);
  }
}

void WorkspaceTreeWidget::popupContextMenu() {
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
    auto ws = mantidTreeItem->data(0, Qt::UserRole).value<Mantid::API::Workspace_sptr>();

    // Add the items that are appropriate for the type
    if (auto matrixWS = std::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(ws)) {
      addMatrixWorkspaceMenuItems(menu, matrixWS);
    } else if (auto mdeventWS = std::dynamic_pointer_cast<const IMDEventWorkspace>(ws)) {
      addMDEventWorkspaceMenuItems(menu, mdeventWS);
    } else if (auto mdWS = std::dynamic_pointer_cast<const IMDWorkspace>(ws)) {
      addMDHistoWorkspaceMenuItems(menu, mdWS);
    } else if (auto peaksWS = std::dynamic_pointer_cast<const IPeaksWorkspace>(ws)) {
      addPeaksWorkspaceMenuItems(menu, peaksWS);
    } else if (auto groupWS = std::dynamic_pointer_cast<const WorkspaceGroup>(ws)) {
      addWorkspaceGroupMenuItems(menu);
    } else if (std::dynamic_pointer_cast<const Mantid::API::ITableWorkspace>(ws)) {
      addTableWorkspaceMenuItems(menu);
    } else {
      // None of the above? -> not a workspace
      return;
    }
    addClearMenuItems(menu, selectedWsName);

    // Get the names of the programs for the send to option
    std::vector<std::string> programNames =
        (Mantid::Kernel::ConfigService::Instance().getKeys("workspace.sendto.name"));
    bool firstPass(true);
    // Check to see if any options aren't visible
    for (const auto &programName : programNames) {
      std::string visible =
          Mantid::Kernel::ConfigService::Instance().getString("workspace.sendto." + programName + ".visible");
      std::string target =
          Mantid::Kernel::ConfigService::Instance().getString("workspace.sendto." + programName + ".target");
      if (Mantid::Kernel::ConfigService::Instance().isExecutable(target) && visible == "Yes") {
        bool compatible(true);
        std::string saveUsing(
            Mantid::Kernel::ConfigService::Instance().getString("workspace.sendto." + programName + ".saveusing"));
        try {
          Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create(saveUsing);
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
      connect(m_programMapper, SIGNAL(mapped(const QString &)), this, SLOT(onClickSaveToProgram(const QString &)));

    // Rename is valid for all workspace types
    menu->addAction(m_rename);
    // separate delete
    menu->addSeparator();
    menu->addAction(m_delete);
  }

  // Show the menu at the cursor's current position
  menu->popup(QCursor::pos());
}

void WorkspaceTreeWidget::onClickShowData() { m_presenter->notifyFromView(ViewNotifiable::Flag::ShowWorkspaceData); }

void WorkspaceTreeWidget::showWorkspaceData() { m_mantidDisplayModel->importWorkspace(); }

void WorkspaceTreeWidget::onClickShowInstrument() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ShowInstrumentView);
}

void WorkspaceTreeWidget::showInstrumentView() { m_mantidDisplayModel->showMantidInstrumentSelected(); }

void WorkspaceTreeWidget::onClickSaveToProgram(const QString &name) {
  m_programName = name;
  m_presenter->notifyFromView(ViewNotifiable::Flag::SaveToProgram);
}

/**
 * Saves a workspace based on the program the user chooses to save to.
 */
void WorkspaceTreeWidget::saveToProgram() {
  // Create a map for the keys and details to go into
  std::map<std::string, std::string> programKeysAndDetails;
  programKeysAndDetails["name"] = m_programName.toStdString();

  // Get a list of the program detail keys (mandatory - target, saveusing)
  // (optional - arguments, save parameters, workspace type)
  std::vector<std::string> programKeys = (Mantid::Kernel::ConfigService::Instance().getKeys(
      ("workspace.sendto." + programKeysAndDetails.find("name")->second)));

  for (const auto &programKey : programKeys) {
    // Assign a key to its value using the map
    programKeysAndDetails[programKey] = (Mantid::Kernel::ConfigService::Instance().getString(
        ("workspace.sendto." + programKeysAndDetails.find("name")->second + "." + programKey)));
  }

  // Check to see if mandatory information is included
  if ((programKeysAndDetails.count("name") != 0) && (programKeysAndDetails.count("target") != 0) &&
      (programKeysAndDetails.count("saveusing") != 0)) {
    std::string expTarget = Poco::Path::expand(programKeysAndDetails.find("target")->second);

    QFileInfo target = QString::fromStdString(expTarget);
    if (target.exists()) {
      try {
        // Convert to QString and create Algorithm
        QString saveUsing = QString::fromStdString(programKeysAndDetails.find("saveusing")->second);

        // Create a new save based on what files the new program can open
        auto alg = m_mantidDisplayModel->createAlgorithm(saveUsing);

        // Get the file extention based on the workspace
        Property *prop = alg->getProperty("Filename");
        auto *fileProp = dynamic_cast<FileProperty *>(prop);
        std::string ext;
        if (fileProp) {
          ext = fileProp->getDefaultExt();
        }

        // Save as.. default save + the file type i.e .nxs
        alg->setPropertyValue("fileName", "auto_save_" + selectedWsName.toStdString() + ext);

        // Save the workspace
        alg->setPropertyValue("InputWorkspace", selectedWsName.toStdString());

        // If there are any save parameters
        if (programKeysAndDetails.count("saveparameters") != 0) {
          QString saveParametersGrouped = QString::fromStdString(programKeysAndDetails.find("saveparameters")->second);
          QStringList saveParameters = saveParametersGrouped.split(',');

          // For each one found split it up and assign the parameter
          for (int i = 0; i < saveParameters.size(); i++) {
            QStringList sPNameAndDetail = saveParameters[i].split('=');
            std::string saveParameterName = sPNameAndDetail[0].trimmed().toStdString();
            std::string saveParameterDetail = sPNameAndDetail[1].trimmed().toStdString();
            if (saveParameterDetail == "True")
              alg->setProperty(saveParameterName, true);
            else if (saveParameterDetail == "False")
              alg->setProperty(saveParameterName, false);
            else // if not true or false then must be a value
            {
              alg->setPropertyValue(saveParameterName, saveParameterDetail);
            }
          }
        }

        // Execute the save
        executeAlgorithmAsync(alg, true);

        // Get the save location of the file (should be default Mantid folder)
        QString savedFile = QString::fromStdString(alg->getProperty("Filename"));
        QStringList arguments;

        // Arguments for the program to take. Default will be the file anyway.
        if (programKeysAndDetails.count("arguments") != 0) {
          QString temp = QString::fromStdString(programKeysAndDetails.find("arguments")->second);
          temp.replace(QString("[file]"), savedFile);
          // temp.replace(QString("[user]"), user;
          arguments = temp.split(",");
        } else
          arguments.insert(0, savedFile);

        // convert the list into a standard vector for compatibility with Poco
        std::vector<std::string> argumentsV;

        for (int i = 0; i < arguments.size(); i++) {
          argumentsV.assign(1, (arguments[i].toStdString()));
        }

        // Execute the program
        try {
          Mantid::Kernel::ConfigService::Instance().launchProcess(expTarget, argumentsV);
        } catch (std::runtime_error &) {
          QMessageBox::information(this, "Error",
                                   "User tried to open program from: " + QString::fromStdString(expTarget) +
                                       " There was an error opening the program. "
                                       "Please check the target and arguments list "
                                       "to ensure that these are correct");
        }
      } catch (std::exception &) {
        QMessageBox::information(this, "Mantid - Send to Program",
                                 "A file property wasn't found. Please check that the correct" +
                                     QString("save algorithm was used.\n(View -> Preferences -> "
                                             "Mantid -> SendTo -> Edit -> SaveUsing)"));
      }
    } else
      QMessageBox::information(this, "Target Path Error",
                               "User tried to open program from: " + QString::fromStdString(expTarget) +
                                   " The target file path for the program "
                                   "can't be found. Please check that the full "
                                   "path is correct");
  }
}

void WorkspaceTreeWidget::onClickPlotSpectra() { m_presenter->notifyFromView(ViewNotifiable::Flag::PlotSpectrum); }

void WorkspaceTreeWidget::onClickPlotSpectraErr() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::PlotSpectrumWithErrors);
}

void WorkspaceTreeWidget::onClickPlotAdvanced() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::PlotSpectrumAdvanced);
}

/** Plots one or more spectra from each selected workspace
 * @param type "Simple", "Errors" show error bars, "Advanced" advanced plotting.
 */
void WorkspaceTreeWidget::plotSpectrum(const std::string &type) {
  const bool isAdvanced = type == "Advanced";
  const auto userInput = m_tree->chooseSpectrumFromSelected(true, true, true, isAdvanced);
  // An empty map will be returned if the user clicks cancel in the spectrum
  // selection
  if (userInput.plots.empty()) {
    return;
  }
  bool showErrorBars = ((type == "Errors") || (type == "Advanced" && userInput.errors));

  // mantidUI knows nothing about userInput, hence the long argument lists.
  if (userInput.tiled) {
    m_mantidDisplayModel->plotSubplots(userInput.plots, MantidQt::DistributionDefault, showErrorBars);
  } else if (userInput.simple || userInput.waterfall) {
    if (userInput.isAdvanced) {
      m_mantidDisplayModel->plot1D(userInput.plots, true, MantidQt::DistributionDefault, showErrorBars, nullptr, false,
                                   userInput.waterfall, userInput.advanced.logName, userInput.advanced.customLogValues);
    } else {
      m_mantidDisplayModel->plot1D(userInput.plots, true, MantidQt::DistributionDefault, showErrorBars, nullptr, false,
                                   userInput.waterfall);
    }

  } else if (userInput.surface) {
    m_mantidDisplayModel->plotSurface(userInput.advanced.accepted, userInput.advanced.plotIndex,
                                      userInput.advanced.axisName, userInput.advanced.logName,
                                      userInput.advanced.customLogValues, userInput.advanced.workspaceNames);
  } else if (userInput.contour) {
    m_mantidDisplayModel->plotContour(userInput.advanced.accepted, userInput.advanced.plotIndex,
                                      userInput.advanced.axisName, userInput.advanced.logName,
                                      userInput.advanced.customLogValues, userInput.advanced.workspaceNames);
  }
}

void WorkspaceTreeWidget::onClickDrawColorFillPlot() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ShowColourFillPlot);
}

/**
 * Draw a color fill plot of the workspaces that are currently selected.
 * NOTE: The drawing of 2D plots is currently intimately linked with
 * MantidMatrix meaning that one of these must be generated first!
 */
void WorkspaceTreeWidget::showColourFillPlot() {
  // Get the selected workspaces
  auto items = m_tree->selectedItems();
  if (items.empty())
    return;

  // Extract child workspace names from any WorkspaceGroups selected.
  // Use a list to preserve workspace order.
  QStringList allWsNames;

  for (auto &item : items) {
    auto ws = item->data(0, Qt::UserRole).value<Workspace_sptr>();

    if (auto wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(ws)) {
      for (const auto &name : wsGroup->getNames())
        allWsNames.append(QString::fromStdString(name));
    } else
      allWsNames.append(item->text(0));
  }

  // remove duplicate workspace entries
  allWsNames.removeDuplicates();

  m_mantidDisplayModel->drawColorFillPlots(allWsNames);
}

void WorkspaceTreeWidget::keyPressEvent(QKeyEvent *e) {
  switch (e->key()) {
  case Qt::Key_Delete:
  case Qt::Key_Backspace:
    m_presenter->notifyFromView(ViewNotifiable::Flag::DeleteWorkspaces);
    break;
  }
}

void WorkspaceTreeWidget::onClickShowDetectorTable() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ShowDetectorsTable);
}

void WorkspaceTreeWidget::showDetectorsTable() {
  // get selected workspace
  auto ws = QString::fromStdString(getSelectedWorkspaceNames()[0]);
  const auto *table = m_mantidDisplayModel->createDetectorTable(ws, std::vector<int>(), false);
  if (!table) {
    QMessageBox::information(this, "Error", QString("Cannot create detectors tables for workspace ") + ws);
  }
}

void WorkspaceTreeWidget::onClickShowBoxData() { m_presenter->notifyFromView(ViewNotifiable::Flag::ShowBoxDataTable); }

void WorkspaceTreeWidget::showBoxDataTable() { m_mantidDisplayModel->importBoxDataTable(); }

void WorkspaceTreeWidget::onClickShowMDPlot() { m_presenter->notifyFromView(ViewNotifiable::Flag::ShowMDPlot); }

void WorkspaceTreeWidget::showMDPlot() { m_mantidDisplayModel->showMDPlot(); }

void WorkspaceTreeWidget::onClickShowListData() { m_presenter->notifyFromView(ViewNotifiable::Flag::ShowListData); }

void WorkspaceTreeWidget::showListData() { m_mantidDisplayModel->showListData(); }

void WorkspaceTreeWidget::onClickShowSpectrumViewer() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ShowSpectrumViewer);
}

void WorkspaceTreeWidget::showSpectrumViewer() { m_mantidDisplayModel->showSpectrumViewer(); }

void WorkspaceTreeWidget::onClickShowSliceViewer() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ShowSliceViewer);
}

void WorkspaceTreeWidget::showSliceViewer() { m_mantidDisplayModel->showSliceViewer(); }

void WorkspaceTreeWidget::onClickShowFileLog() { m_presenter->notifyFromView(ViewNotifiable::Flag::ShowLogs); }

void WorkspaceTreeWidget::showLogs() { m_mantidDisplayModel->showLogFileWindow(); }

void WorkspaceTreeWidget::onClickShowSampleMaterial() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ShowSampleMaterialWindow);
}

void WorkspaceTreeWidget::showSampleMaterialWindow() { m_mantidDisplayModel->showSampleMaterialWindow(); }

void WorkspaceTreeWidget::onClickShowAlgHistory() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ShowAlgorithmHistory);
}

void WorkspaceTreeWidget::showAlgorithmHistory() { m_mantidDisplayModel->showAlgorithmHistory(); }

void WorkspaceTreeWidget::onClickShowTransposed() { m_presenter->notifyFromView(ViewNotifiable::Flag::ShowTransposed); }

void WorkspaceTreeWidget::showTransposed() { m_mantidDisplayModel->importTransposed(); }

void WorkspaceTreeWidget::onClickSaveNexusWorkspace() {
  m_saveFileType = SaveFileType::Nexus;
  m_presenter->notifyFromView(ViewNotifiable::Flag::SaveSingleWorkspace);
}
/**
 * Convert selected TableWorkspace to a MatrixWorkspace.
 */
void WorkspaceTreeWidget::onClickConvertToMatrixWorkspace() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ConvertToMatrixWorkspace);
}

/**
 * Convert selected MDHistoWorkspace to a MatrixWorkspace.
 */
void WorkspaceTreeWidget::onClickConvertMDHistoToMatrixWorkspace() {
  m_presenter->notifyFromView(ViewNotifiable::Flag::ConvertMDHistoToMatrixWorkspace);
}

void WorkspaceTreeWidget::convertToMatrixWorkspace() {
  m_mantidDisplayModel->showAlgorithmDialog("ConvertTableToMatrixWorkspace");
}

void WorkspaceTreeWidget::convertMDHistoToMatrixWorkspace() {
  m_mantidDisplayModel->showAlgorithmDialog("ConvertMDHistoToMatrixWorkspace");
}

/**
 * Handler for the clear the UB matrix event.
 */
void WorkspaceTreeWidget::onClickClearUB() { m_presenter->notifyFromView(ViewNotifiable::Flag::ClearUBMatrix); }

/**
 * Allows asynchronous execution of algorithms. This method is made
 * available in the view for access by the presenter in order to
 * obviate the dependency on Qt in the Unit tests.
 * @param alg : algorithm to be executed
 * @param wait : determines whether or not a non-gui blocking wait should occur.
 */
bool WorkspaceTreeWidget::executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg, const bool wait) {
  return m_mantidDisplayModel->executeAlgorithmAsync(alg, wait);
}

void WorkspaceTreeWidget::hideButtonToolbar() {
  m_loadButton->hide();
  m_saveButton->hide();
  m_deleteButton->hide();
  m_clearButton->hide();
  m_groupButton->hide();
  m_sortButton->hide();
}

} // namespace MantidQt::MantidWidgets
