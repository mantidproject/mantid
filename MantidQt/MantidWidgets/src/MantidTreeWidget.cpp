#include "MantidQtMantidWidgets/MantidTreeWidget.h"
#include <MantidQtMantidWidgets/WorkspacePresenter/QWorkspaceDockView.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtMantidWidgets/MantidDisplayBase.h"

#include <QApplication>
#include <QDragMoveEvent>
#include <QFileInfo>
#include <QList>
#include <QUrl>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger treelog("MantidTreeWidget");
}

namespace MantidQt {
namespace MantidWidgets {

MantidTreeWidget::MantidTreeWidget(QWorkspaceDockView *w,
                                   MantidDisplayBase *mui)
    : QTreeWidget(w), m_dockWidget(w), m_mantidUI(mui),
      m_ads(Mantid::API::AnalysisDataService::Instance()), m_sortScheme() {
  setObjectName("WorkspaceTree");
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setSortOrder(Qt::AscendingOrder);
  setAcceptDrops(true);
}

/**
* Accept a drag move event and selects whether to accept the action
* @param de :: The drag move event
*/
void MantidTreeWidget::dragMoveEvent(QDragMoveEvent *de) {
  // The event needs to be accepted here
  if (de->mimeData()->hasUrls())
    de->accept();
}

/**
* Accept a drag enter event and selects whether to accept the action
* @param de :: The drag enter event
*/
void MantidTreeWidget::dragEnterEvent(QDragEnterEvent *de) {
  // Set the drop action to be the proposed action.
  if (de->mimeData()->hasUrls())
    de->acceptProposedAction();
}

/**
* Accept a drag drop event and process the data appropriately
* @param de :: The drag drop event
*/
void MantidTreeWidget::dropEvent(QDropEvent *de) {
  QStringList filenames;
  const QMimeData *mimeData = de->mimeData();
  if (mimeData->hasUrls()) {
    QList<QUrl> urlList = mimeData->urls();
    for (int i = 0; i < urlList.size(); ++i) {
      QString fName = urlList[i].toLocalFile();
      if (fName.size() > 0) {
        filenames.append(fName);
      }
    }
  }
  de->acceptProposedAction();

  for (int i = 0; i < filenames.size(); ++i) {
    try {
      QFileInfo fi(filenames[i]);
      QString basename = fi.baseName();
      auto alg = m_mantidUI->createAlgorithm("Load");
      alg->initialize();
      alg->setProperty("Filename", filenames[i].toStdString());
      alg->setProperty("OutputWorkspace", basename.toStdString());
      m_mantidUI->executeAlgorithmAsync(alg, true);
    } catch (std::runtime_error &error) {
      treelog.error() << "Failed to Load the file "
                      << filenames[i].toStdString()
                      << " . The reason for failure is: " << error.what()
                      << '\n';
    } catch (std::logic_error &error) {
      treelog.error() << "Failed to Load the file "
                      << filenames[i].toStdString()
                      << " . The reason for failure is: " << error.what()
                      << '\n';
    } catch (std::exception &error) {
      treelog.error() << "Failed to Load the file "
                      << filenames[i].toStdString()
                      << " . The reason for failure is: " << error.what()
                      << '\n';
    }
  }
}

void MantidTreeWidget::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    if (!itemAt(e->pos()))
      selectionModel()->clear();
    m_dragStartPosition = e->pos();
  }

  QTreeWidget::mousePressEvent(e);
}

void MantidTreeWidget::mouseMoveEvent(QMouseEvent *e) {
  if (!(e->buttons() & Qt::LeftButton))
    return;
  if ((e->pos() - m_dragStartPosition).manhattanLength() <
      QApplication::startDragDistance())
    return;

  // Start dragging
  QDrag *drag = new QDrag(this);
  QMimeData *mimeData = new QMimeData;

  QStringList wsnames = getSelectedWorkspaceNames();
  if (wsnames.size() == 0)
    return;
  QString importStatement = "";
  foreach (const QString wsname, wsnames) {
    QString prefix = "";
    if (wsname[0].isDigit())
      prefix = "ws";
    if (importStatement.size() > 0)
      importStatement += "\n";
    importStatement += prefix + wsname + " = mtd[\"" + wsname + "\"]";
  }

  mimeData->setText(importStatement);
  mimeData->setObjectName("MantidWorkspace");

  drag->setMimeData(mimeData);

  Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
  (void)dropAction;
}

void MantidTreeWidget::mouseDoubleClickEvent(QMouseEvent *e) {
  try {
    auto wsNames = getSelectedWorkspaceNames();
    if (wsNames.isEmpty()) {
      return;
    }
    auto wsName = wsNames.front();
    Mantid::API::WorkspaceGroup_sptr grpWSPstr;
    grpWSPstr = boost::dynamic_pointer_cast<WorkspaceGroup>(
        m_ads.retrieve(wsName.toStdString()));
    if (!grpWSPstr) {
      if (!wsName.isEmpty()) {
        m_mantidUI->importWorkspace(wsName, false);
        return;
      }
    }
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    return;
  }
  QTreeWidget::mouseDoubleClickEvent(e);
}

/**
* Returns a list of all selected workspaces.  It does NOT
* extract child workspaces from groups - it only returns
* exactly what has been selected.
*/
QStringList MantidTreeWidget::getSelectedWorkspaceNames() const {
  QStringList names;

  foreach (const auto selectedItem, this->selectedItems()) {
    if (selectedItem)
      names.append(selectedItem->text(0));
  }

  return names;
}

/**
* Filter the list of selected workspace names to account for any
* non-MatrixWorkspaces that may have been selected.  In particular
* WorkspaceGroups (the children of which are to be included if they are
* MatrixWorkspaces) and TableWorkspaces (which are implicitly excluded).
* We only want workspaces we can actually plot!
*/
QList<MatrixWorkspace_const_sptr>
MantidTreeWidget::getSelectedMatrixWorkspaces() const {
  // Check for any selected WorkspaceGroup names and replace with the names of
  // their children.
  QSet<QString> selectedWsNames;
  foreach (const QString wsName, this->getSelectedWorkspaceNames()) {
    const auto groupWs = boost::dynamic_pointer_cast<const WorkspaceGroup>(
        m_ads.retrieve(wsName.toStdString()));
    if (groupWs) {
      const auto childWsNames = groupWs->getNames();
      for (auto childWsName : childWsNames) {
        selectedWsNames.insert(QString::fromStdString(childWsName));
      }
    } else {
      selectedWsNames.insert(wsName);
    }
  }

  // Get the names of, and pointers to, the MatrixWorkspaces only.
  QList<MatrixWorkspace_const_sptr> selectedMatrixWsList;
  QList<QString> selectedMatrixWsNameList;
  foreach (const auto selectedWsName, selectedWsNames) {
    const auto matrixWs = boost::dynamic_pointer_cast<const MatrixWorkspace>(
        m_ads.retrieve(selectedWsName.toStdString()));
    if (matrixWs) {
      selectedMatrixWsList.append(matrixWs);
    }
  }
  return selectedMatrixWsList;
}

/**
* Allows users to choose spectra from the selected workspaces by presenting them
* with a dialog box.  Skips showing the dialog box and automatically chooses
* workspace index 0 for all selected workspaces if one or more of the them are
* single-spectrum workspaces.
*
* @param showWaterfallOpt If true, show the waterfall option on the dialog
* @param showPlotAll :: [input] If true, show the "Plot All" button on the
* dialog
* @return :: A MantidWSIndexDialog::UserInput structure listing the selected
* options
*/
MantidWSIndexWidget::UserInput
MantidTreeWidget::chooseSpectrumFromSelected(bool showWaterfallOpt,
                                             bool showPlotAll) const {
  auto selectedMatrixWsList = getSelectedMatrixWorkspaces();
  QList<QString> selectedMatrixWsNameList;
  foreach (const auto matrixWs, selectedMatrixWsList) {
    selectedMatrixWsNameList.append(QString::fromStdString(matrixWs->name()));
  }

  // Check to see if all workspaces have only a single spectrum ...
  bool allSingleWorkspaces = true;
  foreach (const auto selectedMatrixWs, selectedMatrixWsList) {
    if (selectedMatrixWs->getNumberHistograms() != 1) {
      allSingleWorkspaces = false;
      break;
    }
  }

  // ... and if so, just return all workspace names mapped to workspace index 0;
  if (allSingleWorkspaces) {
    const std::set<int> SINGLE_SPECTRUM = {0};
    QMultiMap<QString, std::set<int>> spectrumToPlot;
    foreach (const auto selectedMatrixWs, selectedMatrixWsList) {
      spectrumToPlot.insert(QString::fromStdString(selectedMatrixWs->name()),
                            SINGLE_SPECTRUM);
    }
    MantidWSIndexWidget::UserInput selections;
    selections.plots = spectrumToPlot;
    selections.waterfall = false;
    return selections;
  }

  // Else, one or more workspaces
  auto dio = m_mantidUI->createWorkspaceIndexDialog(
      0, selectedMatrixWsNameList, showWaterfallOpt, showPlotAll);
  dio->exec();
  return dio->getSelections();
}

/**
* Allows users to choose spectra from the selected workspaces by presenting them
* with a dialog box, and also allows choice of a log to plot against and a name
* for this axis.
* @param type :: [input] Type of plot (for dialog title)
* @param nWorkspaces :: [input] Number of workspaces in selected group
* @returns :: A structure listing the selected options
*/
MantidSurfacePlotDialog::UserInputSurface
MantidTreeWidget::choosePlotOptions(const QString &type,
                                    int nWorkspaces) const {
  auto selectedMatrixWsList = getSelectedMatrixWorkspaces();
  QList<QString> selectedMatrixWsNameList;
  foreach (const auto matrixWs, selectedMatrixWsList) {
    selectedMatrixWsNameList.append(QString::fromStdString(matrixWs->name()));
  }
  auto *dlg =
      m_mantidUI->createSurfacePlotDialog(0, selectedMatrixWsNameList, type);
  dlg->exec();
  auto selections = dlg->getSelections();
  std::stringstream err;

  if (selections.accepted) {
    if (selections.logName == MantidSurfacePlotDialog::CUSTOM) {
      // Check number of values supplied
      if (static_cast<int>(selections.customLogValues.size()) != nWorkspaces) {
        err << "Number of custom log values must be equal to "
               "number of workspaces in group";
        selections.accepted = false;
      }
    }
  }

  auto errors = err.str();
  if (!errors.empty()) {
    MantidSurfacePlotDialog::showPlotOptionsError(errors.c_str());
  }
  return selections;
}

/**
* Allows users to choose spectra from the selected workspaces by presenting them
* with a dialog box, and also allows choice of a log to plot against and a name
* for this axis.
* @param nWorkspaces :: [input] Number of workspaces in selected group
* @returns :: A structure listing the selected options
*/
MantidSurfacePlotDialog::UserInputSurface
MantidTreeWidget::chooseSurfacePlotOptions(int nWorkspaces) const {
  return choosePlotOptions("Surface", nWorkspaces);
}

/**
* Allows users to choose spectra from the selected workspaces by presenting them
* with a dialog box, and also allows choice of a log to plot against and a name
* for this axis.
* @param nWorkspaces :: [input] Number of workspaces in selected group
* @returns :: A structure listing the selected options
*/
MantidSurfacePlotDialog::UserInputSurface
MantidTreeWidget::chooseContourPlotOptions(int nWorkspaces) const {
  return choosePlotOptions("Contour", nWorkspaces);
}

void MantidTreeWidget::setSortScheme(MantidItemSortScheme sortScheme) {
  m_sortScheme = sortScheme;
}

void MantidTreeWidget::setSortOrder(Qt::SortOrder sortOrder) {
  m_sortOrder = sortOrder;
}

Qt::SortOrder MantidTreeWidget::getSortOrder() const { return m_sortOrder; }

MantidItemSortScheme MantidTreeWidget::getSortScheme() const {
  return m_sortScheme;
}

/**
* Sort the items according to the current sort scheme and order.
*/
void MantidTreeWidget::sort() { sortItems(sortColumn(), m_sortOrder); }

/**
* Log a warning message.
* @param msg :: A message to log.
*/
void MantidTreeWidget::logWarningMessage(const std::string &msg) {
  treelog.warning(msg);
}
}
}
