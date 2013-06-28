#include "InstrumentWindow.h"
#include "InstrumentWindowMaskTab.h"
#include "InstrumentActor.h"
#include "ProjectionSurface.h"
#include "DetXMLFile.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
// #include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QTextEdit>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QMessageBox>
#include <QApplication>
#include <QFileDialog>
#include <QToolTip>
#include <QTemporaryFile>
#include <QGroupBox>
#include <QCheckBox>

#include "MantidQtAPI/FileDialogHandler.h"

#include <numeric>
#include <cfloat>
#include <algorithm>
#include <fstream>

InstrumentWindowMaskTab::InstrumentWindowMaskTab(InstrumentWindow* instrWindow):
InstrumentWindowTab(instrWindow),
m_activity(Select),
m_hasMaskToApply(false),
m_userEditing(true)
{

  // main layout
  QVBoxLayout* layout=new QVBoxLayout(this);

  m_masking_on = new QRadioButton("Mask");
  m_grouping_on = new QRadioButton("Group");
  m_masking_on->setChecked(true);
  connect(m_masking_on,SIGNAL(toggled(bool)),this,SLOT(toggleMaskGroup(bool)));
  QHBoxLayout* radioLayout = new QHBoxLayout();
  radioLayout->addWidget(m_masking_on);
  radioLayout->addWidget(m_grouping_on);
  radioLayout->setMargin(0);
  QWidget* radioGroup = new QWidget();
  radioGroup->setLayout(radioLayout);

  layout->addWidget(radioGroup);

  // Create the tool buttons

  m_move = new QPushButton();
  m_move->setCheckable(true);
  m_move->setAutoExclusive(true);
  m_move->setIcon(QIcon(":/PickTools/selection-tube.png"));
  m_move->setToolTip("Move the instrument (Ctrl+Alt+M)");
  m_move->setShortcut(QKeySequence("Ctrl+Alt+M"));

  m_pointer = new QPushButton();
  m_pointer->setCheckable(true);
  m_pointer->setAutoExclusive(true);
  m_pointer->setIcon(QIcon(":/MaskTools/selection-pointer.png"));
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

  QHBoxLayout* toolBox = new QHBoxLayout();
  toolBox->addWidget(m_move);
  toolBox->addWidget(m_pointer);
  toolBox->addWidget(m_ellipse);
  toolBox->addWidget(m_rectangle);
  toolBox->addWidget(m_ring_ellipse);
  toolBox->addWidget(m_ring_rectangle);
  toolBox->addStretch();
  toolBox->setSpacing(2);
  toolBox->setMargin(0);

  connect(m_move,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_pointer,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_ellipse,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_rectangle,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_ring_ellipse,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_ring_rectangle,SIGNAL(clicked()),this,SLOT(setActivity()));
  m_move->setChecked(true);
  QFrame* toolGroup = new QFrame();
  toolGroup->setLayout(toolBox);

  layout->addWidget(toolGroup);

  // Create property browser

  /* Create property managers: they create, own properties, get and set values  */

  m_groupManager = new QtGroupPropertyManager(this);
  m_doubleManager = new QtDoublePropertyManager(this);
  connect(m_doubleManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(doubleChanged(QtProperty*)));

  /* Create editors and assign them to the managers */

  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory(this);

  m_browser = new QtTreePropertyBrowser();
  m_browser->setFactoryForManager(m_doubleManager, doubleEditorFactory);
  
  layout->addWidget(m_browser);

  // Algorithm buttons

  m_apply = new QPushButton("Apply to Data");
  m_apply->setToolTip("Apply current mask to the data workspace. Cannot be reverted.");
  connect(m_apply,SIGNAL(clicked()),this,SLOT(applyMask()));

  m_apply_to_view = new QPushButton("Apply to View");
  m_apply_to_view->setToolTip("Apply current mask to the view.");
  connect(m_apply_to_view,SIGNAL(clicked()),this,SLOT(applyMaskToView()));

  m_clear_all = new QPushButton("Clear All");
  m_clear_all->setToolTip("Clear all masking that have not been applied to the data.");
  connect(m_clear_all,SIGNAL(clicked()),this,SLOT(clearMask()));


  m_save_as_workspace_exclude = new QAction("As Mask to workspace",this);
  m_save_as_workspace_exclude->setToolTip("Save current mask to mask workspace.");
  connect(m_save_as_workspace_exclude,SIGNAL(activated()),this,SLOT(saveMaskToWorkspace()));

  m_save_as_workspace_include = new QAction("As ROI to workspace",this);
  m_save_as_workspace_include->setToolTip("Save current mask as ROI to mask workspace.");
  connect(m_save_as_workspace_include,SIGNAL(activated()),this,SLOT(saveInvertedMaskToWorkspace()));

  m_save_as_file_exclude = new QAction("As Mask to file",this);
  m_save_as_file_exclude->setToolTip("Save current mask to mask file.");
  connect(m_save_as_file_exclude,SIGNAL(activated()),this,SLOT(saveMaskToFile()));

  m_save_as_file_include = new QAction("As ROI to file",this);
  m_save_as_file_include->setToolTip("Save current mask as ROI to mask file.");
  connect(m_save_as_file_include,SIGNAL(activated()),this,SLOT(saveInvertedMaskToFile()));

  m_save_as_cal_file_exclude = new QAction("As Mask to cal file",this);
  m_save_as_cal_file_exclude->setToolTip("Save current mask to cal file.");
  connect(m_save_as_cal_file_exclude,SIGNAL(activated()),this,SLOT(saveMaskToCalFile()));

  m_save_as_cal_file_include = new QAction("As ROI to cal file",this);
  m_save_as_cal_file_include->setToolTip("Save current mask as ROI to cal file.");
  connect(m_save_as_cal_file_include,SIGNAL(activated()),this,SLOT(saveInvertedMaskToCalFile()));

  m_save_as_table_xrange_exclude = new QAction("As Mask to table", this);
  m_save_as_table_xrange_exclude->setToolTip("Save current mask to a table workspace with x-range. "
                                             "The name of output table workspace is 'MaskBinTable'. "
                                             "If the output table workspace has alrady exist, then "
                                             "the newly masked detectors will be added to output workspace.");
  connect(m_save_as_table_xrange_exclude, SIGNAL(activated()), this, SLOT(saveMaskToTable()));

  m_save_group_file_include = new QAction("As include group to file",this);
  m_save_group_file_include->setToolTip("Save current mask as include group to a file.");
  connect(m_save_group_file_include,SIGNAL(activated()),this,SLOT(saveIncludeGroupToFile()));

  m_save_group_file_exclude = new QAction("As exclude group to file",this);
  m_save_group_file_exclude->setToolTip("Save current mask as exclude group to a file.");
  connect(m_save_group_file_exclude,SIGNAL(activated()),this,SLOT(saveExcludeGroupToFile()));

  m_extract_to_workspace = new QAction("Extract detectors to workspace",this);
  m_extract_to_workspace->setToolTip("Extract detectors to workspace.");
  connect(m_extract_to_workspace,SIGNAL(activated()),this,SLOT(extractDetsToWorkspace()));

  m_sum_to_workspace = new QAction("Sum detectors to workspace",this);
  m_sum_to_workspace->setToolTip("Sum detectors to workspace.");
  connect(m_sum_to_workspace,SIGNAL(activated()),this,SLOT(sumDetsToWorkspace()));


  // Save button and its menus
  m_saveButton = new QPushButton("Apply and Save");
  m_saveButton->setToolTip("Save current masking/grouping to a file or a workspace.");

  m_saveMask = new QMenu(this);
  m_saveMask->addAction(m_save_as_workspace_include);
  m_saveMask->addAction(m_save_as_workspace_exclude);
  m_saveMask->addSeparator();
  m_saveMask->addAction(m_save_as_file_include);
  m_saveMask->addAction(m_save_as_file_exclude);
  m_saveMask->addSeparator();
  m_saveMask->addAction(m_save_as_cal_file_include);
  m_saveMask->addAction(m_save_as_cal_file_exclude);
  m_saveMask->addSeparator();
  m_saveMask->addAction(m_save_as_table_xrange_exclude);
  connect(m_saveMask,SIGNAL(hovered(QAction*)),this,SLOT(showSaveMenuTooltip(QAction*)));

  m_saveButton->setMenu(m_saveMask);

  m_saveGroup = new QMenu(this);
  m_saveGroup->addAction(m_extract_to_workspace);
  m_saveGroup->addAction(m_sum_to_workspace);
  m_saveGroup->addSeparator();
  m_saveGroup->addAction(m_save_group_file_include);
  m_saveGroup->addAction(m_save_group_file_exclude);
  connect(m_saveGroup,SIGNAL(hovered(QAction*)),this,SLOT(showSaveMenuTooltip(QAction*)));

  QGroupBox *box = new QGroupBox("View");
  QGridLayout* buttons = new QGridLayout();
  buttons->addWidget(m_apply_to_view,0,0,1,2);
  buttons->addWidget(m_saveButton,1,0);
  buttons->addWidget(m_clear_all,1,1);

  box->setLayout(buttons);
  layout->addWidget(box);

  box = new QGroupBox("Workspace");
  buttons = new QGridLayout();
  buttons->addWidget(m_apply,0,0);
  box->setLayout(buttons);
  layout->addWidget(box);

}

/**
  * Initialize the tab when new projection surface is created.
  */
void InstrumentWindowMaskTab::initSurface()
{
  connect(m_instrWindow->getSurface().get(),SIGNAL(shapeCreated()),this,SLOT(shapeCreated()));
  connect(m_instrWindow->getSurface().get(),SIGNAL(shapeSelected()),this,SLOT(shapeSelected()));
  connect(m_instrWindow->getSurface().get(),SIGNAL(shapesDeselected()),this,SLOT(shapesDeselected()));
  connect(m_instrWindow->getSurface().get(),SIGNAL(shapeChanged()),this,SLOT(shapeChanged()));
  connect(m_instrWindow->getSurface().get(),SIGNAL(shapesCleared()),this,SLOT(shapesCleared()));
  enableApplyButtons();
}

/**
 * Selects between masking/grouping
 * @param mode The required mode, @see Mode
 */
void InstrumentWindowMaskTab::setMode(Mode mode)
{
  switch(mode)
  {
  case Mask: toggleMaskGroup(true);
    break;
  case Group: toggleMaskGroup(false);
    break;
  default: throw std::invalid_argument("Invalid Mask tab mode. Use Mask/Group.");
  };


}

void InstrumentWindowMaskTab::selectTool(Activity tool)
{
  switch(tool)
  {
  case Move: m_move->setChecked(true);
    break;
  case Select: m_pointer->setChecked(true);
    break;
  case DrawEllipse: m_ellipse->setChecked(true);
    break;
  case DrawRectangle: m_rectangle->setChecked(true);
    break;
  case DrawEllipticalRing: m_ring_ellipse->setChecked(true);
    break;
  case DrawRectangularRing: m_ring_rectangle->setChecked(true);
    break;
  default: throw std::invalid_argument("Invalid tool type.");
  }
  setActivity();
}


/**
  * Set tab's activity based on the currently selected tool button.
  */
void InstrumentWindowMaskTab::setActivity()
{
  const QColor borderColor = getShapeBorderColor();
  const QColor fillColor = getShapeFillColor();
  if (m_move->isChecked())
  {
    m_activity = Move;
    m_instrWindow->getSurface()->setInteractionMode(ProjectionSurface::MoveMode);
  }
  else if (m_pointer->isChecked())
  {
    m_activity = Select;
    m_instrWindow->getSurface()->setInteractionMode(ProjectionSurface::DrawMode);
  }
  else if (m_ellipse->isChecked())
  {
    m_activity = DrawEllipse;
    m_instrWindow->getSurface()->startCreatingShape2D("ellipse",borderColor,fillColor);
    m_instrWindow->getSurface()->setInteractionMode(ProjectionSurface::DrawMode);
  }
  else if (m_rectangle->isChecked())
  {
    m_activity = DrawRectangle;
    m_instrWindow->getSurface()->startCreatingShape2D("rectangle",borderColor,fillColor);
    m_instrWindow->getSurface()->setInteractionMode(ProjectionSurface::DrawMode);
  }
  else if (m_ring_ellipse->isChecked())
  {
    m_activity = DrawEllipticalRing;
    m_instrWindow->getSurface()->startCreatingShape2D("ring ellipse",borderColor,fillColor);
    m_instrWindow->getSurface()->setInteractionMode(ProjectionSurface::DrawMode);
  }
  else if (m_ring_rectangle->isChecked())
  {
    m_activity = DrawRectangularRing;
    m_instrWindow->getSurface()->startCreatingShape2D("ring rectangle",borderColor,fillColor);
    m_instrWindow->getSurface()->setInteractionMode(ProjectionSurface::DrawMode);
  }
  m_instrWindow->updateInfoText();
}

/**
  * Slot responding on creation of a new masking shape.
  */
void InstrumentWindowMaskTab::shapeCreated()
{
  setSelectActivity();
  enableApplyButtons();
}

/**
  * Slot responding on selection of a new masking shape.
  */
void InstrumentWindowMaskTab::shapeSelected()
{
  setProperties();
}

/**
  * Slot responding on deselecting all masking shapes.
  */
void InstrumentWindowMaskTab::shapesDeselected()
{
  clearProperties();
}

/**
  * Slot responding on a change of a masking shape.
  */
void InstrumentWindowMaskTab::shapeChanged()
{
  if (!m_left) return; // check that everything is ok
  m_userEditing = false; // this prevents resetting shape proeprties by doubleChanged(...)
  RectF rect = m_instrWindow->getSurface()->getCurrentBoundingRect();
  m_doubleManager->setValue(m_left,rect.x0());
  m_doubleManager->setValue(m_top,rect.y1());
  m_doubleManager->setValue(m_right,rect.x1());
  m_doubleManager->setValue(m_bottom,rect.y0());
  for(QMap<QtProperty *,QString>::iterator it = m_doublePropertyMap.begin(); it != m_doublePropertyMap.end(); ++it)
  {
    m_doubleManager->setValue(it.key(),m_instrWindow->getSurface()->getCurrentDouble(it.value()));
  }
  for(QMap<QString,QtProperty *>::iterator it = m_pointPropertyMap.begin(); it != m_pointPropertyMap.end(); ++it)
  {
    QtProperty* prop = it.value();
    QList<QtProperty*> subs = prop->subProperties();
    if (subs.size() != 2) continue;
    QPointF p = m_instrWindow->getSurface()->getCurrentPoint(it.key());
    m_doubleManager->setValue(subs[0],p.x());
    m_doubleManager->setValue(subs[1],p.y());
  }
  m_userEditing = true;
}

/**
  * Slot responding on removing all masking shapes.
  */
void InstrumentWindowMaskTab::shapesCleared()
{
    enableApplyButtons();
}

/**
  * Removes the mask shapes from the screen.
  */
void InstrumentWindowMaskTab::clearShapes()
{
    m_instrWindow->getSurface()->clearMask();
}

void InstrumentWindowMaskTab::showEvent (QShowEvent *)
{
  setActivity();
  m_instrWindow->setMouseTracking(true);
  enableApplyButtons();
  m_instrWindow->updateInstrumentView(true);
}

void InstrumentWindowMaskTab::clearProperties()
{
  m_browser->clear();
  m_doublePropertyMap.clear();
  m_pointPropertyMap.clear();
  m_pointComponentsMap.clear();
  m_left = NULL;
  m_top = NULL;
  m_right = NULL;
  m_bottom = NULL;
}

void InstrumentWindowMaskTab::setProperties()
{
  clearProperties();
  m_userEditing = false;

  // bounding rect property
  QtProperty* boundingRectGroup = m_groupManager->addProperty("Bounging Rect");
  m_browser->addProperty(boundingRectGroup);
  m_left   = addDoubleProperty("left");
  m_top    = addDoubleProperty("top");
  m_right  = addDoubleProperty("right");
  m_bottom = addDoubleProperty("bottom");
  boundingRectGroup->addSubProperty(m_left);
  boundingRectGroup->addSubProperty(m_top);
  boundingRectGroup->addSubProperty(m_right);
  boundingRectGroup->addSubProperty(m_bottom);

  // point properties
  QStringList pointProperties = m_instrWindow->getSurface()->getCurrentPointNames();
  foreach(QString name,pointProperties)
  {
    QtProperty* point = m_groupManager->addProperty(name);
    QtProperty* prop_x = addDoubleProperty("x");
    QtProperty* prop_y = addDoubleProperty("y");
    point->addSubProperty(prop_x);
    point->addSubProperty(prop_y);
    m_browser->addProperty(point);
    m_pointComponentsMap[prop_x] = name;
    m_pointComponentsMap[prop_y] = name;
    m_pointPropertyMap[name] = point;
  }

  // double properties
  QStringList doubleProperties = m_instrWindow->getSurface()->getCurrentDoubleNames();
  foreach(QString name,doubleProperties)
  {
    QtProperty* prop = addDoubleProperty(name);
    m_browser->addProperty(prop);
    m_doublePropertyMap[prop] = name;
  }

  shapeChanged();
}

void InstrumentWindowMaskTab::doubleChanged(QtProperty* prop)
{
  if (!m_userEditing) return;
  if (prop == m_left || prop == m_top || prop == m_right || prop == m_bottom)
  {
    QRectF rect(QPointF(m_doubleManager->value(m_left),m_doubleManager->value(m_top)),
                QPointF(m_doubleManager->value(m_right),m_doubleManager->value(m_bottom)));
    m_instrWindow->getSurface()->setCurrentBoundingRect(rect);
  }
  else
  {
    QString name = m_doublePropertyMap[prop];
    if (!name.isEmpty())
    {
      m_instrWindow->getSurface()->setCurrentDouble(name,m_doubleManager->value(prop));
    }
    else
    {
      name = m_pointComponentsMap[prop];
      if (!name.isEmpty())
      {
        QtProperty* point_prop = m_pointPropertyMap[name];
        QList<QtProperty*> subs = point_prop->subProperties();
        if (subs.size() != 2) return;
        QPointF p(m_doubleManager->value(subs[0]),m_doubleManager->value(subs[1]));
        m_instrWindow->getSurface()->setCurrentPoint(name,p);
      }
    }
  }
  m_instrWindow->update();
}

/**
  * Apply the constructed mask to the data workspace. This operation cannot be reverted.
  */
void InstrumentWindowMaskTab::applyMask()
{
  storeMask();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_instrWindow->getInstrumentActor()->applyMaskWorkspace();
  enableApplyButtons();
  QApplication::restoreOverrideCursor();
}

/**
  * Apply the constructed mask to the view only.
  */
void InstrumentWindowMaskTab::applyMaskToView()
{
    storeMask();
    enableApplyButtons();
}

/**
  * Remove all masking that has not been applied to the data workspace.
  */
void InstrumentWindowMaskTab::clearMask()
{
  clearShapes();
  m_instrWindow->getInstrumentActor()->clearMaskWorkspace();
  m_instrWindow->updateInstrumentView();
  enableApplyButtons();
}

/**
 * Create a MaskWorkspace from the mask defined in this tab.
 * @param invertMask ::  if true, the selected mask will be inverted; if false, the mask will be used as is
 * @param temp :: Set true to create a temporary workspace with a fixed name. If false the name will be unique.
 */
Mantid::API::MatrixWorkspace_sptr InstrumentWindowMaskTab::createMaskWorkspace(bool invertMask, bool temp)
{
  m_instrWindow->updateInstrumentView(); // to refresh the pick image
  Mantid::API::MatrixWorkspace_sptr inputWS = m_instrWindow->getInstrumentActor()->getMaskMatrixWorkspace();
  Mantid::API::MatrixWorkspace_sptr outputWS;
  const std::string outputWorkspaceName = generateMaskWorkspaceName(temp);

  Mantid::API::IAlgorithm * alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("ExtractMask",-1);
  alg->setProperty("InputWorkspace", inputWS);
  alg->setPropertyValue("OutputWorkspace",outputWorkspaceName);
  alg->execute();

  outputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve( outputWorkspaceName ));

  if (invertMask)
  {
      Mantid::API::IAlgorithm * invertAlg = Mantid::API::FrameworkManager::Instance().createAlgorithm("BinaryOperateMasks",-1);
      invertAlg->setPropertyValue("InputWorkspace1", outputWorkspaceName);
      invertAlg->setPropertyValue("OutputWorkspace", outputWorkspaceName);
      invertAlg->setPropertyValue("OperationType", "NOT");
      invertAlg->execute();

      outputWS->setTitle("InvertedMaskWorkspace");
  }
  else
  {
    outputWS->setTitle("MaskWorkspace");
  }

  return outputWS;
}

void InstrumentWindowMaskTab::saveInvertedMaskToWorkspace()
{
  saveMaskingToWorkspace(true);
}

void InstrumentWindowMaskTab::saveMaskToWorkspace()
{
  saveMaskingToWorkspace(false);
}

void InstrumentWindowMaskTab::saveInvertedMaskToFile()
{
  saveMaskingToFile(true);
}

void InstrumentWindowMaskTab::saveMaskToFile()
{
    saveMaskingToFile(false);
}

void InstrumentWindowMaskTab::saveMaskToCalFile()
{
    saveMaskingToCalFile(false);
}

void InstrumentWindowMaskTab::saveInvertedMaskToCalFile()
{
    saveMaskingToCalFile(true);
}

void InstrumentWindowMaskTab::saveMaskToTable()
{
  saveMaskingToTableWorkspace(false);
}

/**
  * Extract selected detectors to a new workspace
  */
void InstrumentWindowMaskTab::extractDetsToWorkspace()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QList<int> dets;
    m_instrWindow->getSurface()->getMaskedDetectors(dets);
    DetXMLFile mapFile(dets);
    std::string fname = mapFile();
    if (!fname.empty())
    {
      std::string workspaceName = m_instrWindow->getWorkspaceName().toStdString();
      Mantid::API::IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("GroupDetectors");
      alg->setPropertyValue("InputWorkspace",workspaceName);
      alg->setPropertyValue("MapFile",fname);
      alg->setPropertyValue("OutputWorkspace",workspaceName + "_selection");
      alg->execute();
    }
    QApplication::restoreOverrideCursor();
}

/**
  * Sum selected detectors to a new workspace
  */
void InstrumentWindowMaskTab::sumDetsToWorkspace()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QList<int> dets;
    m_instrWindow->getSurface()->getMaskedDetectors(dets);
    DetXMLFile mapFile(dets,DetXMLFile::Sum);
    std::string fname = mapFile();

    if (!fname.empty())
    {
      std::string workspaceName = m_instrWindow->getWorkspaceName().toStdString();
      Mantid::API::IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("GroupDetectors");
      alg->setPropertyValue("InputWorkspace",workspaceName);
      alg->setPropertyValue("MapFile",fname);
      alg->setPropertyValue("OutputWorkspace",workspaceName+"_sum");
      alg->execute();
    }
    QApplication::restoreOverrideCursor();
}

void InstrumentWindowMaskTab::saveIncludeGroupToFile()
{
    QString fname = m_instrWindow->getSaveFileName("Save grouping file", "XML files (*.xml);;All (*.* *)");
    if (!fname.isEmpty())
    {
        QList<int> dets;
        m_instrWindow->getSurface()->getMaskedDetectors(dets);
        DetXMLFile mapFile(dets,DetXMLFile::Sum,fname);
    }
}

void InstrumentWindowMaskTab::saveExcludeGroupToFile()
{
    QString fname = m_instrWindow->getSaveFileName("Save grouping file", "XML files (*.xml);;All (*.* *)");
    if (!fname.isEmpty())
    {
        QList<int> dets;
        m_instrWindow->getSurface()->getMaskedDetectors(dets);
        DetXMLFile mapFile(m_instrWindow->getInstrumentActor()->getAllDetIDs(),dets,fname);
    }
}

void InstrumentWindowMaskTab::showSaveMenuTooltip(QAction *action)
{
    QToolTip::showText(QCursor::pos(),action->toolTip(),this);
}

/**
  * Toggle between masking and grouping.
  *
  * @param maskOn :: True if masking functionality to be set. False is for grouping.
  */
void InstrumentWindowMaskTab::toggleMaskGroup(bool maskOn)
{
    m_masking_on->blockSignals(true);
    m_masking_on->setChecked(maskOn);
    m_grouping_on->setChecked(!maskOn);
    m_masking_on->blockSignals(false);

    enableApplyButtons();
    if ( maskOn )
    {
        m_saveButton->setMenu(m_saveMask);
        m_saveButton->setText("Apply and Save");
    }
    else
    {
        m_saveButton->setMenu(m_saveGroup);
        m_saveButton->setText("Save");
    }
    m_instrWindow->getSurface()->changeBorderColor(getShapeBorderColor());
    m_instrWindow->updateInstrumentView();
}

/**
  * Save the constructed mask to a workspace with unique name of type "MaskWorkspace_#".
  * The mask is not applied to the data workspace being displayed.
  * @param invertMask ::  if true, the selected mask will be inverted; if false, the mask will be used as is
  */
void InstrumentWindowMaskTab::saveMaskingToWorkspace(bool invertMask)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  // Make sure we have stored the Mask in the helper MaskWorkspace
  storeMask();
  setSelectActivity();
  createMaskWorkspace(invertMask, false);

#if 0
  // TESTCASE
  double minbinvalue = m_instrWindow->getInstrumentActor()->minBinValue();
  double maxbinvalue = m_instrWindow->getInstrumentActor()->maxBinValue();
  std::cout << "Range of X: " << minbinvalue << ", " << maxbinvalue << ".\n";

#endif

  enableApplyButtons();
  QApplication::restoreOverrideCursor();
}

/**
  * Save the constructed mask to a file.
  * The mask is not applied to the data workspace being displayed.
  * @param invertMask ::  if true, the selected mask will be inverted; if false, the mask will be used as is
  */
void InstrumentWindowMaskTab::saveMaskingToFile(bool invertMask)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // Make sure we have stored the Mask in the helper MaskWorkspace
  storeMask();

  setSelectActivity();
  Mantid::API::MatrixWorkspace_sptr outputWS = createMaskWorkspace(invertMask,true);
  if (outputWS)
  {
    clearShapes();
    QString fileName = m_instrWindow->getSaveFileName("Select location and name for the mask file", "XML files (*.xml);;All (*.* *)");

    if (!fileName.isEmpty())
    {

      // Call "SaveMask()"
      Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveMask",-1);
      alg->setProperty("InputWorkspace",boost::dynamic_pointer_cast<Mantid::API::Workspace>(outputWS));
      alg->setPropertyValue("OutputFile",fileName.toStdString());
      alg->execute();
    }
    Mantid::API::AnalysisDataService::Instance().remove( outputWS->name() );
  }
  enableApplyButtons();
  QApplication::restoreOverrideCursor();
}

/**
  * Save the constructed mask to a cal file.
  * The mask is not applied to the data workspace being displayed.
  * @param invertMask ::  if true, the selected mask will be inverted; if false, the mask will be used as is
  */
void InstrumentWindowMaskTab::saveMaskingToCalFile(bool invertMask)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Make sure we have stored the Mask in the helper MaskWorkspace
    storeMask();

    setSelectActivity();
    Mantid::API::MatrixWorkspace_sptr outputWS = createMaskWorkspace(false,true);
    if (outputWS)
    {
      clearShapes();
      QString fileName = m_instrWindow->getSaveFileName("Select location and name for the mask file", "cal files (*.cal)");
      if (!fileName.isEmpty())
      {
        Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("MaskWorkspaceToCalFile",-1);
        alg->setPropertyValue("InputWorkspace",outputWS->name());
        alg->setPropertyValue("OutputFile",fileName.toStdString());
        alg->setProperty("Invert",invertMask);
        alg->execute();
      }
      Mantid::API::AnalysisDataService::Instance().remove( outputWS->name() );
    }
    enableApplyButtons();
    QApplication::restoreOverrideCursor();
}

/**
  * Apply and save the mask to a TableWorkspace with X-range
  * @param invertMask :: if true, the selected mask will be inverted; if false, the mask will be used as is
  */
void InstrumentWindowMaskTab::saveMaskingToTableWorkspace(bool invertMask)
{
  UNUSED_ARG(invertMask);

  // Set override cursor
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // Make sure that we have stored the mask in the helper Mask workspace
  storeMask();
  setSelectActivity();

  // Apply the view (no workspace) to a buffered mask workspace
  Mantid::API::MatrixWorkspace_sptr inputWS = m_instrWindow->getInstrumentActor()->getMaskMatrixWorkspace();

  /*
  Mantid::Geometry::Instrument_const_sptr instrument = outputMaskWS->getInstrument();
  std::vector<int> detids = instrument->getDetectorIDs();
  size_t maskedcount = 0;
  for (size_t i = 0; i < detids.size(); ++i)
  {
    int detid = detids[i];
    Mantid::Geometry::IDetector_const_sptr det = instrument->getDetector(detid);
    if (det->isMasked())
      ++ maskedcount;
  }
  std::cout << "There are " << maskedcount << " detectors out of total " << detids.size() << " detectors.\n";
  */

  // Extract from MaskWorkspace to a TableWorkspace
  double xmin = m_instrWindow->getInstrumentActor()->minBinValue();
  double xmax = m_instrWindow->getInstrumentActor()->maxBinValue();
  // std::cout << "[DB] Selected x-range: " << xmin << ", " << xmax << ".\n";

  // Always use the same name
  const std::string outputWorkspaceName("MaskBinTable");

  // Check whether it is going to add a line in an existing workspace
  Mantid::API::ITableWorkspace_sptr temptablews;
  bool overwrite = false;
  try
  {
    temptablews = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(outputWorkspaceName));
  }
  catch (Mantid::Kernel::Exception::NotFoundError)
  {
    std::cout << "TableWorkspace " << outputWorkspaceName << " cannot be found in ADS." << ".\n";
  }

  if (temptablews)
    overwrite = true;

  std::cout << "[DB] MaskTableWorkspace is found? = " << overwrite << ". " << ".\n";

  Mantid::API::IAlgorithm * alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("ExtractMaskToTable",-1);
  alg->setProperty("InputWorkspace", inputWS);
  if (overwrite)
    alg->setPropertyValue("MaskTableWorkspace", outputWorkspaceName);
  alg->setPropertyValue("OutputWorkspace",outputWorkspaceName);
  alg->setProperty("Xmin", xmin);
  alg->setProperty("Xmax", xmax);
  alg->execute();

  if (!alg->isExecuted())
  {
    throw std::runtime_error("Algorithm ExtractMaskToTable fails to execute. ");
  }

    // Mantid::API::MatrixWorkspace_sptr outputWS
  Mantid::API::ITableWorkspace_sptr outputWS =
      boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve( outputWorkspaceName ));

  outputWS->setTitle("MaskBinTable");

  // Restore the previous state
  enableApplyButtons();
  QApplication::restoreOverrideCursor();
}


/**
  * Generate a unique name for the mask worspace which will be saved in the ADS.
  * It will have a form MaskWorkspace[_#]
  */
std::string InstrumentWindowMaskTab::generateMaskWorkspaceName(bool temp) const
{
    if ( temp ) return "__MaskTab_MaskWorkspace";
    std::set<std::string> wsNames = Mantid::API::AnalysisDataService::Instance().getObjectNames();
    int maxIndex = 0;
    const std::string baseName = "MaskWorkspace";
    for(auto name = wsNames.begin(); name != wsNames.end(); ++name)
    {
        if ( name->find(baseName) == 0 )
        {
            int index = Mantid::Kernel::Strings::endsWithInt(*name);
            if ( index > 0 && index > maxIndex ) maxIndex = index;
            else
                maxIndex = 1;
        }
    }
    if ( maxIndex > 0 )
    {
        return baseName + "_" + Mantid::Kernel::Strings::toString(maxIndex + 1);
    }
    return baseName;
}

/**
  * Sets the m_hasMaskToApply flag and
  * enables/disables the apply and clear buttons.
  */
void InstrumentWindowMaskTab::enableApplyButtons()
{
    bool hasMaskShapes = m_instrWindow->getSurface()->hasMasks();
    bool hasMaskWorkspace = m_instrWindow->getInstrumentActor()->hasMaskWorkspace();
    bool hasMask = hasMaskShapes || hasMaskWorkspace;
    if ( isMasking() )
    {
        m_hasMaskToApply = hasMask;
        m_apply->setEnabled(hasMask);
        m_apply_to_view->setEnabled(hasMaskShapes);
    }
    else
    {
        m_apply->setEnabled(false);
        m_apply_to_view->setEnabled(false);
    }
    m_saveButton->setEnabled(hasMask);
    m_clear_all->setEnabled(hasMask);
}

/**
  * Sets tab activity to Select: select and modify shapes.
  */
void InstrumentWindowMaskTab::setSelectActivity()
{
    m_pointer->setChecked(true);
    setActivity();
}

/**
  * It tab in masking or grouping mode?
  */
bool InstrumentWindowMaskTab::isMasking() const
{
    return m_masking_on->isChecked();
}

/**
  * Border color.
  */
QColor InstrumentWindowMaskTab::getShapeBorderColor() const
{
    if ( isMasking() ) return Qt::red;
    return Qt::blue;
}

/**
  * Shape fill color.
  */
QColor InstrumentWindowMaskTab::getShapeFillColor() const
{
    return QColor(255,255,255,100);
}

QtProperty *InstrumentWindowMaskTab::addDoubleProperty(const QString &name) const
{
    QtProperty* prop = m_doubleManager->addProperty( name );
    m_doubleManager->setDecimals(prop, 6);
    return prop;
}

/**
 * Store the mask defined by the shape tools to the helper m_maskWorkspace.
 */
void InstrumentWindowMaskTab::storeMask()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_pointer->setChecked(true);
  setActivity();
  m_instrWindow->updateInstrumentView(); // to refresh the pick image

  QList<int> dets;
  // get detectors covered by the shapes
  m_instrWindow->getSurface()->getMaskedDetectors(dets);
  if (!dets.isEmpty())
  {
    std::set<Mantid::detid_t> detList;
    foreach(int id,dets)
    {
      detList.insert( id );
    }
    if ( !detList.empty() )
    {
      // try to mask each detector separatly and ignore any failure
      for(auto det = detList.begin(); det != detList.end(); ++det)
      {
        try
        {
          m_instrWindow->getInstrumentActor()->getMaskWorkspace()->setMasked( *det );
        }
        catch(...){}
      }
      // update detector colours
      m_instrWindow->getInstrumentActor()->update();
      m_instrWindow->updateInstrumentDetectors();
    }
  }
  // remove masking shapes
  clearShapes();
  QApplication::restoreOverrideCursor();
}
