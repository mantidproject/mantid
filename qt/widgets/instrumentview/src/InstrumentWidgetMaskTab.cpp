// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h"
#include "MantidQtWidgets/InstrumentView/DetXMLFile.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"
// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

#include <Poco/Path.h>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QToolTip>
#include <QVBoxLayout>

#include "MantidQtWidgets/Common/FileDialogHandler.h"

#include <algorithm>
#include <cfloat>
#include <fstream>
#include <numeric>

using Mantid::API::AlgorithmManager;

namespace MantidQt::MantidWidgets {
InstrumentWidgetMaskTab::InstrumentWidgetMaskTab(InstrumentWidget *instrWidget)
    : InstrumentWidgetTab(instrWidget), m_activity(Select), m_hasMaskToApply(false), m_maskBins(false),
      m_userEditing(true), m_groupManager(nullptr), m_stringManager(nullptr), m_doubleManager(nullptr),
      m_browser(nullptr), m_left(nullptr), m_top(nullptr), m_right(nullptr), m_bottom(nullptr), m_rotation(nullptr) {

  // main layout
  QVBoxLayout *layout = new QVBoxLayout(this);

  m_activeTool = new QLabel(this);
  layout->addWidget(m_activeTool);

  // Create the tool buttons

  m_move = new QPushButton();
  m_move->setCheckable(true);
  m_move->setAutoExclusive(true);
  m_move->setIcon(QIcon(":/PickTools/zoom.png"));
  m_move->setToolTip("Move the instrument (Ctrl+Alt+M)");
  m_move->setShortcut(QKeySequence("Ctrl+Alt+M"));

  m_pointer = new QPushButton();
  m_pointer->setCheckable(true);
  m_pointer->setAutoExclusive(true);
  m_pointer->setIcon(QIcon(":/MaskTools/selection-edit.png"));
  m_pointer->setToolTip("Select and edit shapes (Ctrl+Alt+P)");
  m_pointer->setShortcut(QKeySequence("Ctrl+Alt+P"));

  m_ellipse = new QPushButton();
  m_ellipse->setCheckable(true);
  m_ellipse->setAutoExclusive(true);
  m_ellipse->setIcon(QIcon(":/MaskTools/selection-circle.png"));
  m_ellipse->setToolTip("Draw an ellipse (Ctrl+Alt+E)");
  m_ellipse->setShortcut(QKeySequence("Ctrl+Alt+E"));

  m_rectangle = new QPushButton();
  m_rectangle->setCheckable(true);
  m_rectangle->setAutoExclusive(true);
  m_rectangle->setIcon(QIcon(":/MaskTools/selection-box.png"));
  m_rectangle->setToolTip("Draw a rectangle (Ctrl+Alt+R)");
  m_rectangle->setShortcut(QKeySequence("Ctrl+Alt+R"));

  m_ring_ellipse = new QPushButton();
  m_ring_ellipse->setCheckable(true);
  m_ring_ellipse->setAutoExclusive(true);
  m_ring_ellipse->setIcon(QIcon(":/MaskTools/selection-circle-ring.png"));
  m_ring_ellipse->setToolTip("Draw an elliptical ring (Shift+Alt+E)");
  m_ring_ellipse->setShortcut(QKeySequence("Shift+Alt+E"));

  m_ring_rectangle = new QPushButton();
  m_ring_rectangle->setCheckable(true);
  m_ring_rectangle->setAutoExclusive(true);
  m_ring_rectangle->setIcon(QIcon(":/MaskTools/selection-box-ring.png"));
  m_ring_rectangle->setToolTip("Draw a rectangular ring (Shift+Alt+R)");
  m_ring_rectangle->setShortcut(QKeySequence("Shift+Alt+R"));

  m_sector = new QPushButton();
  m_sector->setCheckable(true);
  m_sector->setAutoExclusive(true);
  m_sector->setIcon(QIcon(":/MaskTools/selection-sector.png"));
  m_sector->setToolTip("Draw a circle sector (Shift+Alt+S)");
  m_sector->setShortcut(QKeySequence("Shift+Alt+S"));

  m_pixel = new QPushButton();
  m_pixel->setCheckable(true);
  m_pixel->setAutoExclusive(true);
  m_pixel->setIcon(QIcon(":/PickTools/selection-pointer.png"));
  m_pixel->setToolTip("Select a pixel");

  m_tube = new QPushButton();
  m_tube->setCheckable(true);
  m_tube->setAutoExclusive(true);
  m_tube->setIcon(QIcon(":/PickTools/selection-tube.png"));
  m_tube->setToolTip("Select a tube/bank");

  m_free_draw = new QPushButton();
  m_free_draw->setCheckable(true);
  m_free_draw->setAutoExclusive(true);
  m_free_draw->setIcon(QIcon(":/MaskTools/brush.png"));
  m_free_draw->setToolTip("Draw an arbitrary shape (Shift+Alt+A)");
  m_free_draw->setShortcut(QKeySequence("Shift+Alt+A"));

  auto *toolBox = new QGridLayout();
  layout->addLayout(toolBox);

  toolBox->addWidget(m_move, 0, 0);
  toolBox->addWidget(m_pointer, 0, 1);
  toolBox->addWidget(m_ellipse, 0, 2);
  toolBox->addWidget(m_rectangle, 0, 3);
  toolBox->addWidget(m_ring_ellipse, 0, 4);
  toolBox->addWidget(m_ring_rectangle, 0, 5);
  toolBox->addWidget(m_sector, 0, 6);
  toolBox->addWidget(m_free_draw, 0, 7);
  toolBox->addWidget(m_pixel, 1, 0);
  toolBox->addWidget(m_tube, 1, 1);
  toolBox->setColumnStretch(8, 1);
  toolBox->setSpacing(2);

  connect(m_move, SIGNAL(clicked()), this, SLOT(setActivity()));
  connect(m_pointer, SIGNAL(clicked()), this, SLOT(setActivity()));
  connect(m_ellipse, SIGNAL(clicked()), this, SLOT(setActivity()));
  connect(m_rectangle, SIGNAL(clicked()), this, SLOT(setActivity()));
  connect(m_ring_ellipse, SIGNAL(clicked()), this, SLOT(setActivity()));
  connect(m_ring_rectangle, SIGNAL(clicked()), this, SLOT(setActivity()));
  connect(m_sector, SIGNAL(clicked()), this, SLOT(setActivity()));
  connect(m_tube, SIGNAL(clicked()), this, SLOT(setActivity()));
  connect(m_pixel, SIGNAL(clicked()), this, SLOT(setActivity()));
  connect(m_free_draw, SIGNAL(clicked()), this, SLOT(setActivity()));
  m_move->setChecked(true);

  // create mask/group switch
  m_masking_on = new QRadioButton("Mask");
  m_grouping_on = new QRadioButton("Group");
  m_roi_on = new QRadioButton("ROI");
  m_masking_on->setChecked(true);
  connect(m_masking_on, SIGNAL(clicked()), this, SLOT(toggleMaskGroup()));
  connect(m_grouping_on, SIGNAL(clicked()), this, SLOT(toggleMaskGroup()));
  connect(m_roi_on, SIGNAL(clicked()), this, SLOT(toggleMaskGroup()));
  auto *radioLayout = new QHBoxLayout();
  radioLayout->addWidget(m_masking_on);
  radioLayout->addWidget(m_roi_on);
  radioLayout->addWidget(m_grouping_on);
  radioLayout->setMargin(0);
  auto *radioGroup = new QGroupBox();
  radioGroup->setStyleSheet("border: none;");
  radioGroup->setLayout(radioLayout);

  layout->addWidget(radioGroup);

  // Create property browser

  /* Create property managers: they create, own properties, get and set values
   */

  m_groupManager = new QtGroupPropertyManager(this);
  m_doubleManager = new QtDoublePropertyManager(this);
  connect(m_doubleManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(doubleChanged(QtProperty *)));

  /* Create editors and assign them to the managers */

  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory(this);

  m_browser = new QtTreePropertyBrowser();
  m_browser->setFactoryForManager(m_doubleManager, doubleEditorFactory);

  layout->addWidget(m_browser);

  // Algorithm buttons

  m_applyToData = new QPushButton("Apply to Data (Cannot be reverted)");
  m_applyToData->setToolTip("Apply current detector and bin masks to the data "
                            "workspace. Cannot be reverted.");
  connect(m_applyToData, SIGNAL(clicked()), this, SLOT(applyMask()));

  m_applyToView = new QPushButton("Apply to View");
  m_applyToView->setToolTip("Apply current mask to the view.");
  connect(m_applyToView, SIGNAL(clicked()), this, SLOT(applyMaskToView()));

  m_saveShapesToTable = new QPushButton("Save Shapes to Table");
  m_saveShapesToTable->setToolTip("Store the current Mask/ROI/Group shapes as a table");
  connect(m_saveShapesToTable, SIGNAL(clicked()), this, SLOT(saveShapesToTable()));

  m_clearAll = new QPushButton("Clear All");
  m_clearAll->setToolTip("Clear all masking that have not been applied to the data.");
  connect(m_clearAll, SIGNAL(clicked()), this, SLOT(clearMask()));

  m_save_as_workspace_exclude = new QAction("As Detector Mask to workspace", this);
  m_save_as_workspace_exclude->setToolTip("Save current detector mask to mask workspace.");
  connect(m_save_as_workspace_exclude, SIGNAL(triggered()), this, SLOT(saveMaskToWorkspace()));

  m_save_as_workspace_include = new QAction("As Detector ROI to workspace", this);
  m_save_as_workspace_include->setToolTip("Save current detector mask as ROI to mask workspace.");
  connect(m_save_as_workspace_include, SIGNAL(triggered()), this, SLOT(saveInvertedMaskToWorkspace()));

  m_save_as_file_exclude = new QAction("As Detector Mask to file", this);
  m_save_as_file_exclude->setToolTip("Save current detector mask to mask file.");
  connect(m_save_as_file_exclude, SIGNAL(triggered()), this, SLOT(saveMaskToFile()));

  m_save_as_file_include = new QAction("As Detector ROI to file", this);
  m_save_as_file_include->setToolTip("Save current mask as ROI to mask file.");
  connect(m_save_as_file_include, SIGNAL(triggered()), this, SLOT(saveInvertedMaskToFile()));

  m_save_as_cal_file_exclude = new QAction("As Detector Mask to cal file", this);
  m_save_as_cal_file_exclude->setToolTip("Save current detector mask to cal file.");
  connect(m_save_as_cal_file_exclude, SIGNAL(triggered()), this, SLOT(saveMaskToCalFile()));

  m_save_as_cal_file_include = new QAction("As Detector ROI to cal file", this);
  m_save_as_cal_file_include->setToolTip("Save current detector mask as ROI to cal file.");
  connect(m_save_as_cal_file_include, SIGNAL(triggered()), this, SLOT(saveInvertedMaskToCalFile()));

  m_save_as_table_xrange_exclude = new QAction("As Detector Mask to table", this);
  m_save_as_table_xrange_exclude->setToolTip("Save current detector mask to a table workspace with x-range. "
                                             "The name of output table workspace is 'MaskBinTable'. "
                                             "If the output table workspace has alrady exist, then "
                                             "the newly masked detectors will be added to output workspace.");
  connect(m_save_as_table_xrange_exclude, SIGNAL(triggered()), this, SLOT(saveMaskToTable()));

  m_save_group_file_include = new QAction("As include group to file", this);
  m_save_group_file_include->setToolTip("Save current mask as include group to a file.");
  connect(m_save_group_file_include, SIGNAL(triggered()), this, SLOT(saveIncludeGroupToFile()));

  m_save_group_file_exclude = new QAction("As exclude group to file", this);
  m_save_group_file_exclude->setToolTip("Save current mask as exclude group to a file.");
  connect(m_save_group_file_exclude, SIGNAL(triggered()), this, SLOT(saveExcludeGroupToFile()));

  m_extract_to_workspace = new QAction("Extract detectors to workspace", this);
  m_extract_to_workspace->setToolTip("Extract detectors to workspace.");
  connect(m_extract_to_workspace, SIGNAL(triggered()), this, SLOT(extractDetsToWorkspace()));

  m_sum_to_workspace = new QAction("Sum detectors to workspace", this);
  m_sum_to_workspace->setToolTip("Sum detectors to workspace.");
  connect(m_sum_to_workspace, SIGNAL(triggered()), this, SLOT(sumDetsToWorkspace()));

  // Save button and its menus
  m_saveButton = new QPushButton("Apply and Save");
  m_saveButton->setToolTip("Save current masking/grouping to a file or a workspace.");

  m_saveMask = new QMenu(this);
  m_saveMask->addAction(m_save_as_workspace_exclude);
  m_saveMask->addAction(m_save_as_file_exclude);
  m_saveMask->addAction(m_save_as_cal_file_exclude);
  m_saveMask->addSeparator();
  m_saveMask->addAction(m_save_as_table_xrange_exclude);
  connect(m_saveMask, SIGNAL(hovered(QAction *)), this, SLOT(showSaveMenuTooltip(QAction *)));

  m_saveButton->setMenu(m_saveMask);

  m_saveGroup = new QMenu(this);
  m_saveGroup->addAction(m_extract_to_workspace);
  m_saveGroup->addAction(m_sum_to_workspace);
  m_saveGroup->addSeparator();

  connect(m_saveGroup, SIGNAL(hovered(QAction *)), this, SLOT(showSaveMenuTooltip(QAction *)));

  m_saveROI = new QMenu(this);
  m_saveROI->addAction(m_save_as_workspace_include);
  m_saveROI->addAction(m_save_as_file_include);
  m_saveROI->addAction(m_save_as_cal_file_include);
  m_saveROI->addSeparator();
  m_saveROI->addAction(m_extract_to_workspace);
  m_saveROI->addAction(m_sum_to_workspace);

  connect(m_saveROI, SIGNAL(hovered(QAction *)), this, SLOT(showSaveMenuTooltip(QAction *)));

  auto *box = new QGroupBox("View");
  auto *buttons = new QGridLayout();
  buttons->addWidget(m_applyToView, 0, 0, 1, 2);
  buttons->addWidget(m_saveShapesToTable, 1, 0, 1, 2);
  buttons->addWidget(m_saveButton, 2, 0);
  buttons->addWidget(m_clearAll, 2, 1);

  box->setLayout(buttons);
  layout->addWidget(box);

  box = new QGroupBox("Workspace");
  buttons = new QGridLayout();
  buttons->addWidget(m_applyToData, 0, 0);
  box->setLayout(buttons);
  layout->addWidget(box);

  connect(m_instrWidget, SIGNAL(maskedWorkspaceOverlayed()), this, SLOT(enableApplyButtons()));
}

InstrumentWidgetMaskTab::~InstrumentWidgetMaskTab() { m_browser->unsetFactoryForManager(m_doubleManager); }

/**
 * Initialize the tab when new projection surface is created.
 */
void InstrumentWidgetMaskTab::initSurface() {
  connect(m_instrWidget->getSurface().get(), SIGNAL(singleComponentPickedForMasking(size_t)), this,
          SLOT(singlePixelPicked(size_t)));
  connect(m_instrWidget->getSurface().get(), SIGNAL(shapeCreated()), this, SLOT(shapeCreated()));
  connect(m_instrWidget->getSurface().get(), SIGNAL(shapeSelected()), this, SLOT(shapeSelected()));
  connect(m_instrWidget->getSurface().get(), SIGNAL(shapesDeselected()), this, SLOT(shapesDeselected()));
  connect(m_instrWidget->getSurface().get(), SIGNAL(shapeChanged()), this, SLOT(shapeChanged()));
  connect(m_instrWidget->getSurface().get(), SIGNAL(shapesCleared()), this, SLOT(shapesCleared()));
  enableApplyButtons();
}

/**
 * Selects between masking/grouping
 * @param mode The required mode, @see Mode
 */
void InstrumentWidgetMaskTab::setMode(Mode mode) {
  switch (mode) {
  case Mask:
    m_masking_on->setChecked(true);
    break;
  case Group:
    m_grouping_on->setChecked(true);
    break;
  case ROI:
    m_roi_on->setChecked(true);
    break;
  default:
    throw std::invalid_argument("Invalid Mask tab mode. Use Mask/Group.");
  };
  toggleMaskGroup();
}

void InstrumentWidgetMaskTab::selectTool(Activity tool) {
  switch (tool) {
  case Move:
    m_move->setChecked(true);
    break;
  case Select:
    m_pointer->setChecked(true);
    break;
  case DrawEllipse:
    m_ellipse->setChecked(true);
    break;
  case DrawRectangle:
    m_rectangle->setChecked(true);
    break;
  case DrawEllipticalRing:
    m_ring_ellipse->setChecked(true);
    break;
  case DrawRectangularRing:
    m_ring_rectangle->setChecked(true);
    break;
  case DrawSector:
    m_sector->setChecked(true);
    break;
  case DrawFree:
    m_free_draw->setChecked(true);
    break;
  case Pixel:
    m_pixel->setChecked(true);
    break;
  case Tube:
    m_tube->setChecked(true);
    break;
  default:
    throw std::invalid_argument("Invalid tool type.");
  }
  setActivity();
}

/**
 * Set tab's activity based on the currently selected tool button.
 */
void InstrumentWidgetMaskTab::setActivity() {
  const QColor borderColor = getShapeBorderColor();
  const QColor fillColor = getShapeFillColor();
  QString whatIsBeingSelected = m_maskBins && getMode() == Mode::Mask ? "Selecting bins" : "Selecting detectors";
  if (m_move->isChecked()) {
    m_activity = Move;
    m_instrWidget->getSurface()->setInteractionMode(ProjectionSurface::MoveMode);
    m_activeTool->setText("Tool: Navigation");
  } else if (m_pointer->isChecked()) {
    m_activity = Select;
    m_instrWidget->getSurface()->setInteractionMode(ProjectionSurface::EditShapeMode);
    m_activeTool->setText("Tool: Shape editing. " + whatIsBeingSelected);
  } else if (m_ellipse->isChecked()) {
    m_activity = DrawEllipse;
    m_instrWidget->getSurface()->startCreatingShape2D("ellipse", borderColor, fillColor);
    m_instrWidget->getSurface()->setInteractionMode(ProjectionSurface::EditShapeMode);
    m_activeTool->setText("Tool: Ellipse. " + whatIsBeingSelected);
  } else if (m_rectangle->isChecked()) {
    m_activity = DrawRectangle;
    m_instrWidget->getSurface()->startCreatingShape2D("rectangle", borderColor, fillColor);
    m_instrWidget->getSurface()->setInteractionMode(ProjectionSurface::EditShapeMode);
    m_activeTool->setText("Tool: Rectangle. " + whatIsBeingSelected);
  } else if (m_ring_ellipse->isChecked()) {
    m_activity = DrawEllipticalRing;
    m_instrWidget->getSurface()->startCreatingShape2D("ring ellipse", borderColor, fillColor);
    m_instrWidget->getSurface()->setInteractionMode(ProjectionSurface::EditShapeMode);
    m_activeTool->setText("Tool: Elliptical ring. " + whatIsBeingSelected);
  } else if (m_ring_rectangle->isChecked()) {
    m_activity = DrawRectangularRing;
    m_instrWidget->getSurface()->startCreatingShape2D("ring rectangle", borderColor, fillColor);
    m_instrWidget->getSurface()->setInteractionMode(ProjectionSurface::EditShapeMode);
    m_activeTool->setText("Tool: Rectangular ring. " + whatIsBeingSelected);
  } else if (m_sector->isChecked()) {
    m_activity = DrawSector;
    m_instrWidget->getSurface()->startCreatingShape2D("sector", borderColor, fillColor);
    m_instrWidget->getSurface()->setInteractionMode(ProjectionSurface::EditShapeMode);
    m_activeTool->setText("Tool: Sector. " + whatIsBeingSelected);

  } else if (m_free_draw->isChecked()) {
    m_activity = DrawFree;
    m_instrWidget->getSurface()->startCreatingFreeShape(borderColor, fillColor);
    m_instrWidget->getSurface()->setInteractionMode(ProjectionSurface::DrawFreeMode);
    m_activeTool->setText("Tool: Free draw. " + whatIsBeingSelected);
  } else if (m_pixel->isChecked()) {
    m_activity = Pixel;
    m_instrWidget->getSurface()->setInteractionMode(ProjectionSurface::PickSingleMode);
    m_activeTool->setText("Tool: Pixel mask. " + whatIsBeingSelected);
  } else if (m_tube->isChecked()) {
    m_activity = Tube;
    m_instrWidget->getSurface()->setInteractionMode(ProjectionSurface::PickTubeMode);
    m_activeTool->setText("Tool: Tube/bank mask. " + whatIsBeingSelected);
  }
  m_instrWidget->updateInfoText();
}

/**
 * Slot responding on the pick of a single detector pixel for masking.
 * Can be used either to mask the picked pixel or its parent.
 */
void InstrumentWidgetMaskTab::singlePixelPicked(size_t pickID) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();
  if (!componentInfo.isDetector(pickID)) {
    return;
  }

  size_t parentIndex = componentInfo.hasParent(pickID) ? componentInfo.parent(pickID) : 0;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  std::vector<size_t> detectorsId{pickID};
  m_instrWidget->updateInstrumentView(); // to refresh the pick image

  if (m_masking_on->isChecked() || m_roi_on->isChecked()) {
    if (m_tube->isChecked()) {
      if (!componentInfo.hasParent(pickID)) {
        return;
      }
      detectorsId = componentInfo.detectorsInSubtree(parentIndex);
    }
    storeDetectorMask(m_roi_on->isChecked(), detectorsId);

  } else if (m_grouping_on->isChecked()) {
    if (m_pixel->isChecked()) {
      Mantid::detid_t detId = actor.getDetID(pickID);
      m_detectorsToGroup.clear();
      m_detectorsToGroup.push_back(detId);
    } else if (m_tube->isChecked()) {
      if (!componentInfo.hasParent(pickID)) {
        return;
      }
      const auto dets = actor.getDetIDs(componentInfo.detectorsInSubtree(parentIndex));
      m_detectorsToGroup.clear();
      std::copy(dets.cbegin(), dets.cend(), std::back_inserter(m_detectorsToGroup));
    }
  }

  // update detector colours
  m_instrWidget->getInstrumentActor().updateColors();
  m_instrWidget->updateInstrumentDetectors();
  QApplication::restoreOverrideCursor();
  enableApplyButtons();

  if (m_grouping_on->isChecked()) {
    if (m_pixel->isChecked()) {
      m_instrWidget->updateInfoText(QString("Pixel %0 picked for grouping").arg(pickID));

    } else if (m_tube->isChecked()) {
      QString message = QString("Component %0 picked for grouping")
                            .arg(QString::fromStdString(componentInfo.componentID(parentIndex)->getName()));
      m_instrWidget->updateInfoText(message);
    }
  }
}

/**
 * Slot responding on creation of a new masking shape.
 */
void InstrumentWidgetMaskTab::shapeCreated() {
  if (!isVisible())
    return;
  if (m_activity != DrawFree) {
    setSelectActivity();
  }
  enableApplyButtons();
}

/**
 * Slot responding on selection of a new masking shape.
 */
void InstrumentWidgetMaskTab::shapeSelected() { setProperties(); }

/**
 * Slot responding on deselecting all masking shapes.
 */
void InstrumentWidgetMaskTab::shapesDeselected() { clearProperties(); }

/**
 * Slot responding on a change of a masking shape.
 */
void InstrumentWidgetMaskTab::shapeChanged() {
  if (!m_left)
    return;              // check that everything is ok
  m_userEditing = false; // this prevents resetting shape properties by doubleChanged(...)
  RectF rect = m_instrWidget->getSurface()->getCurrentBoundingRect();

  m_doubleManager->setValue(m_left, std::min(rect.x0(), rect.x1()));
  m_doubleManager->setValue(m_top, std::max(rect.y0(), rect.y1()));
  m_doubleManager->setValue(m_right, std::max(rect.x0(), rect.x1()));
  m_doubleManager->setValue(m_bottom, std::min(rect.y0(), rect.y1()));

  for (QMap<QtProperty *, QString>::iterator it = m_doublePropertyMap.begin(); it != m_doublePropertyMap.end(); ++it) {
    m_doubleManager->setValue(it.key(), m_instrWidget->getSurface()->getCurrentDouble(it.value()));
  }
  for (QMap<QString, QtProperty *>::iterator it = m_pointPropertyMap.begin(); it != m_pointPropertyMap.end(); ++it) {
    QtProperty *prop = it.value();
    QList<QtProperty *> subs = prop->subProperties();
    if (subs.size() != 2)
      continue;
    QPointF p = m_instrWidget->getSurface()->getCurrentPoint(it.key());
    m_doubleManager->setValue(subs[0], p.x());
    m_doubleManager->setValue(subs[1], p.y());
  }
  m_userEditing = true;
}

/**
 * Slot responding on removing all masking shapes.
 */
void InstrumentWidgetMaskTab::shapesCleared() { enableApplyButtons(); }

/**
 * Removes the mask shapes from the screen.
 */
void InstrumentWidgetMaskTab::clearShapes() {
  m_instrWidget->getSurface()->clearMask();
  setSelectActivity();
}

void InstrumentWidgetMaskTab::showEvent(QShowEvent * /*unused*/) {
  setActivity();
  m_instrWidget->setMouseTracking(true);
  enableApplyButtons();
  m_instrWidget->updateInstrumentView(true);
  m_instrWidget->getSurface()->changeBorderColor(getShapeBorderColor());
}

void InstrumentWidgetMaskTab::clearProperties() {
  m_browser->clear();
  m_doublePropertyMap.clear();
  m_pointPropertyMap.clear();
  m_pointComponentsMap.clear();
  m_left = nullptr;
  m_top = nullptr;
  m_right = nullptr;
  m_bottom = nullptr;
  m_rotation = nullptr;
}

void InstrumentWidgetMaskTab::setProperties() {
  clearProperties();
  m_userEditing = false;

  // bounding rect property
  QtProperty *boundingRectGroup = m_groupManager->addProperty("Bounding Rect");
  m_browser->addProperty(boundingRectGroup);
  m_left = addDoubleProperty("left");
  m_top = addDoubleProperty("top");
  m_right = addDoubleProperty("right");
  m_bottom = addDoubleProperty("bottom");

  boundingRectGroup->addSubProperty(m_left);
  boundingRectGroup->addSubProperty(m_top);
  boundingRectGroup->addSubProperty(m_right);
  boundingRectGroup->addSubProperty(m_bottom);

  if (isRotationSupported()) {
    m_rotation = addDoubleProperty("rotation");
    boundingRectGroup->addSubProperty(m_rotation);
  }

  // point properties
  const QStringList pointProperties = m_instrWidget->getSurface()->getCurrentPointNames();
  for (const auto &name : pointProperties) {
    QtProperty *point = m_groupManager->addProperty(name);
    QtProperty *prop_x = addDoubleProperty("x");
    QtProperty *prop_y = addDoubleProperty("y");
    point->addSubProperty(prop_x);
    point->addSubProperty(prop_y);
    m_browser->addProperty(point);
    m_pointComponentsMap[prop_x] = name;
    m_pointComponentsMap[prop_y] = name;
    m_pointPropertyMap[name] = point;
  }

  // double properties
  const QStringList doubleProperties = m_instrWidget->getSurface()->getCurrentDoubleNames();
  for (const auto &name : doubleProperties) {
    QtProperty *prop = addDoubleProperty(name);
    m_browser->addProperty(prop);
    m_doublePropertyMap[prop] = name;
  }

  // rotation property
  if (isRotationSupported())
    m_doubleManager->setValue(m_rotation, m_instrWidget->getSurface()->getCurrentBoundingRotation());

  shapeChanged();
}

/**
 * Save shapes to a table workspace
 */
void InstrumentWidgetMaskTab::saveShapesToTable() const { m_instrWidget->getSurface()->saveShapesToTableWorkspace(); }

void InstrumentWidgetMaskTab::doubleChanged(QtProperty *prop) {
  if (!m_userEditing)
    return;

  if (prop == m_left || prop == m_top || prop == m_right || prop == m_bottom || prop == m_rotation) {
    m_userEditing = false;
    double x0 = std::min(m_doubleManager->value(m_left), m_doubleManager->value(m_right));
    double x1 = std::max(m_doubleManager->value(m_left), m_doubleManager->value(m_right));
    double y0 = std::min(m_doubleManager->value(m_top), m_doubleManager->value(m_bottom));
    double y1 = std::max(m_doubleManager->value(m_top), m_doubleManager->value(m_bottom));

    QRectF rect(QPointF(x0, y0), QPointF(x1, y1));
    m_instrWidget->getSurface()->setCurrentBoundingRect(RectF(rect));

    if (isRotationSupported())
      m_instrWidget->getSurface()->setCurrentBoundingRotation(m_doubleManager->value(m_rotation));

  } else {
    QString name = m_doublePropertyMap[prop];
    if (!name.isEmpty()) {
      m_instrWidget->getSurface()->setCurrentDouble(name, m_doubleManager->value(prop));
    } else {
      name = m_pointComponentsMap[prop];
      if (!name.isEmpty()) {
        QtProperty *point_prop = m_pointPropertyMap[name];
        QList<QtProperty *> subs = point_prop->subProperties();
        if (subs.size() != 2)
          return;
        QPointF p(m_doubleManager->value(subs[0]), m_doubleManager->value(subs[1]));
        m_instrWidget->getSurface()->setCurrentPoint(name, p);
      }
    }
  }
  // when the user validates the field's edit, the view is immediatly updated
  m_instrWidget->updateInstrumentView();
  m_instrWidget->update();
}

/**
 * Apply the constructed mask to the data workspace. This operation cannot be
 * reverted.
 */
void InstrumentWidgetMaskTab::applyMask() {
  storeMask();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_instrWidget->getInstrumentActor().applyMaskWorkspace();
  m_instrWidget->setupColorMap();
  enableApplyButtons();
  QApplication::restoreOverrideCursor();
}

/**
 * Apply the constructed mask to the view only.
 */
void InstrumentWidgetMaskTab::applyMaskToView() {
  storeMask();
  m_instrWidget->setupColorMap();
  enableApplyButtons();
}

/**
 * Remove all masking that has not been applied to the data workspace.
 */
void InstrumentWidgetMaskTab::clearMask() {
  clearShapes();
  m_detectorsToGroup.clear();
  m_instrWidget->getInstrumentActor().clearMasks();
  m_instrWidget->getInstrumentActor().updateColors();
  m_instrWidget->setupColorMap();
  m_instrWidget->updateInstrumentView();
  enableApplyButtons();
}

/**
 * Create a MaskWorkspace from the mask defined in this tab.
 * @param invertMask ::  if true, the selected mask will be inverted; if false,
 * the mask will be used as is
 * @param temp :: Set true to create a temporary workspace with a fixed name. If
 * false the name will be unique.
 */
Mantid::API::MatrixWorkspace_sptr InstrumentWidgetMaskTab::createMaskWorkspace(bool invertMask, bool temp) const {
  m_instrWidget->updateInstrumentView(); // to refresh the pick image
  Mantid::API::MatrixWorkspace_sptr inputWS = m_instrWidget->getInstrumentActor().getMaskMatrixWorkspace();
  Mantid::API::MatrixWorkspace_sptr outputWS;
  const std::string outputWorkspaceName = generateMaskWorkspaceName(temp);

  auto alg = AlgorithmManager::Instance().create("ExtractMask", -1);
  alg->setProperty("InputWorkspace", inputWS);
  alg->setPropertyValue("OutputWorkspace", outputWorkspaceName);
  alg->execute();

  outputWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(outputWorkspaceName));

  if (invertMask) {
    auto invertAlg = AlgorithmManager::Instance().create("BinaryOperateMasks", -1);
    invertAlg->setPropertyValue("InputWorkspace1", outputWorkspaceName);
    invertAlg->setPropertyValue("OutputWorkspace", outputWorkspaceName);
    invertAlg->setPropertyValue("OperationType", "NOT");
    invertAlg->execute();

    outputWS->setTitle("InvertedMaskWorkspace");
  } else {
    outputWS->setTitle("MaskWorkspace");
  }

  return outputWS;
}

void InstrumentWidgetMaskTab::saveInvertedMaskToWorkspace() { saveMaskingToWorkspace(true); }

void InstrumentWidgetMaskTab::saveMaskToWorkspace() { saveMaskingToWorkspace(false); }

void InstrumentWidgetMaskTab::saveInvertedMaskToFile() { saveMaskingToFile(true); }

void InstrumentWidgetMaskTab::saveMaskToFile() { saveMaskingToFile(false); }

void InstrumentWidgetMaskTab::saveMaskToCalFile() { saveMaskingToCalFile(false); }

void InstrumentWidgetMaskTab::saveInvertedMaskToCalFile() { saveMaskingToCalFile(true); }

void InstrumentWidgetMaskTab::saveMaskToTable() { saveMaskingToTableWorkspace(false); }

void InstrumentWidgetMaskTab::showMessageBox(const QString &message) {
  QMessageBox::information(this, "GroupDetectors Error", message, "OK");
}

/**
 * Extract selected detectors to a new workspace
 */
void InstrumentWidgetMaskTab::extractDetsToWorkspace() {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  std::vector<size_t> dets;
  m_instrWidget->getSurface()->getMaskedDetectors(dets);
  const auto &actor = m_instrWidget->getInstrumentActor();
  auto detectorIDs = actor.getDetIDs(dets);
  if (m_pixel->isChecked() || m_tube->isChecked())
    detectorIDs.insert(detectorIDs.end(), m_detectorsToGroup.cbegin(), m_detectorsToGroup.cend());
  DetXMLFile mapFile(detectorIDs);
  std::string fname = mapFile();
  if (!fname.empty()) {
    std::string workspaceName = m_instrWidget->getWorkspaceName().toStdString();
    auto alg = AlgorithmManager::Instance().create("GroupDetectors");
    try {
      alg->setPropertyValue("InputWorkspace", workspaceName);
      alg->setPropertyValue("MapFile", fname);
      alg->setPropertyValue("OutputWorkspace", workspaceName + "_selection");
      alg->execute();
    } catch (std::exception &e) {
      showMessageBox(e.what());
    }
  }
  QApplication::restoreOverrideCursor();
}

/**
 * Sum selected detectors to a new workspace
 */
void InstrumentWidgetMaskTab::sumDetsToWorkspace() {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  std::vector<size_t> dets;
  m_instrWidget->getSurface()->getMaskedDetectors(dets);
  auto detectorIDs = m_instrWidget->getInstrumentActor().getDetIDs(dets);
  if (m_pixel->isChecked() || m_tube->isChecked())
    detectorIDs.insert(detectorIDs.end(), m_detectorsToGroup.cbegin(), m_detectorsToGroup.cend());
  DetXMLFile mapFile(detectorIDs, DetXMLFile::Sum);
  std::string fname = mapFile();

  if (!fname.empty()) {
    std::string workspaceName = m_instrWidget->getWorkspaceName().toStdString();
    auto alg = AlgorithmManager::Instance().create("GroupDetectors");
    try {
      alg->setPropertyValue("InputWorkspace", workspaceName);
      alg->setPropertyValue("MapFile", fname);
      alg->setPropertyValue("OutputWorkspace", workspaceName + "_sum");
      alg->execute();
    } catch (std::exception &e) {
      showMessageBox(e.what());
    }
  }
  QApplication::restoreOverrideCursor();
}

void InstrumentWidgetMaskTab::saveIncludeGroupToFile() {
  QString fname = m_instrWidget->getSaveFileName("Save grouping file", "XML files (*.xml);;All (*)");
  if (!fname.isEmpty()) {
    std::vector<size_t> dets;
    m_instrWidget->getSurface()->getMaskedDetectors(dets);
    DetXMLFile mapFile(m_instrWidget->getInstrumentActor().getDetIDs(dets), DetXMLFile::Sum, fname);
  }
}

void InstrumentWidgetMaskTab::saveExcludeGroupToFile() {
  QString fname = m_instrWidget->getSaveFileName("Save grouping file", "XML files (*.xml);;All (*)");
  if (!fname.isEmpty()) {
    std::vector<size_t> dets;
    m_instrWidget->getSurface()->getMaskedDetectors(dets);
    const auto &actor = m_instrWidget->getInstrumentActor();
    DetXMLFile mapFile(actor.getAllDetIDs(), actor.getDetIDs(dets), fname);
  }
}

void InstrumentWidgetMaskTab::showSaveMenuTooltip(QAction *action) {
  QToolTip::showText(QCursor::pos(), action->toolTip(), this);
}

/**
 * Toggle between different modes
 *
 */
void InstrumentWidgetMaskTab::toggleMaskGroup() {
  Mode mode = getMode();

  enableApplyButtons();
  if (mode == Mode::Mask) {
    m_saveButton->setMenu(m_saveMask);
    m_saveButton->setText("Apply and Save");
  } else if (mode == Mode::ROI) {
    m_saveButton->setMenu(m_saveROI);
    m_saveButton->setText("Apply and Save");
  } else {
    m_saveButton->setMenu(m_saveGroup);
    m_saveButton->setText("Save");
  }
  m_instrWidget->getSurface()->changeBorderColor(getShapeBorderColor());
  m_instrWidget->updateInstrumentView();
}

/**
 * Save the constructed mask to a workspace with unique name of type
 * "MaskWorkspace_#".
 * The mask is not applied to the data workspace being displayed.
 * @param invertMask ::  if true, the selected mask will be inverted; if false,
 * the mask will be used as is
 */
void InstrumentWidgetMaskTab::saveMaskingToWorkspace(bool invertMask) {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  // Make sure we have stored the Mask in the helper MaskWorkspace
  storeDetectorMask(invertMask);
  setSelectActivity();
  createMaskWorkspace(false, false);

#if 0
			// TESTCASE
			double minbinvalue = m_instrWidget->getInstrumentActor()->minBinValue();
			double maxbinvalue = m_instrWidget->getInstrumentActor()->maxBinValue();
			std::cout << "Range of X: " << minbinvalue << ", " << maxbinvalue << ".\n";

#endif

  enableApplyButtons();
  QApplication::restoreOverrideCursor();
}

/**
 * Save the constructed mask to a file.
 * The mask is not applied to the data workspace being displayed.
 * @param invertMask ::  if true, the selected mask will be inverted; if false,
 * the mask will be used as is
 */
void InstrumentWidgetMaskTab::saveMaskingToFile(bool invertMask) {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // Make sure we have stored the Mask in the helper MaskWorkspace
  storeDetectorMask(invertMask);
  setSelectActivity();
  Mantid::API::MatrixWorkspace_sptr outputWS = createMaskWorkspace(false, true);
  if (outputWS) {
    clearShapes();

    QApplication::restoreOverrideCursor();
    QString fileName =
        m_instrWidget->getSaveFileName("Select location and name for the mask file", "XML files (*.xml);;All (*)");
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (!fileName.isEmpty()) {

      // Call "SaveMask()"
      Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveMask", -1);
      alg->setProperty("InputWorkspace", std::dynamic_pointer_cast<Mantid::API::Workspace>(outputWS));
      alg->setPropertyValue("OutputFile", fileName.toStdString());
      alg->execute();
    }
    Mantid::API::AnalysisDataService::Instance().remove(outputWS->getName());
  }
  enableApplyButtons();
  QApplication::restoreOverrideCursor();
}

/**
 * Save the constructed mask to a cal file.
 * The mask is not applied to the data workspace being displayed.
 * @param invertMask ::  if true, the selected mask will be inverted; if false,
 * the mask will be used as is
 */
void InstrumentWidgetMaskTab::saveMaskingToCalFile(bool invertMask) {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // Make sure we have stored the Mask in the helper MaskWorkspace
  storeDetectorMask(invertMask);

  setSelectActivity();
  Mantid::API::MatrixWorkspace_sptr outputWS = createMaskWorkspace(false, true);
  if (outputWS) {
    clearShapes();
    QString fileName =
        m_instrWidget->getSaveFileName("Select location and name for the mask file", "cal files (*.cal)");
    if (!fileName.isEmpty()) {
      Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("MaskWorkspaceToCalFile", -1);
      alg->setPropertyValue("InputWorkspace", outputWS->getName());
      alg->setPropertyValue("OutputFile", fileName.toStdString());
      alg->setProperty("Invert", false);
      alg->execute();
    }
    Mantid::API::AnalysisDataService::Instance().remove(outputWS->getName());
  }
  enableApplyButtons();
  QApplication::restoreOverrideCursor();
}

/**
 * Apply and save the mask to a TableWorkspace with X-range
 * @param invertMask :: if true, the selected mask will be inverted; if false,
 * the mask will be used as is
 */
void InstrumentWidgetMaskTab::saveMaskingToTableWorkspace(bool invertMask) {
  UNUSED_ARG(invertMask);

  // Set override cursor
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // Make sure that we have stored the mask in the helper Mask workspace
  storeDetectorMask();
  setSelectActivity();

  // Apply the view (no workspace) to a buffered mask workspace
  Mantid::API::MatrixWorkspace_sptr inputWS = m_instrWidget->getInstrumentActor().getMaskMatrixWorkspace();

  // Extract from MaskWorkspace to a TableWorkspace
  double xmin = m_instrWidget->getInstrumentActor().minBinValue();
  double xmax = m_instrWidget->getInstrumentActor().maxBinValue();
  // std::cout << "[DB] Selected x-range: " << xmin << ", " << xmax << ".\n";

  // Always use the same name
  const std::string outputWorkspaceName("MaskBinTable");

  // Check whether it is going to add a line in an existing workspace
  Mantid::API::ITableWorkspace_sptr temptablews;
  bool overwrite = false;
  try {
    temptablews = std::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(outputWorkspaceName));
  } catch (const Mantid::Kernel::Exception::NotFoundError &) {
    std::cout << "TableWorkspace " << outputWorkspaceName << " cannot be found in ADS."
              << ".\n";
  }

  if (temptablews)
    overwrite = true;

  std::cout << "[DB] MaskTableWorkspace is found? = " << overwrite << ". "
            << ".\n";

  auto alg = AlgorithmManager::Instance().create("ExtractMaskToTable", -1);
  alg->setProperty("InputWorkspace", inputWS);
  if (overwrite)
    alg->setPropertyValue("MaskTableWorkspace", outputWorkspaceName);
  alg->setPropertyValue("OutputWorkspace", outputWorkspaceName);
  alg->setProperty("Xmin", xmin);
  alg->setProperty("Xmax", xmax);
  alg->execute();

  // Restore the previous state
  enableApplyButtons();
  QApplication::restoreOverrideCursor();

  if (alg->isExecuted()) {
    // Mantid::API::MatrixWorkspace_sptr outputWS
    Mantid::API::ITableWorkspace_sptr outputWS = std::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(outputWorkspaceName));

    outputWS->setTitle("MaskBinTable");
  } else {
    QMessageBox::critical(this, QCoreApplication::applicationName() + " Error",
                          "Algorithm ExtractMaskToTable fails to execute. ");
  }
}

/**
 * Generate a unique name for the mask worspace which will be saved in the ADS.
 * It will have a form MaskWorkspace[_#]
 */
std::string InstrumentWidgetMaskTab::generateMaskWorkspaceName(bool temp) const {
  if (temp)
    return "__MaskTab_MaskWorkspace";
  auto wsNames = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  int maxIndex = 0;
  const std::string baseName = "MaskWorkspace";
  for (auto &wsName : wsNames) {
    if (wsName.find(baseName) == 0) {
      int index = Mantid::Kernel::Strings::endsWithInt(wsName);
      if (index > 0 && index > maxIndex)
        maxIndex = index;
      else
        maxIndex = 1;
    }
  }
  if (maxIndex > 0) {
    return baseName + "_" + Mantid::Kernel::Strings::toString(maxIndex + 1);
  }
  return baseName;
}

/**
 * Sets the m_hasMaskToApply flag and
 * enables/disables the apply and clear buttons.
 */
void InstrumentWidgetMaskTab::enableApplyButtons() {
  const auto &instrActor = m_instrWidget->getInstrumentActor();
  auto mode = getMode();

  m_maskBins = !instrActor.wholeRange();
  bool hasMaskShapes = m_instrWidget->getSurface()->hasMasks();
  bool hasMaskWorkspace = instrActor.hasMaskWorkspace();
  bool hasBinMask = instrActor.hasBinMask();
  bool hasDetectorMask = hasMaskShapes || hasMaskWorkspace;
  bool hasMask = hasDetectorMask || hasBinMask;
  bool canGroup = (m_detectorsToGroup.size() != 0) && (mode == Mode::Group);
  bool enableBinMasking = hasMaskShapes && m_maskBins && mode == Mode::Mask;

  if (m_maskBins && mode == Mode::Mask) {
    m_applyToView->setText("Apply bin mask to View");
  } else {
    m_applyToView->setText("Apply detector mask to View");
  }

  if ((mode == Mode::Mask) || (mode == Mode::ROI)) {
    m_hasMaskToApply = hasMask;
    m_applyToData->setEnabled(hasMask);
    m_applyToView->setEnabled(hasMaskShapes);
  } else {
    m_applyToData->setEnabled(false);
    m_applyToView->setEnabled(false);
  }
  m_saveShapesToTable->setEnabled(hasMaskShapes);
  m_saveButton->setEnabled((hasDetectorMask || canGroup) && (!enableBinMasking));
  m_clearAll->setEnabled(hasMask || canGroup);
  setActivity();
}

/**
 * Sets tab activity to Select: select and modify shapes.
 */
void InstrumentWidgetMaskTab::setSelectActivity() {
  m_pointer->setChecked(true);
  setActivity();
}

/**
 * It tab in masking, ROI or grouping mode?
 */
InstrumentWidgetMaskTab::Mode InstrumentWidgetMaskTab::getMode() const {
  if (m_masking_on->isChecked())
    return Mode::Mask;
  if (m_roi_on->isChecked())
    return Mode::ROI;
  if (m_grouping_on->isChecked())
    return Mode::Group;

  throw std::logic_error("Invalid mode");
}

/**
 * Border color.
 */
QColor InstrumentWidgetMaskTab::getShapeBorderColor() const {
  if (getMode() == Mode::Mask)
    return Qt::red;
  if (getMode() == Mode::ROI)
    return Qt::yellow;
  return Qt::blue;
}

/**
 * Shape fill color.
 */
QColor InstrumentWidgetMaskTab::getShapeFillColor() const { return QColor(255, 255, 255, 100); }

QtProperty *InstrumentWidgetMaskTab::addDoubleProperty(const QString &name) const {
  QtProperty *prop = m_doubleManager->addProperty(name);
  m_doubleManager->setDecimals(prop, 6);
  return prop;
}

/**
 * Store the mask defined by the shape tools to the helper m_maskWorkspace.
 */
void InstrumentWidgetMaskTab::storeDetectorMask(bool isROI, const std::vector<size_t> &onClickDets) {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_instrWidget->updateInstrumentView(); // to refresh the pick image
  Mantid::API::IMaskWorkspace_sptr wsFresh;

  const auto &actor = m_instrWidget->getInstrumentActor();
  std::vector<size_t> dets;
  if (onClickDets.size() == 0) {
    // get detectors covered by the shapes
    m_instrWidget->getSurface()->getMaskedDetectors(dets);
    m_pointer->setChecked(true);
    setActivity();
  }
  dets.insert(dets.end(), onClickDets.begin(), onClickDets.end());
  if (!dets.empty()) {
    auto wsMask = actor.getMaskWorkspace();
    // have to cast up to the MaskWorkspace to get access to clone()

    std::set<Mantid::detid_t> detList;
    if (isROI) {
      // need to invert the mask before adding the new shape
      // but not if the mask is fresh and empty
      if (wsMask->getNumberMasked() > 0) {
        wsFresh = std::dynamic_pointer_cast<Mantid::API::IMaskWorkspace>(actor.extractCurrentMask());
        actor.invertMaskWorkspace();
      }
    }
    for (auto det : dets)
      detList.insert(actor.getDetID(det));

    if (!detList.empty()) {
      // try to mask each detector separately and ignore any failure
      for (auto det : detList) {
        try {
          if (isROI && wsFresh) {
            if (wsMask->isMasked(det))
              wsFresh->setMasked(det);
          } else {
            wsMask->setMasked(det);
          }
        } catch (...) {
        }
      }
      if (isROI) {
        if (wsFresh)
          m_instrWidget->getInstrumentActor().setMaskMatrixWorkspace(
              std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsFresh));
        // need to invert the mask before displaying
        m_instrWidget->getInstrumentActor().invertMaskWorkspace();
      }
      // update detector colours
      m_instrWidget->getInstrumentActor().updateColors();
      m_instrWidget->updateInstrumentDetectors();
    }
  }
  if (onClickDets.size() == 0) {
    // remove masking shapes
    clearShapes();
  }
  QApplication::restoreOverrideCursor();
}

void InstrumentWidgetMaskTab::storeBinMask() {
  std::vector<size_t> dets;
  // get detectors covered by the shapes
  m_instrWidget->getSurface()->getMaskedDetectors(dets);
  // mask some bins
  m_instrWidget->getInstrumentActor().addMaskBinsData(dets);
  // remove masking shapes
  clearShapes();
  enableApplyButtons();
}

/// Store current shapes as a mask (detector or bin)
void InstrumentWidgetMaskTab::storeMask() {
  if (m_maskBins && getMode() == Mode::Mask) {
    storeBinMask();
  } else {
    storeDetectorMask(getMode() == Mode::ROI);
  }
}

void InstrumentWidgetMaskTab::changedIntegrationRange(double /*unused*/, double /*unused*/) {
  if (m_instrWidget->isCurrentTab(this)) {
    enableApplyButtons();
  }
}

/** Load mask tab state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 */
void InstrumentWidgetMaskTab::loadFromProject(const std::string &lines) {
  Q_UNUSED(lines);
  throw std::runtime_error("InstrumentWidgetMaskTab::loadFromProject() not implemented for Qt >= 5");
}

/** Load a mask workspace applied to the instrument actor from the project
 *
 * This is for the case where masks have been applied to the instrument actor
 * but not to the workspace itself or saved to a workspace/table
 *
 * @param name :: name of the file to load from the project folder
 */
void InstrumentWidgetMaskTab::loadMaskViewFromProject(const std::string &name) {
  using namespace Mantid::API;
  using namespace Mantid::Kernel;

  QSettings settings;
  auto workingDir = settings.value("Project/WorkingDirectory", "").toString();
  auto fileName = workingDir.toStdString() + "/" + name;
  auto maskWS = loadMask(fileName);

  if (!maskWS)
    return; // if we couldn't load it then just fail silently

  auto &actor = m_instrWidget->getInstrumentActor();
  actor.setMaskMatrixWorkspace(maskWS);
  actor.updateColors();
  m_instrWidget->updateInstrumentDetectors();
  m_instrWidget->updateInstrumentView();
}

/** Load a mask workspace given a file name
 *
 * This will attempt to load a mask workspace using the supplied file name and
 * assume that the instrument is the same one as the actor in the instrument
 * view.
 *
 * @param fileName :: the full path to the mask file on disk
 * @return a pointer to the loaded mask workspace
 */
Mantid::API::MatrixWorkspace_sptr InstrumentWidgetMaskTab::loadMask(const std::string &fileName) {
  using namespace Mantid::API;

  // build path and input properties etc.
  const auto &actor = m_instrWidget->getInstrumentActor();
  auto workspace = actor.getWorkspace();
  auto instrument = workspace->getInstrument();
  auto instrumentName = instrument->getName();
  auto tempName = "__" + workspace->getName() + "MaskView";

  // load the mask from the project folder
  try {
    auto alg = AlgorithmManager::Instance().create("LoadMask", -1);
    alg->initialize();
    alg->setPropertyValue("Instrument", instrumentName);
    alg->setPropertyValue("InputFile", fileName);
    alg->setPropertyValue("OutputWorkspace", tempName);
    alg->execute();
  } catch (...) {
    // just fail silently, if we can't load the mask then we should
    // give up at this point.
    return nullptr;
  }

  // get the mask workspace and remove from ADS to clean up
  auto &ads = AnalysisDataService::Instance();
  auto maskWS = ads.retrieveWS<MatrixWorkspace>(tempName);
  ads.remove(tempName);

  return maskWS;
}

/** Save the state of the mask tab to a Mantid project file
 * @return a string representing the state of the mask tab
 */
std::string InstrumentWidgetMaskTab::saveToProject() const {
  throw std::runtime_error("InstrumentWidgetMaskTab::saveToProject() not implemented for Qt >= 5");
}

/** Save a mask workspace containing masks applied to the instrument view
 *
 * This will save masks which have been applied to the instrument view actor
 * but have not be applied to the workspace or exported to a seperate
 * workspace/table already
 *
 * @param name :: the name to call the workspace in the project folder
 * @return whether a workspace was successfully saved to the project
 */
bool InstrumentWidgetMaskTab::saveMaskViewToProject(const std::string &name, const std::string &projectPath) const {
  using namespace Mantid::API;
  using namespace Mantid::Kernel;

  try {
    QString workingDir;
    QSettings settings;
    if (projectPath == "") {
      workingDir = settings.value("Project/WorkingDirectory", "").toString();
    } else {
      workingDir = QString::fromStdString(projectPath);
    }
    Poco::Path filepath(workingDir.toStdString());
    auto fileName = filepath.append(name).toString();

    // get masked detector workspace from actor
    const auto &actor = m_instrWidget->getInstrumentActor();
    if (actor.hasMaskWorkspace()) {
      auto outputWS = actor.getMaskMatrixWorkspace();
      // save mask to file inside project folder
      auto alg = AlgorithmManager::Instance().createUnmanaged("SaveMask", -1);
      alg->setChild(true);
      alg->setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(outputWS));
      alg->setPropertyValue("OutputFile", fileName);
      alg->setLogging(false);
      alg->execute();
    } else {
      return false;
    }
  } catch (...) {
    // just fail silently, if we can't save the mask then we should
    // give up at this point.
    return false;
  }

  return true;
}

bool InstrumentWidgetMaskTab::isRotationSupported() {
  const auto shapeType = m_instrWidget->getSurface()->getCurrentShapeType();
  return shapeType == "rectangle" || shapeType == "ellipse";
}

} // namespace MantidQt::MantidWidgets
