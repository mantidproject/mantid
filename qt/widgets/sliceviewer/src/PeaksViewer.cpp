// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/PeaksViewer.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/UsageService.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"
#include "MantidQtWidgets/SliceViewer/PeaksTableColumnsDialog.h"
#include "MantidQtWidgets/SliceViewer/PeaksWorkspaceWidget.h"
#include "MantidQtWidgets/SliceViewer/ProxyCompositePeaksPresenter.h"
#include <QBoxLayout>
#include <QLayoutItem>

namespace MantidQt {
namespace SliceViewer {
/// Constructor
PeaksViewer::PeaksViewer(QWidget *parent) : QWidget(parent) {
  this->setMinimumWidth(500);
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "SliceViewer->PeaksViewer", false);
}

void PeaksViewer::setPeaksWorkspaces(const SetPeaksWorkspaces &) {}

/**
 * Remove the layout
 * @param widget
 */
void removeLayout(QWidget *widget) {
  QLayout *layout = widget->layout();
  if (layout != nullptr) {
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
      layout->removeItem(item);
      delete item->widget();
    }
    delete layout;
  }
}

/**
 * Set the peaks presenter. This allows the peaks workspace reporting controls
 * to talk to the outside world.
 * @param presenter : Proxy through which all information can be fetched.
 */
void PeaksViewer::setPresenter(
    boost::shared_ptr<ProxyCompositePeaksPresenter> presenter) {
  m_presenter = presenter;
  m_presenter->registerView(this);

  // Configure the entire control using the managed workspaces.
  auto workspaces = m_presenter->presentedWorkspaces();

  auto coordinateSystem = presenter->getTransformName();

  if (layout()) {
    removeLayout(this);
  }
  QVBoxLayout *masterLayout = new QVBoxLayout;
  this->setLayout(masterLayout);

  auto it = workspaces.begin();
  while (it != workspaces.end()) {
    Mantid::API::IPeaksWorkspace_const_sptr ws = *it;

    const auto backgroundPeakViewColor =
        m_presenter->getBackgroundPeakViewColor(ws);
    const auto foregroundPeakViewColor =
        m_presenter->getForegroundPeakViewColor(ws);

    auto widget =
        new PeaksWorkspaceWidget(ws, coordinateSystem, foregroundPeakViewColor,
                                 backgroundPeakViewColor, this);

    widget->setSelectedPeak(m_presenter->getZoomedPeakIndex());

    connect(widget,
            SIGNAL(peakColorchanged(Mantid::API::IPeaksWorkspace_const_sptr,
                                    PeakViewColor)),
            this,
            SLOT(onPeakColorChanged(Mantid::API::IPeaksWorkspace_const_sptr,
                                    PeakViewColor)));
    connect(widget,
            SIGNAL(backgroundColorChanged(
                Mantid::API::IPeaksWorkspace_const_sptr, PeakViewColor)),
            this,
            SLOT(onBackgroundColorChanged(
                Mantid::API::IPeaksWorkspace_const_sptr, PeakViewColor)));

    connect(widget,
            SIGNAL(backgroundRadiusShown(
                Mantid::API::IPeaksWorkspace_const_sptr, bool)),
            this,
            SLOT(onBackgroundRadiusShown(
                Mantid::API::IPeaksWorkspace_const_sptr, bool)));
    connect(widget,
            SIGNAL(removeWorkspace(Mantid::API::IPeaksWorkspace_const_sptr)),
            this,
            SLOT(onRemoveWorkspace(Mantid::API::IPeaksWorkspace_const_sptr)));
    connect(widget,
            SIGNAL(hideInPlot(Mantid::API::IPeaksWorkspace_const_sptr, bool)),
            this,
            SLOT(onHideInPlot(Mantid::API::IPeaksWorkspace_const_sptr, bool)));
    connect(widget,
            SIGNAL(zoomToPeak(Mantid::API::IPeaksWorkspace_const_sptr, int)),
            this,
            SLOT(onZoomToPeak(Mantid::API::IPeaksWorkspace_const_sptr, int)));
    layout()->addWidget(widget);
    ++it;
  }
}

/**
 * Hide this view.
 */
void PeaksViewer::hide() {
  QLayout *layout = this->layout();
  const int size = layout->count();
  for (int i = 0; i < size; ++i) {
    auto item = layout->itemAt(i);
    if (auto widget = item->widget()) {
      // This is important, otherwise the removed widgets sit around on the
      // layout.
      widget->hide();
    }
  }
  QWidget::hide();
}

/// Destructor
PeaksViewer::~PeaksViewer() {}

/**
 * @brief PeaksViewer::hasThingsToShow
 * @return True if there are workspaces to present.
 */
bool PeaksViewer::hasThingsToShow() const { return m_presenter->size() >= 1; }

void PeaksViewer::clearPeaksModeRequest(
    const PeaksWorkspaceWidget *const originWidget, const bool on) {
  EditMode mode = None;
  if (on) {
    QList<PeaksWorkspaceWidget *> children =
        this->findChildren<PeaksWorkspaceWidget *>();
    for (auto candidateWidget : children) {
      // For all but the most recently selected peaks workspace. Exit clear
      // mode.
      if (candidateWidget != originWidget) {
        // Exit clear mode on others.
        candidateWidget->exitClearPeaksMode();
      }
      // One mode, and One Workspace at a time. Cannot be in Add mode for any
      // Workspace while clearing peaks.
      candidateWidget->exitAddPeaksMode();
      mode = DeletePeaks;
    }
  } else {
    mode = None;
  }
  m_presenter->editCommand(mode, originWidget->getPeaksWorkspace());
}

void PeaksViewer::addPeaksModeRequest(
    const PeaksWorkspaceWidget *const originWidget, const bool on) {
  EditMode mode = None;
  if (on) {
    QList<PeaksWorkspaceWidget *> children =
        this->findChildren<PeaksWorkspaceWidget *>();
    for (auto candidateWidget : children) {
      // For all but the most recently selected peaks workspace. Exit clear
      // mode.
      if (candidateWidget != originWidget) {
        // Exit Add mode on others.
        candidateWidget->exitAddPeaksMode();
      }
      // One mode, and One Workspace at a time. Cannot be in Clear mode for any
      // Workspace while clearing peaks.
      candidateWidget->exitClearPeaksMode();
      mode = AddPeaks;
    }
  } else {
    mode = None;
  }
  m_presenter->editCommand(mode, originWidget->getPeaksWorkspace());
}

/** Load the state of the peaks viewer from a Mantid project file
 * @param lines :: the lines from the project file to read state from
 */
void PeaksViewer::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);

  if (!tsv.hasSection("peaksworkspace"))
    return;

  // load presented workspaces
  for (const auto &section : tsv.sections("peaksworkspace"))
    loadPresentedWorkspace(section);

  // Apply zooming/peak selection
  if (tsv.hasLine("ZoomedPeakWorkspaces")) {
    std::string zoomWSName;
    int index;
    tsv.selectLine("ZoomedPeakIndex");
    tsv >> index;
    tsv.selectLine("ZoomedPeakWorkspaces");
    tsv >> zoomWSName;

    auto zoomWS = getPeaksWorkspace(zoomWSName);
    if (zoomWS) {
      m_presenter->zoomToPeak(zoomWS, index);
    }
  }

  // set shown columns
  if (tsv.selectLine("ShownColumns")) {
    std::set<QString> columns;
    for (auto &name : tsv.values("ShownColumns"))
      columns.insert(QString::fromStdString(name));
    setVisibleColumns(columns);
  }

  // set scaling
  double sizeOn, sizeInto;
  tsv.selectLine("PeakSizeOnProjection");
  tsv >> sizeOn;
  tsv.selectLine("PeakSizeIntoProjection");
  tsv >> sizeInto;

  m_presenter->setPeakSizeOnProjection(sizeOn);
  m_presenter->setPeakSizeIntoProjection(sizeInto);

  performUpdate();
}

/** Load a presented workspace and settings from a Mantid project file
 * @param section :: the lines in the project file to read from
 */
void PeaksViewer::loadPresentedWorkspace(const std::string &section) {
  API::TSVSerialiser tsv(section);

  std::string name;
  bool hidden, showBackground;
  QColor backgroundCross, backgroundSphere, backgroundEllipsoid;
  QColor foregroundCross, foregroundSphere, foregroundEllipsoid;

  tsv.selectLine("Name");
  tsv >> name;
  tsv.selectLine("ShowBackground");
  tsv >> showBackground;
  tsv.selectLine("Foreground");
  tsv >> foregroundCross >> foregroundSphere >> foregroundEllipsoid;
  tsv.selectLine("Background");
  tsv >> backgroundCross >> backgroundSphere >> backgroundEllipsoid;
  tsv.selectLine("Hidden");
  tsv >> hidden;

  PeakViewColor foreground(foregroundCross, foregroundSphere,
                           foregroundEllipsoid);
  PeakViewColor background(backgroundCross, backgroundSphere,
                           backgroundEllipsoid);

  auto ws = getPeaksWorkspace(name);
  if (ws) {
    m_presenter->setForegroundColor(ws, foreground);
    m_presenter->setBackgroundColor(ws, background);
    m_presenter->hideInPlot(ws, hidden);
    m_presenter->setBackgroundRadiusShown(ws, showBackground);
  }
}

/** Save the state of the peaks viewer to a Mantid project file
 * @return the state of the peaks viewer as a project file string
 */
std::string PeaksViewer::saveToProject() const {
  API::TSVSerialiser tsv;

  // save all workspaces
  auto workspaces = m_presenter->presentedWorkspaces();
  for (const auto &ws : workspaces)
    tsv.writeSection("peaksworkspace", savePresentedWorkspace(ws));

  // save zoom a particular peak
  auto zoomPresenter = m_presenter->getZoomedPeakPresenter();
  if (zoomPresenter) {
    auto zoomedWorkspaces = (*zoomPresenter)->presentedWorkspaces();
    tsv.writeLine("ZoomedPeakIndex") << m_presenter->getZoomedPeakIndex();
    tsv.writeLine("ZoomedPeakWorkspaces");
    for (const auto &ws : zoomedWorkspaces) {
      tsv << ws->getName();
    }
  }

  // find shown columns
  std::set<QString> areShown = getVisibleColumns();
  tsv.writeLine("ShownColumns");
  for (auto &name : areShown) {
    tsv << name;
  }

  // save scaling
  tsv.writeLine("PeakSizeOnProjection");
  tsv << m_presenter->getPeakSizeOnProjection();
  tsv.writeLine("PeakSizeIntoProjection");
  tsv << m_presenter->getPeakSizeIntoProjection();

  return tsv.outputLines();
}

/** Save the state of the presented peaks workspace to a Mantid project file
 * @param ws :: the workspace to save the state of
 * @return the state of the presented peaks workspace as a project file string
 */
std::string PeaksViewer::savePresentedWorkspace(
    Mantid::API::IPeaksWorkspace_const_sptr ws) const {
  API::TSVSerialiser tsv;
  tsv.writeLine("Name") << ws->getName();
  tsv.writeLine("ShowBackground") << m_presenter->getShowBackground(ws);

  tsv.writeLine("Foreground");
  auto foreground = m_presenter->getForegroundPeakViewColor(ws);
  tsv << foreground.colorCross;
  tsv << foreground.colorSphere;
  tsv << foreground.colorEllipsoid;

  tsv.writeLine("Background");
  auto background = m_presenter->getBackgroundPeakViewColor(ws);
  tsv << background.colorCross;
  tsv << background.colorSphere;
  tsv << background.colorEllipsoid;

  tsv.writeLine("Hidden") << m_presenter->getIsHidden(ws);
  return tsv.outputLines();
}

/**
 * Handler for changing the peak  color.
 * @param peaksWS : Peaks workspace to change the foreground color on.
 * @param newColor : New color to apply.
 */
void PeaksViewer::onPeakColorChanged(
    Mantid::API::IPeaksWorkspace_const_sptr peaksWS, PeakViewColor newColor) {
  m_presenter->setForegroundColor(peaksWS, newColor);
}

/**
 * Handler for Changing the background color on a peak.
 * @param peaksWS : Peaks workspace to change the background colors on.
 * @param newColor : New color to apply to the background.
 */
void PeaksViewer::onBackgroundColorChanged(
    Mantid::API::IPeaksWorkspace_const_sptr peaksWS, PeakViewColor newColor) {
  m_presenter->setBackgroundColor(peaksWS, newColor);
}

/**
 * Event hander for showing the background radius.
 * @param peaksWS : Workspace to show the background on.
 * @param show : Flag to indicate that the background should be shown/hidden.
 */
void PeaksViewer::onBackgroundRadiusShown(
    Mantid::API::IPeaksWorkspace_const_sptr peaksWS, bool show) {
  m_presenter->setBackgroundRadiusShown(peaksWS, show);
}

/**
 * Event hander for removal of a workspace from the plot.
 * @param peaksWS : Workspace to remove
 */
void PeaksViewer::onRemoveWorkspace(
    Mantid::API::IPeaksWorkspace_const_sptr peaksWS) {
  this->removePeaksWorkspace(peaksWS);
}

/**
 * Event hander for hiding a set of peaks in the plot.
 * @param peaksWS : Peaks workspace to hide.
 * @param hide : boolean toggle for hide/unhide
 */
void PeaksViewer::onHideInPlot(Mantid::API::IPeaksWorkspace_const_sptr peaksWS,
                               bool hide) {
  m_presenter->hideInPlot(peaksWS, hide);
}

/**
 * Handler for dealing with zooming actions onto a peak.
 * @param peaksWS : Workspace to zoom in on.
 * @param peakIndex : Index of the peak to zoom in on.
 */
void PeaksViewer::onZoomToPeak(Mantid::API::IPeaksWorkspace_const_sptr peaksWS,
                               int peakIndex) {
  m_presenter->zoomToPeak(peaksWS, peakIndex);
}

/**
 * Perform an update based on the proxy composite. Re-fetch data.
 */
void PeaksViewer::performUpdate() {
  auto allWS = m_presenter->presentedWorkspaces();
  for (auto ws : allWS) {
    auto backgroundPeakViewColor = m_presenter->getBackgroundPeakViewColor(ws);
    auto foregroundPeakViewColor = m_presenter->getForegroundPeakViewColor(ws);

    bool showBackground = m_presenter->getShowBackground(ws);
    bool isHidden = m_presenter->getIsHidden(ws);
    auto optionalZoomedPresenter = m_presenter->getZoomedPeakPresenter();
    int optionalZoomedIndex = m_presenter->getZoomedPeakIndex();

    // Now find the PeaksWorkspaceWidget corresponding to this workspace name.
    QList<PeaksWorkspaceWidget *> children =
        this->findChildren<PeaksWorkspaceWidget *>();
    for (auto candidateWidget : children) {
      Mantid::API::IPeaksWorkspace_const_sptr candidateWorkspace =
          candidateWidget->getPeaksWorkspace();
      if (candidateWorkspace == ws) {
        // We have the right widget to update.
        candidateWidget->setBackgroundColor(backgroundPeakViewColor);
        candidateWidget->setForegroundColor(foregroundPeakViewColor);
        candidateWidget->setShowBackground(showBackground);
        candidateWidget->setHidden(isHidden);
        if (optionalZoomedPresenter.is_initialized()) {
          // Is the zoomed peaks workspace the current workspace.
          if (optionalZoomedPresenter.get().get() ==
              m_presenter->getPeaksPresenter(ws->getName().c_str())) {
            candidateWidget->setSelectedPeak(optionalZoomedIndex);
          }
        }
      }
      // We also update the widget in case the workspace has changed for
      // added/deleted peaks
      candidateWidget->workspaceUpdate();
    }
  }
}

void PeaksViewer::updatePeaksWorkspace(
    const std::string &toName,
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> toWorkspace) {
  /* Any widget with *toWorkspace*  peaks workspace being wrapped. Although, if
   *that's the case, we don't need to perform
   * a replacement, we only need to prompt the widget to update itself around
   *the existing reference.
   *
   * Alternately, if the name is the same, but the workspace has changed, then
   *we need to replace the workspace first.
   */

  // Now find the PeaksWorkspaceWidget corresponding to this workspace name.
  QList<PeaksWorkspaceWidget *> children =
      this->findChildren<PeaksWorkspaceWidget *>();

  for (auto candidateWidget : children) {
    const std::string candidateName = candidateWidget->getWSName();
    if (candidateName == toName) {
      // We have the right widget to update. Swap the workspace and redraw the
      // table
      candidateWidget->workspaceUpdate(toWorkspace);
      return;
    }
  }
  for (auto candidateWidget : children) {
    Mantid::API::IPeaksWorkspace_const_sptr candidateWorkspace =
        candidateWidget->getPeaksWorkspace();
    if (candidateWorkspace == toWorkspace) {
      // We have the right widget to update. Workspace is the same, just redraw
      // the table.
      candidateWidget->workspaceUpdate();
      return;
    }
  }
}

bool PeaksViewer::removePeaksWorkspace(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> toRemove) {
  bool somethingToRemove = false;
  if (m_presenter) {

    QList<PeaksWorkspaceWidget *> children =
        this->findChildren<PeaksWorkspaceWidget *>();

    for (int i = 0; i < children.size(); ++i) {
      PeaksWorkspaceWidget *candidateWidget = children.at(i);
      Mantid::API::IPeaksWorkspace_const_sptr candidateWorkspace =
          candidateWidget->getPeaksWorkspace();
      somethingToRemove = (candidateWorkspace == toRemove);
      if (somethingToRemove) {
        // We have the right widget to update. Workspace is the same, just
        // redraw
        // the table.
        candidateWidget->hide();
        children.removeAt(i);
        break;
      }
    }
    m_presenter->hideInPlot(toRemove, somethingToRemove);
    m_presenter->remove(toRemove);
  }
  return somethingToRemove;
}

bool PeaksViewer::removePeaksWorkspace(const std::string &toRemove) {
  bool somethingToRemove = false;

  if (m_presenter) {

    QList<PeaksWorkspaceWidget *> children =
        this->findChildren<PeaksWorkspaceWidget *>();

    for (int i = 0; i < children.size(); ++i) {
      PeaksWorkspaceWidget *candidateWidget = children.at(i);
      const std::string candidateWorkspaceName = candidateWidget->getWSName();
      somethingToRemove = (candidateWorkspaceName == toRemove);
      if (somethingToRemove) {
        // We have the right widget to update. Workspace is the same, just
        // redraw
        // the table.
        candidateWidget->hide();
        children.removeAt(i);
        m_presenter->remove(candidateWidget->getPeaksWorkspace());
        break;
      }
    }
  }
  return somethingToRemove;
}

/**
 * Slot called when the user wants to see the dialog for selecting
 * what columns are visible in the tables of peaks.
 */
void PeaksViewer::showPeaksTableColumnOptions() {
  auto areShown = getVisibleColumns();
  auto toShow = chooseVisibleColumnsFromDialog(areShown);
  setVisibleColumns(toShow);
}

/**
 * Get columns which are currently visible on the widget
 * @return a set of the visible column names
 */
std::set<QString> PeaksViewer::getVisibleColumns() const {
  std::set<QString> areShown;

  QLayout *layout = this->layout();
  const int size = layout->count();
  for (int i = 0; i < size; ++i) {
    auto item = layout->itemAt(i);
    if (auto widget = item->widget()) {
      if (auto table = dynamic_cast<PeaksWorkspaceWidget *>(widget)) {
        auto shown = table->getShownColumns();
        areShown.insert(shown.begin(), shown.end());
      }
    }
  }

  return areShown;
}

/**
 * Set the columns which are currently visible on the widget
 * @param columns :: the names of the columns which should be visible
 */
void PeaksViewer::setVisibleColumns(const std::set<QString> &columns) {
  QLayout *layout = this->layout();
  const int size = layout->count();
  for (int i = 0; i < size; ++i) {
    auto item = layout->itemAt(i);
    if (auto widget = item->widget()) {
      if (auto table = dynamic_cast<PeaksWorkspaceWidget *>(widget)) {
        table->setShownColumns(columns);
      }
    }
  }
}

/**
 * Show a PeaksTableColumnsDialog to allow the user to select the columns shown.
 *
 * This shows the currently visible columns and returns a new set of columns
 * which shown now be made visible.
 *
 * @param columns :: the names of the columns which are currently visible
 * @return a new set of column names which should now be visible
 */
std::set<QString>
PeaksViewer::chooseVisibleColumnsFromDialog(const std::set<QString> &columns) {
  PeaksTableColumnsDialog dialog(this);
  dialog.setVisibleColumns(columns);
  dialog.exec();
  auto result = static_cast<QDialog::DialogCode>(dialog.result());

  if (result != QDialog::DialogCode::Accepted)
    return columns;

  return dialog.getVisibleColumns();
}

/**
 * Get a peaks workspace from the ADS
 * @param name :: the name of the workspace in the ADS
 * @return a pointer to the peaks workspaces
 */
Mantid::API::IPeaksWorkspace_const_sptr
PeaksViewer::getPeaksWorkspace(const std::string &name) const {
  using namespace Mantid::API;
  auto &ads = AnalysisDataService::Instance();

  if (ads.doesExist(name)) {
    return ads.retrieveWS<IPeaksWorkspace>(name);
  }

  return nullptr;
}

} // namespace SliceViewer
} // namespace MantidQt
