// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MantidTreeWidget.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidget.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/DropEventHelper.h"
#include "MantidQtWidgets/Common/MantidDisplayBase.h"

#include <QApplication>
#include <QDrag>
#include <QDragMoveEvent>
#include <QFileInfo>
#include <QList>
#include <QMimeData>
#include <QUrl>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger treelog("MantidTreeWidget");
}

namespace MantidQt::MantidWidgets {

MantidTreeWidget::MantidTreeWidget(MantidDisplayBase *mui, QWidget *parent)
    : QTreeWidget(parent), m_mantidUI(mui), m_ads(Mantid::API::AnalysisDataService::Instance()), m_sortScheme() {
  setObjectName("WorkspaceTree");
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setSortOrder(Qt::AscendingOrder);
  setAcceptDrops(true);

  m_doubleClickAction = [&](const QString &wsName) { m_mantidUI->importWorkspace(wsName, false); };
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
  const auto filenames = DropEventHelper::getFileNames(de);
  de->acceptProposedAction();

  for (int i = 0; i < filenames.size(); ++i) {
    try {
      QFileInfo fi(filenames[i]);
      QString basename = fi.completeBaseName();
      auto alg = m_mantidUI->createAlgorithm("Load");
      alg->initialize();
      alg->setProperty("Filename", filenames[i].toStdString());
      alg->setProperty("OutputWorkspace", basename.toStdString());
      m_mantidUI->executeAlgorithmAsync(alg, true);
    } catch (std::runtime_error &error) {
      treelog.error() << "Failed to Load the file " << filenames[i].toStdString()
                      << " . The reason for failure is: " << error.what() << '\n';
    } catch (std::logic_error &error) {
      treelog.error() << "Failed to Load the file " << filenames[i].toStdString()
                      << " . The reason for failure is: " << error.what() << '\n';
    } catch (std::exception &error) {
      treelog.error() << "Failed to Load the file " << filenames[i].toStdString()
                      << " . The reason for failure is: " << error.what() << '\n';
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
  if ((e->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
    return;

  QStringList wsNames = getSelectedWorkspaceNames();
  if (wsNames.isEmpty())
    return;

  // Start dragging - Qt docs say not to delete the QDrag object
  // manually
  QDrag *drag = new QDrag(this);
  auto *mimeData = new QMimeData;
  drag->setMimeData(mimeData);
  mimeData->setObjectName("MantidWorkspace");
  mimeData->setText(wsNames.join("\n"));
  drag->exec(Qt::CopyAction | Qt::MoveAction);
}

void MantidTreeWidget::mouseDoubleClickEvent(QMouseEvent *e) {
  try {
    auto wsNames = getSelectedWorkspaceNames();
    if (wsNames.isEmpty()) {
      return;
    }
    auto wsName = wsNames.front();
    Mantid::API::WorkspaceGroup_sptr grpWSPstr;
    grpWSPstr = std::dynamic_pointer_cast<WorkspaceGroup>(m_ads.retrieve(wsName.toStdString()));
    if (!grpWSPstr || grpWSPstr->isGroupPeaksWorkspaces()) {
      if (!wsName.isEmpty()) {
        m_doubleClickAction(wsName);
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
QList<MatrixWorkspace_const_sptr> MantidTreeWidget::getSelectedMatrixWorkspaces() const {
  // Check for any selected WorkspaceGroup names and replace with the names of
  // their children.
  // We preserve the order, but use a set to avoid adding duplicate workspaces.
  std::set<QString> selectedWsNameSet;
  std::vector<QString> selectedWsNameList;
  foreach (const QString wsName, this->getSelectedWorkspaceNames()) {
    const auto groupWs = std::dynamic_pointer_cast<const WorkspaceGroup>(m_ads.retrieve(wsName.toStdString()));
    if (groupWs) {
      const auto childWsNames = groupWs->getNames();
      for (const auto &childWsName : childWsNames) {
        if (selectedWsNameSet.find(QString::fromStdString(childWsName)) == selectedWsNameSet.end()) {
          selectedWsNameSet.insert(QString::fromStdString(childWsName));
          selectedWsNameList.emplace_back(QString::fromStdString(childWsName));
        }
      }
    } else {
      if (selectedWsNameSet.insert(wsName).second) {
        selectedWsNameList.emplace_back(wsName);
      }
    }
  }

  // Get the names of, and pointers to, the MatrixWorkspaces only.
  QList<MatrixWorkspace_const_sptr> selectedMatrixWsList;
  QList<QString> selectedMatrixWsNameList;
  foreach (const auto selectedWsName, selectedWsNameList) {
    const auto matrixWs =
        std::dynamic_pointer_cast<const MatrixWorkspace>(m_ads.retrieve(selectedWsName.toStdString()));
    if (matrixWs) {
      selectedMatrixWsList.append(matrixWs);
    }
  }
  return selectedMatrixWsList;
}

/**
 * Allows users to choose spectra from the selected workspaces by presenting
 * them with a dialog box.  Skips showing the dialog box and automatically
 * chooses workspace index 0 for all selected workspaces if one or more of the
 * them are single-spectrum workspaces.
 *
 * @param showWaterfallOpt If true, show the waterfall option on the dialog
 * @param showPlotAll :: [input] If true, show the "Plot All" button on the
 * dialog
 * @param showTiledOpt :: [input] If true, show the "Tiled" option on the dialog
 * @param isAdvanced :: [input] If true, advanced plotting being done
 * @return :: A MantidWSIndexDialog::UserInput structure listing the selected
 * options
 */
MantidWSIndexWidget::UserInput MantidTreeWidget::chooseSpectrumFromSelected(bool showWaterfallOpt, bool showPlotAll,
                                                                            bool showTiledOpt, bool isAdvanced) const {
  auto selectedMatrixWsList = getSelectedMatrixWorkspaces();
  QList<QString> selectedMatrixWsNameList;
  foreach (const auto matrixWs, selectedMatrixWsList) {
    selectedMatrixWsNameList.append(QString::fromStdString(matrixWs->getName()));
  }

  // Check workspaces to see whether to plot immediately without dialog box
  bool plotImmediately = true;
  if (isAdvanced) {
    plotImmediately = selectedMatrixWsList.size() == 1 && selectedMatrixWsList[0]->getNumberHistograms() == 1;
  } else {
    plotImmediately =
        std::none_of(selectedMatrixWsList.cbegin(), selectedMatrixWsList.cend(),
                     [](const auto &selectedMatrixWs) { return selectedMatrixWs->getNumberHistograms() != 1; });
  }

  // ... and if so, just return all workspace names mapped to workspace index 0;
  if (plotImmediately) {
    const std::set<int> SINGLE_SPECTRUM = {0};
    QMultiMap<QString, std::set<int>> spectrumToPlot;
    foreach (const auto selectedMatrixWs, selectedMatrixWsList) {
      spectrumToPlot.insert(QString::fromStdString(selectedMatrixWs->getName()), SINGLE_SPECTRUM);
    }
    // and get simple 1D plot done
    MantidWSIndexWidget::UserInput selections;
    selections.plots = spectrumToPlot;
    selections.simple = true;
    selections.waterfall = false;
    selections.tiled = false;
    selections.surface = false;
    selections.contour = false;
    return selections;
  }

  // Else, one or more workspaces
  auto dio = m_mantidUI->createWorkspaceIndexDialog(0, selectedMatrixWsNameList, showWaterfallOpt, showPlotAll,
                                                    showTiledOpt, isAdvanced);
  dio->exec();
  return dio->getSelections();
}

void MantidTreeWidget::setSortScheme(MantidItemSortScheme sortScheme) { m_sortScheme = sortScheme; }

void MantidTreeWidget::setSortOrder(Qt::SortOrder sortOrder) { m_sortOrder = sortOrder; }

Qt::SortOrder MantidTreeWidget::getSortOrder() const { return m_sortOrder; }

MantidItemSortScheme MantidTreeWidget::getSortScheme() const { return m_sortScheme; }

/**
 * Sort the items according to the current sort scheme and order.
 */
void MantidTreeWidget::sort() { sortItems(sortColumn(), m_sortOrder); }

/**
 * Log a warning message.
 * @param msg :: A message to log.
 */
void MantidTreeWidget::logWarningMessage(const std::string &msg) { treelog.warning(msg); }
} // namespace MantidQt::MantidWidgets
