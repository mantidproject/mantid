// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/Common/TSVSerialiser.h"
#endif
#include "MantidQtWidgets/InstrumentView/DetXMLFile.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Unit.h"
#include "MantidQtWidgets/InstrumentView/PanelsSurface.h"
#include "MantidQtWidgets/InstrumentView/Projection3D.h"
#include "MantidQtWidgets/InstrumentView/SimpleWidget.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedCylinder.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedSphere.h"
#include "MantidQtWidgets/InstrumentView/XIntegrationControl.h"

#include <Poco/ActiveResult.h>

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleValidator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImageWriter>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSplitter>
#include <QStackedLayout>
#include <QString>
#include <QTemporaryFile>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <numeric>
#include <stdexcept>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt::API;

namespace MantidQt {
namespace MantidWidgets {
// Name of the QSettings group to store the InstrumentWindw settings
const char *InstrumentWidgetSettingsGroup = "Mantid/InstrumentWidget";

/**
 * Exception type thrown when an istrument has no sample and cannot be displayed
 * in the instrument view.
 */
class InstrumentHasNoSampleError : public std::runtime_error {
public:
  InstrumentHasNoSampleError()
      : std::runtime_error("Instrument has no sample.\nSource and sample need "
                           "to be set in the IDF.") {}
};

/**
 * Constructor.
 */
InstrumentWidget::InstrumentWidget(const QString &wsName, QWidget *parent,
                                   bool resetGeometry, bool autoscaling,
                                   double scaleMin, double scaleMax,
                                   bool setDefaultView)
    : QWidget(parent), WorkspaceObserver(), m_InstrumentDisplay(nullptr),
      m_simpleDisplay(nullptr), m_workspaceName(wsName),
      m_instrumentActor(nullptr), m_surfaceType(FULL3D),
      m_savedialog_dir(QString::fromStdString(
          Mantid::Kernel::ConfigService::Instance().getString(
              "defaultsave.directory"))),
      mViewChanged(false), m_blocked(false),
      m_instrumentDisplayContextMenuOn(false) {
  setFocusPolicy(Qt::StrongFocus);
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  QSplitter *controlPanelLayout = new QSplitter(Qt::Horizontal);

  // Add Tab control panel
  mControlsTab = new QTabWidget(this);
  controlPanelLayout->addWidget(mControlsTab);
  controlPanelLayout->setSizePolicy(QSizePolicy::Expanding,
                                    QSizePolicy::Expanding);

  // Create the display widget
  m_InstrumentDisplay = new MantidGLWidget(this);
  m_InstrumentDisplay->installEventFilter(this);
  connect(this, SIGNAL(enableLighting(bool)), m_InstrumentDisplay,
          SLOT(enableLighting(bool)));

  // Create simple display widget
  m_simpleDisplay = new SimpleWidget(this);
  m_simpleDisplay->installEventFilter(this);

  QWidget *aWidget = new QWidget(this);
  m_instrumentDisplayLayout = new QStackedLayout(aWidget);
  m_instrumentDisplayLayout->addWidget(m_InstrumentDisplay);
  m_instrumentDisplayLayout->addWidget(m_simpleDisplay);

  controlPanelLayout->addWidget(aWidget);

  mainLayout->addWidget(controlPanelLayout);

  m_xIntegration = new XIntegrationControl(this);
  mainLayout->addWidget(m_xIntegration);
  connect(m_xIntegration, SIGNAL(changed(double, double)), this,
          SLOT(setIntegrationRange(double, double)));

  // Set the mouse/keyboard operation info and help button
  QHBoxLayout *infoLayout = new QHBoxLayout();
  mInteractionInfo = new QLabel();
  infoLayout->addWidget(mInteractionInfo);
  QPushButton *helpButton = new QPushButton("?");
  helpButton->setMaximumWidth(25);
  connect(helpButton, SIGNAL(clicked()), this, SLOT(helpClicked()));
  infoLayout->addWidget(helpButton);
  infoLayout->setStretchFactor(mInteractionInfo, 1);
  infoLayout->setStretchFactor(helpButton, 0);
  mainLayout->addLayout(infoLayout);

  QSettings settings;
  settings.beginGroup(InstrumentWidgetSettingsGroup);

  // Background colour
  setBackgroundColor(
      settings.value("BackgroundColor", QColor(0, 0, 0, 1.0)).value<QColor>());

  m_instrumentActor.reset(
      new InstrumentActor(m_workspaceName, autoscaling, scaleMin, scaleMax));

  // Create the b=tabs
  createTabs(settings);

  settings.endGroup();

  // Init actions
  m_clearPeakOverlays = new QAction("Clear peaks", this);
  connect(m_clearPeakOverlays, SIGNAL(triggered()), this,
          SLOT(clearPeakOverlays()));

  // Clear alignment plane action
  m_clearAlignment = new QAction("Clear alignment plane", this);
  connect(m_clearAlignment, SIGNAL(triggered()), this,
          SLOT(clearAlignmentPlane()));

  // confirmClose(app->confirmCloseInstrWindow);

  setAttribute(Qt::WA_DeleteOnClose);

  // Watch for the deletion of the associated workspace
  observePreDelete();
  observeAfterReplace();
  observeRename();
  observeADSClear();

  const int windowWidth = 800;
  const int tabsSize = windowWidth / 4;
  QList<int> sizes;
  sizes << tabsSize << windowWidth - tabsSize;
  controlPanelLayout->setSizes(sizes);
  controlPanelLayout->setStretchFactor(0, 0);
  controlPanelLayout->setStretchFactor(1, 1);

  resize(windowWidth, 650);

  tabChanged(0);

  connect(this, SIGNAL(needSetIntegrationRange(double, double)), this,
          SLOT(setIntegrationRange(double, double)), Qt::QueuedConnection);
  setAcceptDrops(true);

  setWindowTitle(QString("Instrument - ") + m_workspaceName);

  const bool resetActor(false);
  init(resetGeometry, autoscaling, scaleMin, scaleMax, setDefaultView,
       resetActor);
}

/**
 * Destructor
 */
InstrumentWidget::~InstrumentWidget() {
  if (m_instrumentActor) {
    saveSettings();
  }
}

QString InstrumentWidget::getWorkspaceName() const { return m_workspaceName; }

std::string InstrumentWidget::getWorkspaceNameStdString() const {
  return m_workspaceName.toStdString();
}

void InstrumentWidget::renameWorkspace(const std::string &workspace) {
  m_workspaceName = QString::fromStdString(workspace);
}

/**
 * Get the axis vector for the surface projection type.
 * @param surfaceType :: Surface type for this projection
 * @return a V3D for the axis being projected on
 */
Mantid::Kernel::V3D
InstrumentWidget::getSurfaceAxis(const int surfaceType) const {
  Mantid::Kernel::V3D axis;

  // define the axis
  if (surfaceType == SPHERICAL_Y || surfaceType == CYLINDRICAL_Y) {
    axis = Mantid::Kernel::V3D(0, 1, 0);
  } else if (surfaceType == SPHERICAL_Z || surfaceType == CYLINDRICAL_Z) {
    axis = Mantid::Kernel::V3D(0, 0, 1);
  } else if (surfaceType == SPHERICAL_X || surfaceType == CYLINDRICAL_X) {
    axis = Mantid::Kernel::V3D(1, 0, 0);
  } else // SIDE_BY_SIDE
  {
    axis = Mantid::Kernel::V3D(0, 0, 1);
  }

  return axis;
}

/**
 * Init the geometry and colour map outside constructor to prevent creating a
 * broken MdiSubwindow.
 * Must be called straight after constructor.
 * @param resetGeometry :: Set true for resetting the view's geometry: the
 * bounding box and rotation. Default is true.
 * @param autoscaling :: True to start with autoscaling option on.
 * @param scaleMin :: Minimum value of the colormap scale. Ignored if
 * autoscaling == true.
 * @param scaleMax :: Maximum value of the colormap scale. Ignored if
 * autoscaling == true.
 * @param setDefaultView :: Set the default surface type
 * @param resetActor :: If true reset the instrumentActor object
 */
void InstrumentWidget::init(bool resetGeometry, bool autoscaling,
                            double scaleMin, double scaleMax,
                            bool setDefaultView, bool resetActor) {
  if (resetActor) {
    m_instrumentActor.reset(
        new InstrumentActor(m_workspaceName, autoscaling, scaleMin, scaleMax));
  }
  m_xIntegration->setTotalRange(m_instrumentActor->minBinValue(),
                                m_instrumentActor->maxBinValue());
  m_xIntegration->setUnits(QString::fromStdString(
      m_instrumentActor->getWorkspace()->getAxis(0)->unit()->caption()));
  auto surface = getSurface();
  if (resetGeometry || !surface) {
    if (setDefaultView) {
      // set the view type to the instrument's default view
      QString defaultView =
          QString::fromStdString(m_instrumentActor->getDefaultView());
      if (defaultView == "3D" &&
          !Mantid::Kernel::ConfigService::Instance()
               .getValue<bool>("MantidOptions.InstrumentView.UseOpenGL")
               .get_value_or(true)) {
        // if OpenGL is switched off don't open the 3D view at start up
        defaultView = "CYLINDRICAL_Y";
      }
      setSurfaceType(defaultView);
    } else {
      setSurfaceType(m_surfaceType); // This call must come after the
                                     // InstrumentActor is created
    }
    setupColorMap();
  } else {
    surface->resetInstrumentActor(m_instrumentActor.get());
    updateInfoText();
  }
}

/**
 * Deletes instrument actor before re-initializing.
 * @param resetGeometry
 */
void InstrumentWidget::resetInstrument(bool resetGeometry) {
  init(resetGeometry, true, 0.0, 0.0, false);
  updateInstrumentDetectors();
}

void InstrumentWidget::resetSurface() {
  auto surface = getSurface();
  surface->updateDetectors();
  update();
}

/**
 * Select the tab to be displayed
 */
void InstrumentWidget::selectTab(int tab) {
  mControlsTab->setCurrentIndex(tab);
}

/**
 * Returns the named tab or the current tab if none supplied
 * @param title Optional title of a tab (default="")
 */
InstrumentWidgetTab *InstrumentWidget::getTab(const QString &title) const {
  QWidget *tab(nullptr);
  if (title.isEmpty())
    tab = mControlsTab->currentWidget();
  else {
    for (int i = 0; i < mControlsTab->count(); ++i) {
      if (mControlsTab->tabText(i) == title) {
        tab = mControlsTab->widget(i);
        break;
      }
    }
  }

  if (tab)
    return qobject_cast<InstrumentWidgetTab *>(tab);
  else
    return nullptr;
}

/**
 * @param tab An enumeration for the tab to select
 * @returns A pointer to the requested tab
 */
InstrumentWidgetTab *InstrumentWidget::getTab(const Tab tab) const {
  QWidget *widget = mControlsTab->widget(static_cast<int>(tab));
  if (widget)
    return qobject_cast<InstrumentWidgetTab *>(widget);
  else
    return nullptr;
}

/**
 * Opens Qt file dialog to select the filename.
 * The dialog opens in the directory used last for saving or the default user
 * directory.
 *
 * @param title :: The title of the dialog.
 * @param filters :: The filters
 * @param selectedFilter :: The selected filter.
 */
QString InstrumentWidget::getSaveFileName(const QString &title,
                                          const QString &filters,
                                          QString *selectedFilter) {
  QString filename = QFileDialog::getSaveFileName(this, title, m_savedialog_dir,
                                                  filters, selectedFilter);

  // If its empty, they cancelled the dialog
  if (!filename.isEmpty()) {
    // Save the directory used
    QFileInfo finfo(filename);
    m_savedialog_dir = finfo.dir().path();
  }
  return filename;
}

/**
 * Update the info text displayed at the bottom of the window.
 */
void InstrumentWidget::updateInfoText() { setInfoText(getSurfaceInfoText()); }

void InstrumentWidget::setSurfaceType(int type) {
  // we cannot do 3D without OpenGL
  if (type == FULL3D && !isGLEnabled()) {
    QMessageBox::warning(
        this, "Mantid - Warning",
        "OpenGL must be enabled to render the instrument in 3D.");
    return;
  }

  if (type < RENDERMODE_SIZE) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    SurfaceType surfaceType = SurfaceType(type);
    if (!m_instrumentActor)
      return;

    ProjectionSurface *surface = getSurface().get();
    int peakLabelPrecision = 6;
    bool showPeakRow = true;
    bool showPeakLabels = true;
    bool showPeakRelativeIntensity = true;
    if (surface) {
      peakLabelPrecision = surface->getPeakLabelPrecision();
      showPeakRow = surface->getShowPeakRowsFlag();
      showPeakLabels = surface->getShowPeakLabelsFlag();
    } else {
      QSettings settings;
      settings.beginGroup(InstrumentWidgetSettingsGroup);
      peakLabelPrecision = settings.value("PeakLabelPrecision", 2).toInt();
      showPeakRow = settings.value("ShowPeakRows", true).toBool();
      showPeakLabels = settings.value("ShowPeakLabels", true).toBool();

      // By default this is should be off for now.
      showPeakRelativeIntensity =
          settings.value("ShowPeakRelativeIntensities", false).toBool();
      settings.endGroup();
    }

    // Surface factory
    // If anything throws during surface creation, store error message here
    QString errorMessage;
    try {
      const auto &componentInfo = m_instrumentActor->componentInfo();
      if (!componentInfo.hasSample()) {
        throw InstrumentHasNoSampleError();
      }
      auto sample_pos = componentInfo.samplePosition();
      auto axis = getSurfaceAxis(surfaceType);

      m_maskTab->setDisabled(false);

      // create the surface
      if (surfaceType == FULL3D) {
        m_renderTab->forceLayers(false);

        if (m_instrumentActor->hasGridBank())
          m_maskTab->setDisabled(true);

        surface = new Projection3D(m_instrumentActor.get(),
                                   getInstrumentDisplayWidth(),
                                   getInstrumentDisplayHeight());
      } else if (surfaceType <= CYLINDRICAL_Z) {
        m_renderTab->forceLayers(true);
        surface =
            new UnwrappedCylinder(m_instrumentActor.get(), sample_pos, axis);
      } else if (surfaceType <= SPHERICAL_Z) {
        m_renderTab->forceLayers(true);
        surface =
            new UnwrappedSphere(m_instrumentActor.get(), sample_pos, axis);
      } else // SIDE_BY_SIDE
      {
        m_renderTab->forceLayers(true);
        surface = new PanelsSurface(m_instrumentActor.get(), sample_pos, axis);
      }
    } catch (InstrumentHasNoSampleError &) {
      QApplication::restoreOverrideCursor();
      throw;
    } catch (std::exception &e) {
      errorMessage = e.what();
    } catch (...) {
      errorMessage = "Unknown exception thrown.";
    }
    if (!errorMessage.isNull()) {
      // if exception was thrown roll back to the current surface type.
      QApplication::restoreOverrideCursor();
      QMessageBox::critical(
          this, "MantidPlot - Error",
          "Surface cannot be created because of an exception:\n\n  " +
              errorMessage + "\n\nPlease select a different surface type.");
      // if suface change was initialized by the GUI this should ensure its
      // consistency
      emit surfaceTypeChanged(m_surfaceType);
      return;
    }
    // end Surface factory

    m_surfaceType = surfaceType;
    surface->setPeakLabelPrecision(peakLabelPrecision);
    surface->setShowPeakRowsFlag(showPeakRow);
    surface->setShowPeakLabelsFlag(showPeakLabels);
    surface->setShowPeakRelativeIntensityFlag(showPeakRelativeIntensity);
    // set new surface
    setSurface(surface);

    // init tabs with new surface
    foreach (InstrumentWidgetTab *tab, m_tabs) { tab->initSurface(); }

    connect(surface, SIGNAL(executeAlgorithm(Mantid::API::IAlgorithm_sptr)),
            this, SLOT(executeAlgorithm(Mantid::API::IAlgorithm_sptr)));
    connect(surface, SIGNAL(updateInfoText()), this, SLOT(updateInfoText()),
            Qt::QueuedConnection);
    QApplication::restoreOverrideCursor();
  }
  emit surfaceTypeChanged(type);
  updateInfoText();
  update();
}

/**
 * Set the surface type from a string.
 * @param typeStr :: Symbolic name of the surface type: same as the names in
 * SurfaceType enum. Caseless.
 */
void InstrumentWidget::setSurfaceType(const QString &typeStr) {
  int typeIndex = 0;
  QString upperCaseStr = typeStr.toUpper();
  if (upperCaseStr == "FULL3D" || upperCaseStr == "3D") {
    typeIndex = 0;
  } else if (upperCaseStr == "CYLINDRICAL_X") {
    typeIndex = 1;
  } else if (upperCaseStr == "CYLINDRICAL_Y") {
    typeIndex = 2;
  } else if (upperCaseStr == "CYLINDRICAL_Z") {
    typeIndex = 3;
  } else if (upperCaseStr == "SPHERICAL_X") {
    typeIndex = 4;
  } else if (upperCaseStr == "SPHERICAL_Y") {
    typeIndex = 5;
  } else if (upperCaseStr == "SPHERICAL_Z") {
    typeIndex = 6;
  } else if (upperCaseStr == "SIDE_BY_SIDE") {
    typeIndex = 7;
  }
  setSurfaceType(typeIndex);
}

/**
 * Update the colormap on the render tab.
 */
void InstrumentWidget::setupColorMap() { emit colorMapChanged(); }

/**
 * Connected to QTabWidget::currentChanged signal
 */
void InstrumentWidget::tabChanged(int /*unused*/) { updateInfoText(); }

/**
 * Change color map button slot. This provides the file dialog box to select
 * colormap or sets it directly a string is provided
 * @param cmapNameOrPath Name of a color map or a file path
 */
void InstrumentWidget::changeColormap(const QString &cmapNameOrPath) {
  if (!m_instrumentActor)
    return;
  const auto currentCMap = m_instrumentActor->getCurrentColorMap();
  QString selection;
  if (cmapNameOrPath.isEmpty()) {
    // ask user
    selection = ColorMap::chooseColorMap(currentCMap, this);
    if (selection.isEmpty()) {
      // assume cancelled request
      return;
    }
  } else {
    selection = ColorMap::exists(cmapNameOrPath);
  }

  if (selection == currentCMap) {
    // selection matches current
    return;
  }
  m_instrumentActor->loadColorMap(selection);

  if (this->isVisible()) {
    setupColorMap();
    updateInstrumentView();
  }
}

QString InstrumentWidget::confirmDetectorOperation(const QString &opName,
                                                   const QString &inputWS,
                                                   int ndets) {
  QString message("This operation will affect %1 detectors.\nSelect output "
                  "workspace option:");
  QMessageBox prompt(this);
  prompt.setWindowTitle("MantidPlot");
  prompt.setText(message.arg(QString::number(ndets)));
  QPushButton *replace = prompt.addButton("Replace", QMessageBox::ActionRole);
  QPushButton *create = prompt.addButton("New", QMessageBox::ActionRole);
  prompt.addButton("Cancel", QMessageBox::ActionRole);
  prompt.exec();
  QString outputWS;
  if (prompt.clickedButton() == replace) {
    outputWS = inputWS;
  } else if (prompt.clickedButton() == create) {
    outputWS = inputWS + "_" + opName;
  } else {
    outputWS = "";
  }
  return outputWS;
}

/**
 * Convert a list of integers to a comma separated string of numbers
 */
QString InstrumentWidget::asString(const std::vector<int> &numbers) const {
  QString num_str;
  std::vector<int>::const_iterator iend = numbers.end();
  for (std::vector<int>::const_iterator itr = numbers.begin(); itr < iend;
       ++itr) {
    num_str += QString::number(*itr) + ",";
  }
  // Remove trailing comma
  num_str.chop(1);
  return num_str;
}

/// Set a maximum and minimum for the colour map range
void InstrumentWidget::setColorMapRange(double minValue, double maxValue) {
  emit colorMapRangeChanged(minValue, maxValue);
  update();
}

/// Set the minimum value of the colour map
void InstrumentWidget::setColorMapMinValue(double minValue) {
  emit colorMapMinValueChanged(minValue);
  update();
}

/// Set the maximumu value of the colour map
void InstrumentWidget::setColorMapMaxValue(double maxValue) {
  emit colorMapMaxValueChanged(maxValue);
  update();
}

/**
 * This is the callback for the combo box that selects the view direction
 */
void InstrumentWidget::setViewDirection(const QString &input) {
  auto p3d = boost::dynamic_pointer_cast<Projection3D>(getSurface());
  if (p3d) {
    p3d->setViewDirection(input);
  }
  updateInstrumentView();
  repaint();
}

/**
 *  For the scripting API. Selects a component in the tree and zooms to it.
 *  @param name The name of the component
 */
void InstrumentWidget::selectComponent(const QString &name) {
  emit requestSelectComponent(name);
}

/**
 * Set the scale type programmatically
 * @param type :: The scale choice
 */
void InstrumentWidget::setScaleType(ColorMap::ScaleType type) {
  emit scaleTypeChanged(static_cast<int>(type));
}

/**
 * Set the exponent for the Power scale type
 * @param nth_power :: The exponent choice
 */
void InstrumentWidget::setExponent(double nth_power) {
  emit nthPowerChanged(nth_power);
}

/**
 * This method opens a color dialog to pick the background color,
 * and then sets it.
 */
void InstrumentWidget::pickBackgroundColor() {
  QColor color = QColorDialog::getColor(Qt::green, this);
  setBackgroundColor(color);
}

/**
 * Saves the current image buffer as a png file.
 * @param filename Optional filename. Empty string raises a save dialog
 */
void InstrumentWidget::saveImage(QString filename) {
  QString defaultExt = ".png";
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  if (filename.isEmpty()) {
    QListIterator<QByteArray> itr(formats);
    QString filter("");
    while (itr.hasNext()) {
      filter += "*." + itr.next();
      if (itr.hasNext()) {
        filter += ";;";
      }
    }
    QString selectedFilter = "*" + defaultExt;
    filename = getSaveFileName("Save image ...", filter, &selectedFilter);

    // If its empty, they cancelled the dialog
    if (filename.isEmpty())
      return;
  }

  QFileInfo finfo(filename);
  QString ext = finfo.completeSuffix();

  if (ext.isEmpty()) {
    filename += defaultExt;
  } else {
    if (!formats.contains(ext.toLatin1())) {
      QString msg("Unsupported file extension. Choose one of the following: ");
      QListIterator<QByteArray> itr(formats);
      while (itr.hasNext()) {
        msg += itr.next() + ", ";
      }
      msg.chop(2); // Remove last space and comma
      QMessageBox::warning(this, "MantidPlot", msg);
      return;
    }
  }

  if (isGLEnabled()) {
    m_InstrumentDisplay->saveToFile(filename);
  } else {
    m_simpleDisplay->saveToFile(filename);
  }
}

/**
 * Use the file dialog to select a filename to save grouping.
 */
QString InstrumentWidget::getSaveGroupingFilename() {
  QString filename =
      QFileDialog::getSaveFileName(this, "Save grouping file", m_savedialog_dir,
                                   "Grouping (*.xml);;All files (*)");

  // If its empty, they cancelled the dialog
  if (!filename.isEmpty()) {
    // Save the directory used
    QFileInfo finfo(filename);
    m_savedialog_dir = finfo.dir().path();
  }

  return filename;
}

///**
// * Update the text display that informs the user of the current mode and
// details about it
// */
void InstrumentWidget::setInfoText(const QString &text) {
  mInteractionInfo->setText(text);
}

/**
 * Save properties of the window a persistent store
 */
void InstrumentWidget::saveSettings() {
  QSettings settings;
  settings.beginGroup(InstrumentWidgetSettingsGroup);
  if (m_InstrumentDisplay)
    settings.setValue("BackgroundColor",
                      m_InstrumentDisplay->currentBackgroundColor());
  auto surface = getSurface();
  if (surface) {
    // if surface is null istrument view wasn't created and there is nothing to
    // save
    settings.setValue("PeakLabelPrecision",
                      getSurface()->getPeakLabelPrecision());
    settings.setValue("ShowPeakRows", getSurface()->getShowPeakRowsFlag());
    settings.setValue("ShowPeakLabels", getSurface()->getShowPeakLabelsFlag());
    settings.setValue("ShowPeakRelativeIntensities",
                      getSurface()->getShowPeakRelativeIntensityFlag());
    foreach (InstrumentWidgetTab *tab, m_tabs) { tab->saveSettings(settings); }
  }
  settings.endGroup();
}

void InstrumentWidget::helpClicked() {
  MantidDesktopServices::openUrl(
      QUrl("http://www.mantidproject.org/MantidPlot:_Instrument_View"));
}

void InstrumentWidget::set3DAxesState(bool on) {
  auto p3d = boost::dynamic_pointer_cast<Projection3D>(getSurface());
  if (p3d) {
    p3d->set3DAxesState(on);
    updateInstrumentView();
  }
}

void InstrumentWidget::finishHandle(const Mantid::API::IAlgorithm *alg) {
  UNUSED_ARG(alg);
  emit needSetIntegrationRange(m_instrumentActor->minBinValue(),
                               m_instrumentActor->maxBinValue());
  // m_instrumentActor->update();
  // m_InstrumentDisplay->refreshView();
}

void InstrumentWidget::changeScaleType(int type) {
  m_instrumentActor->changeScaleType(type);
  setupColorMap();
  updateInstrumentView();
}

void InstrumentWidget::changeNthPower(double nth_power) {
  m_instrumentActor->changeNthPower(nth_power);
  setupColorMap();
  updateInstrumentView();
}

void InstrumentWidget::changeColorMapMinValue(double minValue) {
  m_instrumentActor->setMinValue(minValue);
  setupColorMap();
  updateInstrumentView();
}

/// Set the maximumu value of the colour map
void InstrumentWidget::changeColorMapMaxValue(double maxValue) {
  m_instrumentActor->setMaxValue(maxValue);
  setupColorMap();
  updateInstrumentView();
}

void InstrumentWidget::changeColorMapRange(double minValue, double maxValue) {
  m_instrumentActor->setMinMaxRange(minValue, maxValue);
  setupColorMap();
  updateInstrumentView();
}

void InstrumentWidget::setWireframe(bool on) {
  auto p3d = boost::dynamic_pointer_cast<Projection3D>(getSurface());
  if (p3d) {
    p3d->setWireframe(on);
  }
  updateInstrumentView();
}

/**
 * Set new integration range but don't update XIntegrationControl (because the
 * control calls this slot)
 */
void InstrumentWidget::setIntegrationRange(double xmin, double xmax) {
  m_instrumentActor->setIntegrationRange(xmin, xmax);
  setupColorMap();
  updateInstrumentDetectors();
  emit integrationRangeChanged(xmin, xmax);
}

/**
 * Set new integration range and update XIntegrationControl. To be called from
 * python.
 */
void InstrumentWidget::setBinRange(double xmin, double xmax) {
  m_xIntegration->setRange(xmin, xmax);
}

/**
 * Update the display to view a selected component. The selected component
 * is visible the rest of the instrument is hidden.
 * @param id :: The component id.
 */
void InstrumentWidget::componentSelected(size_t componentIndex) {
  auto surface = getSurface();
  if (surface) {
    surface->componentSelected(componentIndex);
    updateInstrumentView();
  }
}

void InstrumentWidget::executeAlgorithm(const QString & /*unused*/,
                                        const QString & /*unused*/) {
  // emit execMantidAlgorithm(alg_name, param_list, this);
}

void InstrumentWidget::executeAlgorithm(Mantid::API::IAlgorithm_sptr alg) {
  try {
    alg->executeAsync();
  } catch (Poco::NoThreadAvailableException &) {
    return;
  }

  return;
}

/**
 * Set the type of the view (SurfaceType).
 * @param type :: String code for the type. One of:
 * FULL3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z, SPHERICAL_X,
 * SPHERICAL_Y, SPHERICAL_Z
 */
void InstrumentWidget::setViewType(const QString &type) {
  QString type_upper = type.toUpper();
  SurfaceType itype = FULL3D;
  if (type_upper == "FULL3D") {
    itype = FULL3D;
  } else if (type_upper == "CYLINDRICAL_X") {
    itype = CYLINDRICAL_X;
  } else if (type_upper == "CYLINDRICAL_Y") {
    itype = CYLINDRICAL_Y;
  } else if (type_upper == "CYLINDRICAL_Z") {
    itype = CYLINDRICAL_Z;
  } else if (type_upper == "SPHERICAL_X") {
    itype = SPHERICAL_X;
  } else if (type_upper == "SPHERICAL_Y") {
    itype = SPHERICAL_Y;
  } else if (type_upper == "SPHERICAL_Z") {
    itype = SPHERICAL_Z;
  }
  setSurfaceType(itype);
}

void InstrumentWidget::dragEnterEvent(QDragEnterEvent *e) {
  QString name = e->mimeData()->objectName();
  if (name == "MantidWorkspace") {
    e->accept();
  } else {
    e->ignore();
  }
}

void InstrumentWidget::dropEvent(QDropEvent *e) {
  QString name = e->mimeData()->objectName();
  if (name == "MantidWorkspace") {
    QStringList wsNames = e->mimeData()->text().split("\n");
    foreach (const auto &wsName, wsNames) {
      if (this->overlay(wsName))
        e->accept();
    }
  }
  e->ignore();
}

/**
 * Filter events directed to m_InstrumentDisplay and ContextMenuEvent in
 * particular.
 * @param obj :: Object which events will be filtered.
 * @param ev :: An ingoing event.
 */
bool InstrumentWidget::eventFilter(QObject *obj, QEvent *ev) {
  if (ev->type() == QEvent::ContextMenu &&
      (dynamic_cast<MantidGLWidget *>(obj) == m_InstrumentDisplay ||
       dynamic_cast<SimpleWidget *>(obj) == m_simpleDisplay) &&
      getSurface() && getSurface()->canShowContextMenu()) {
    // an ugly way of preventing the curve in the pick tab's miniplot
    // disappearing when
    // cursor enters the context menu
    m_instrumentDisplayContextMenuOn = true;
    QMenu context(this);
    // add tab specific actions
    InstrumentWidgetTab *tab = getTab();
    tab->addToDisplayContextMenu(context);
    if (getSurface()->hasPeakOverlays()) {
      context.addSeparator();
      context.addAction(m_clearPeakOverlays);
      context.addAction(m_clearAlignment);
    }
    if (!context.isEmpty()) {
      context.exec(QCursor::pos());
    }
    m_instrumentDisplayContextMenuOn = false;
    return true;
  }
  return QWidget::eventFilter(obj, ev);
}

/**
 * Disable colormap autoscaling
 */
void InstrumentWidget::disableColorMapAutoscaling() {
  setColorMapAutoscaling(false);
}

/**
 * Set on / off autoscaling of the color map on the render tab.
 * @param on :: On or Off.
 */
void InstrumentWidget::setColorMapAutoscaling(bool on) {
  m_instrumentActor->setAutoscaling(on);
  setupColorMap();
  updateInstrumentView();
}

/**
 *  Overlay a workspace with the given name
 * @param wsName The name of a workspace in the ADS
 * @returns True if the overlay was successful, false otherwise
 */
bool InstrumentWidget::overlay(const QString &wsName) {
  using namespace Mantid::API;

  auto workspace = getWorkspaceFromADS(wsName.toStdString());

  auto pws = boost::dynamic_pointer_cast<IPeaksWorkspace>(workspace);
  auto table = boost::dynamic_pointer_cast<ITableWorkspace>(workspace);
  auto mask = boost::dynamic_pointer_cast<IMaskWorkspace>(workspace);

  if (!pws && !table && !mask) {
    QMessageBox::warning(this, "Mantid - Warning",
                         "Work space called '" + wsName +
                             "' is not suitable."
                             " Please select another workspace. ");
    return false;
  }

  if (pws) {
    overlayPeaksWorkspace(pws);
  } else if (table) {
    overlayShapesWorkspace(table);
  } else if (mask) {
    overlayMaskedWorkspace(mask);
  }

  return true;
}

/**
 * Remove all peak overlays from the instrument display.
 */
void InstrumentWidget::clearPeakOverlays() {
  getSurface()->clearPeakOverlays();
  updateInstrumentView();
}

void InstrumentWidget::clearAlignmentPlane() {
  getSurface()->clearAlignmentPlane();
  updateInstrumentView();
}

/**
 * Set the precision (significant digits) with which the HKL peak labels are
 * displayed.
 * @param n :: Precision, > 0
 */
void InstrumentWidget::setPeakLabelPrecision(int n) {
  getSurface()->setPeakLabelPrecision(n);
  updateInstrumentView();
}

/**
 * Enable or disable the show peak row flag
 * @param on :: True to show, false to hide.
 */
void InstrumentWidget::setShowPeakRowFlag(bool on) {
  getSurface()->setShowPeakRowsFlag(on);
  updateInstrumentView();
}

/**
 * Enable or disable the show peak hkl labels flag
 * @param on :: True to show, false to hide.
 */
void InstrumentWidget::setShowPeakLabelsFlag(bool on) {
  getSurface()->setShowPeakLabelsFlag(on);
  updateInstrumentView();
}

/**
 * Enable or disable indication of relative peak intensities
 *
 * @param on :: True to show, false to hide.
 */
void InstrumentWidget::setShowPeakRelativeIntensity(bool on) {
  getSurface()->setShowPeakRelativeIntensityFlag(on);
  updateInstrumentView();
}

/**
 * Set background color of the instrument display
 * @param color :: New background colour.
 */
void InstrumentWidget::setBackgroundColor(const QColor &color) {
  if (m_InstrumentDisplay)
    m_InstrumentDisplay->setBackgroundColor(color);
}

/**
 * Get the surface info string
 */
QString InstrumentWidget::getSurfaceInfoText() const {
  ProjectionSurface *surface = getSurface().get();
  return surface ? surface->getInfoText() : "";
}

/**
 * Get pointer to the projection surface
 */
ProjectionSurface_sptr InstrumentWidget::getSurface() const {
  if (m_InstrumentDisplay) {
    return m_InstrumentDisplay->getSurface();
  } else if (m_simpleDisplay) {
    return m_simpleDisplay->getSurface();
  }
  return ProjectionSurface_sptr();
}

/**
 * Set newly created projection surface
 * @param surface :: Pointer to the new surace.
 */
void InstrumentWidget::setSurface(ProjectionSurface *surface) {
  ProjectionSurface_sptr sharedSurface(surface);
  if (m_InstrumentDisplay) {
    m_InstrumentDisplay->setSurface(sharedSurface);
    m_InstrumentDisplay->update();
  }
  if (m_simpleDisplay) {
    m_simpleDisplay->setSurface(sharedSurface);
    m_simpleDisplay->update();
  }
  UnwrappedSurface *unwrappedSurface =
      dynamic_cast<UnwrappedSurface *>(surface);
  if (unwrappedSurface) {
    m_renderTab->flipUnwrappedView(unwrappedSurface->isFlippedView());
  }
}

/// Return the width of the instrunemt display
int InstrumentWidget::getInstrumentDisplayWidth() const {
  if (m_InstrumentDisplay) {
    return m_InstrumentDisplay->width();
  } else if (m_simpleDisplay) {
    return m_simpleDisplay->width();
  }
  return 0;
}

/// Return the height of the instrunemt display
int InstrumentWidget::getInstrumentDisplayHeight() const {
  if (m_InstrumentDisplay) {
    return m_InstrumentDisplay->height();
  } else if (m_simpleDisplay) {
    return m_simpleDisplay->height();
  }
  return 0;
}

/// Redraw the instrument view
/// @param picking :: Set to true to update the picking image regardless the
/// interaction
///   mode of the surface.
void InstrumentWidget::updateInstrumentView(bool picking) {
  if (m_InstrumentDisplay && m_instrumentDisplayLayout->currentWidget() ==
                                 dynamic_cast<QWidget *>(m_InstrumentDisplay)) {
    m_InstrumentDisplay->updateView(picking);
  } else {
    m_simpleDisplay->updateView(picking);
  }
}

/// Recalculate the colours and redraw the instrument view
void InstrumentWidget::updateInstrumentDetectors() {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  if (m_InstrumentDisplay && m_instrumentDisplayLayout->currentWidget() ==
                                 dynamic_cast<QWidget *>(m_InstrumentDisplay)) {
    m_InstrumentDisplay->updateDetectors();
  } else {
    m_simpleDisplay->updateDetectors();
  }
  QApplication::restoreOverrideCursor();
}

void InstrumentWidget::deletePeaksWorkspace(
    Mantid::API::IPeaksWorkspace_sptr pws) {
  this->getSurface()->deletePeaksWorkspace(pws);
  updateInstrumentView();
}

/**
 * Choose which widget to use.
 * @param yes :: True to use the OpenGL one or false to use the Simple
 */
void InstrumentWidget::selectOpenGLDisplay(bool yes) {
  int widgetIndex = yes ? 0 : 1;
  const int oldIndex = m_instrumentDisplayLayout->currentIndex();
  if (oldIndex == widgetIndex)
    return;
  m_instrumentDisplayLayout->setCurrentIndex(widgetIndex);
  auto surface = getSurface();
  if (surface) {
    surface->updateView();
  }
}

/// Public slot to toggle between the GL and simple instrument display widgets
void InstrumentWidget::enableOpenGL(bool on) {
  enableGL(on);
  emit glOptionChanged(on);
}

/// Private slot to toggle between the GL and simple instrument display widgets
void InstrumentWidget::enableGL(bool on) {
  m_useOpenGL = on;
  selectOpenGLDisplay(isGLEnabled());
}

/// True if the GL instrument display is currently on
bool InstrumentWidget::isGLEnabled() const { return m_useOpenGL; }

/**
 * Create and add the tab widgets.
 */
void InstrumentWidget::createTabs(QSettings &settings) {
  // Render Controls
  m_renderTab = new InstrumentWidgetRenderTab(this);
  connect(m_renderTab, SIGNAL(setAutoscaling(bool)), this,
          SLOT(setColorMapAutoscaling(bool)));
  connect(m_renderTab, SIGNAL(rescaleColorMap()), this, SLOT(setupColorMap()));
  mControlsTab->addTab(m_renderTab, QString("Render"));
  m_renderTab->loadSettings(settings);

  // Pick controls
  m_pickTab = new InstrumentWidgetPickTab(this);
  mControlsTab->addTab(m_pickTab, QString("Pick"));
  m_pickTab->loadSettings(settings);

  // Mask controls
  m_maskTab = new InstrumentWidgetMaskTab(this);
  mControlsTab->addTab(m_maskTab, QString("Draw"));
  connect(m_maskTab, SIGNAL(executeAlgorithm(const QString &, const QString &)),
          this, SLOT(executeAlgorithm(const QString &, const QString &)));
  connect(m_xIntegration, SIGNAL(changed(double, double)), m_maskTab,
          SLOT(changedIntegrationRange(double, double)));
  m_maskTab->loadSettings(settings);

  // Instrument tree controls
  m_treeTab = new InstrumentWidgetTreeTab(this);
  mControlsTab->addTab(m_treeTab, QString("Instrument"));
  m_treeTab->loadSettings(settings);

  connect(mControlsTab, SIGNAL(currentChanged(int)), this,
          SLOT(tabChanged(int)));

  m_tabs << m_renderTab << m_pickTab << m_maskTab << m_treeTab;
}

/**
 * Return a name for a group in QSettings to store InstrumentWidget
 * configuration.
 */
QString InstrumentWidget::getSettingsGroupName() const {
  return QString::fromLatin1(InstrumentWidgetSettingsGroup);
}

/**
 * Construct a name for a group in QSettings to store instrument-specific
 * configuration.
 */
QString InstrumentWidget::getInstrumentSettingsGroupName() const {
  return QString::fromLatin1(InstrumentWidgetSettingsGroup) + "/" +
         QString::fromStdString(getInstrumentActor().getInstrumentName());
}

bool InstrumentWidget::hasWorkspace(const std::string &wsName) const {
  return wsName == getWorkspaceNameStdString();
}

void InstrumentWidget::handleWorkspaceReplacement(
    const std::string &wsName, const boost::shared_ptr<Workspace> workspace) {
  // Replace current workspace
  if (hasWorkspace(wsName)) {
    if (m_instrumentActor) {
      // Check if it's still the same workspace underneath (as well as having
      // the same name)
      auto matrixWS =
          boost::dynamic_pointer_cast<const MatrixWorkspace>(workspace);
      if (!matrixWS || matrixWS->detectorInfo().size() == 0) {
        emit preDeletingHandle();
        close();
        return;
      }
      // try to detect if the instrument changes (unlikely if the workspace
      // hasn't, but theoretically possible)
      bool resetGeometry =
          matrixWS->detectorInfo().size() != m_instrumentActor->ndetectors();
      try {
        if (matrixWS == m_instrumentActor->getWorkspace() && !resetGeometry) {
          m_instrumentActor->updateColors();
          setupColorMap();
          updateInstrumentView();
        }
      } catch (std::runtime_error &) {
        resetInstrument(resetGeometry);
      }
    }
  }
}

/**
 * Closes the window if the associated workspace is deleted.
 * @param ws_name :: Name of the deleted workspace.
 * @param workspace_ptr :: Pointer to the workspace to be deleted
 */
void InstrumentWidget::preDeleteHandle(
    const std::string &ws_name,
    const boost::shared_ptr<Workspace> workspace_ptr) {
  if (hasWorkspace(ws_name)) {
    emit preDeletingHandle();
    close();
    return;
  }
  Mantid::API::IPeaksWorkspace_sptr pws =
      boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(workspace_ptr);
  if (pws) {
    deletePeaksWorkspace(pws);
    return;
  }
}

void InstrumentWidget::afterReplaceHandle(
    const std::string &wsName, const boost::shared_ptr<Workspace> workspace) {
  handleWorkspaceReplacement(wsName, workspace);
}

void InstrumentWidget::renameHandle(const std::string &oldName,
                                    const std::string &newName) {
  if (hasWorkspace(oldName)) {
    renameWorkspace(newName);
    setWindowTitle(QString("Instrument - ") + getWorkspaceName());
  }
}

void InstrumentWidget::clearADSHandle() {
  emit clearingHandle();
  close();
}

/**
 * Overlay a peaks workspace on the surface projection
 * @param ws :: peaks workspace to overlay
 */
void InstrumentWidget::overlayPeaksWorkspace(IPeaksWorkspace_sptr ws) {
  auto surface = getUnwrappedSurface();
  if (surface) {
    surface->setPeaksWorkspace(ws);
    updateInstrumentView();
  }
}

/**
 * Overlay a mask workspace on the surface projection
 * @param ws :: mask workspace to overlay
 */
void InstrumentWidget::overlayMaskedWorkspace(IMaskWorkspace_sptr ws) {
  auto &actor = getInstrumentActor();
  actor.setMaskMatrixWorkspace(
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws));
  actor.updateColors();
  updateInstrumentDetectors();
  emit maskedWorkspaceOverlayed();
}

/**
 * Overlay a table workspace containing shape parameters
 * @param ws :: a workspace of shape parameters to create
 */
void InstrumentWidget::overlayShapesWorkspace(ITableWorkspace_sptr ws) {
  auto surface = getUnwrappedSurface();
  if (surface) {
    surface->loadShapesFromTableWorkspace(ws);
    updateInstrumentView();
  }
}

/**
 * Get a workspace from the ADS using its name
 * @param name :: name of the workspace
 * @return a handle to the workspace (null if not found)
 */
Workspace_sptr InstrumentWidget::getWorkspaceFromADS(const std::string &name) {
  Workspace_sptr workspace;

  try {
    workspace = AnalysisDataService::Instance().retrieve(name);
  } catch (std::runtime_error) {
    QMessageBox::warning(this, "Mantid - Warning",
                         "No workspace called '" +
                             QString::fromStdString(name) + "' found. ");
    return nullptr;
  }

  return workspace;
}

/**
 * Get an unwrapped surface
 * @return a handle to the unwrapped surface (or null if view was not found).
 */
boost::shared_ptr<UnwrappedSurface> InstrumentWidget::getUnwrappedSurface() {
  auto surface = boost::dynamic_pointer_cast<UnwrappedSurface>(getSurface());
  if (!surface) {
    QMessageBox::warning(
        this, "Mantid - Warning",
        "Please change to an unwrapped view to overlay a workspace.");
    return nullptr;
  }
  return surface;
}

int InstrumentWidget::getCurrentTab() const {
  return mControlsTab->currentIndex();
}

/**
 * Save the state of the instrument widget to a project file.
 * @return string representing the current state of the instrumet widget.
 */
std::string InstrumentWidget::saveToProject() const {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  TSVSerialiser tsv;

  // serialise widget properties
  tsv.writeLine("WorkspaceName") << getWorkspaceNameStdString();
  tsv.writeLine("SurfaceType") << getSurfaceType();
  tsv.writeSection("surface", getSurface()->saveToProject());
  tsv.writeLine("CurrentTab") << getCurrentTab();
  tsv.writeLine("EnergyTransfer")
      << m_xIntegration->getMinimum() << m_xIntegration->getMaximum();

  // serialise widget subsections
  tsv.writeSection("actor", m_instrumentActor->saveToProject());
  tsv.writeSection("tabs", saveTabs());

  return tsv.outputLines();
#else
  throw std::runtime_error(
      "InstrumentWidget::saveToProject() not implemented for Qt >= 5");
#endif
}

/**
 * Save each tab on the widget to a string.
 * @return a string representing the state of each tab on the widget
 */
std::string InstrumentWidget::saveTabs() const {
  std::string tabContents;
  for (auto tab : m_tabs) {
    tabContents += tab->saveToProject();
  }
  return tabContents;
}

/**
 * Load the state of each tab from a string.
 * @param lines :: string containing the tabs states from the project file.
 */
void InstrumentWidget::loadTabs(const std::string &lines) const {
  for (auto tab : m_tabs) {
    tab->loadFromProject(lines);
  }
}

/**
 * Load the state of the widget from a project file.
 * @param lines :: string containing the state of the widget from the project
 * file.
 */
void InstrumentWidget::loadFromProject(const std::string &lines) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  TSVSerialiser tsv(lines);

  if (tsv.selectLine("SurfaceType")) {
    int surfaceType;
    tsv >> surfaceType;
    setSurfaceType(surfaceType);
  }

  if (tsv.selectSection("actor")) {
    std::string actorLines;
    tsv >> actorLines;
    m_instrumentActor->loadFromProject(actorLines);
  }

  if (tsv.selectLine("CurrentTab")) {
    int tab;
    tsv >> tab;
    selectTab(tab);
  }

  if (tsv.selectLine("EnergyTransfer")) {
    double min, max;
    tsv >> min >> max;
    setBinRange(min, max);
  }

  if (tsv.selectSection("Surface")) {
    std::string surfaceLines;
    tsv >> surfaceLines;
    getSurface()->loadFromProject(surfaceLines);
  }

  if (tsv.selectSection("tabs")) {
    std::string tabLines;
    tsv >> tabLines;
    loadTabs(tabLines);
  }

  updateInstrumentView();
#else
  Q_UNUSED(lines);
  throw std::runtime_error(
      "InstrumentWidget::loadFromProject() not implemented for Qt >= 5");
#endif
}

} // namespace MantidWidgets
} // namespace MantidQt
