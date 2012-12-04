#include "InstrumentWindow.h"
#include "InstrumentWindowMaskTab.h"
#include "InstrumentActor.h"
#include "ProjectionSurface.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMaskWorkspace.h"
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
#include <QTextEdit>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QMessageBox>
#include <QApplication>
#include <QFileDialog>

#include <numeric>
#include <cfloat>
#include <algorithm>

InstrumentWindowMaskTab::InstrumentWindowMaskTab(InstrumentWindow* instrWindow):
InstrumentWindowTab(instrWindow),
m_instrumentWindow(instrWindow),
m_activity(Select),
m_hasMaskToApply(false),
m_userEditing(true)
{

  // main layout
  QVBoxLayout* layout=new QVBoxLayout(this);

  // Create the tool buttons

  m_move = new QPushButton();
  m_move->setCheckable(true);
  m_move->setAutoExclusive(true);
  m_move->setIcon(QIcon(":/PickTools/selection-tube.png"));
  m_move->setToolTip("Move the instrument");

  m_pointer = new QPushButton();
  m_pointer->setCheckable(true);
  m_pointer->setAutoExclusive(true);
  m_pointer->setIcon(QIcon(":/MaskTools/selection-pointer.png"));
  m_pointer->setToolTip("Select and edit shapes");

  m_ellipse = new QPushButton();
  m_ellipse->setCheckable(true);
  m_ellipse->setAutoExclusive(true);
  m_ellipse->setIcon(QIcon(":/MaskTools/selection-circle.png"));
  m_ellipse->setToolTip("Draw an ellipse");

  m_rectangle = new QPushButton();
  m_rectangle->setCheckable(true);
  m_rectangle->setAutoExclusive(true);
  m_rectangle->setIcon(QIcon(":/MaskTools/selection-box.png"));
  m_rectangle->setToolTip("Draw a rectangle");

  m_ring_ellipse = new QPushButton();
  m_ring_ellipse->setCheckable(true);
  m_ring_ellipse->setAutoExclusive(true);
  m_ring_ellipse->setIcon(QIcon(":/MaskTools/selection-circle.png"));
  m_ring_ellipse->setToolTip("Draw an elliptical ring");

  m_ring_rectangle = new QPushButton();
  m_ring_rectangle->setCheckable(true);
  m_ring_rectangle->setAutoExclusive(true);
  m_ring_rectangle->setIcon(QIcon(":/MaskTools/selection-box.png"));
  m_ring_rectangle->setToolTip("Draw a rectangular ring ");

  QHBoxLayout* toolBox = new QHBoxLayout();
  toolBox->addWidget(m_move);
  toolBox->addWidget(m_pointer);
  toolBox->addWidget(m_ellipse);
  toolBox->addWidget(m_rectangle);
  toolBox->addWidget(m_ring_ellipse);
  toolBox->addWidget(m_ring_rectangle);
  toolBox->addStretch();
  toolBox->setSpacing(2);

  connect(m_move,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_pointer,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_ellipse,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_rectangle,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_ring_ellipse,SIGNAL(clicked()),this,SLOT(setActivity()));
  connect(m_ring_rectangle,SIGNAL(clicked()),this,SLOT(setActivity()));
  m_move->setChecked(true);

  layout->addLayout(toolBox);

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

  m_apply = new QPushButton("Apply");
  connect(m_apply,SIGNAL(clicked()),this,SLOT(applyMask()));

  m_clear_all = new QPushButton("Clear All");
  connect(m_clear_all,SIGNAL(clicked()),this,SLOT(clearMask()));

  m_save_as_workspace_exclude = new QAction("As Mask to workspace",this);
  connect(m_save_as_workspace_exclude,SIGNAL(activated()),this,SLOT(saveMaskToWorkspace()));

  m_save_as_workspace_include = new QAction("As ROI to workspace",this);
  connect(m_save_as_workspace_include,SIGNAL(activated()),this,SLOT(saveInvertedMaskToWorkspace()));

  m_save_as_file_exclude = new QAction("As Mask to file",this);
  connect(m_save_as_file_exclude,SIGNAL(activated()),this,SLOT(saveMaskToFile()));

  m_save_as_file_include = new QAction("As ROI to file",this);
  connect(m_save_as_file_include,SIGNAL(activated()),this,SLOT(saveInvertedMaskToFile()));

  m_saveButton = new QPushButton("Save");
  QMenu* saveMenu = new QMenu(this);
  saveMenu->addAction(m_save_as_workspace_include);
  saveMenu->addAction(m_save_as_workspace_exclude);
  saveMenu->addAction(m_save_as_file_include);
  saveMenu->addAction(m_save_as_file_exclude);
  m_saveButton->setMenu(saveMenu);

  QGridLayout* buttons = new QGridLayout();
  buttons->addWidget(m_apply,0,0);
  buttons->addWidget(m_clear_all,0,1);
  buttons->addWidget(m_saveButton,1,0,1,2);
  
  layout->addLayout(buttons);

}

void InstrumentWindowMaskTab::initOnShow()
{
  connect(m_instrumentWindow->getSurface(),SIGNAL(shapeCreated()),this,SLOT(shapeCreated()));
  connect(m_instrumentWindow->getSurface(),SIGNAL(shapeSelected()),this,SLOT(shapeSelected()));
  connect(m_instrumentWindow->getSurface(),SIGNAL(shapesDeselected()),this,SLOT(shapesDeselected()));
  connect(m_instrumentWindow->getSurface(),SIGNAL(shapeChanged()),this,SLOT(shapeChanged()));
  enableApply( m_instrumentWindow->getSurface()->hasMasks() );
}

void InstrumentWindowMaskTab::setActivity()
{
  const QColor borderColor = Qt::red;
  const QColor fillColor = QColor(255,255,255,100);
  if (m_move->isChecked())
  {
    m_activity = Move;
    m_instrumentWindow->getSurface()->setInteractionMode(ProjectionSurface::MoveMode);
  }
  else if (m_pointer->isChecked())
  {
    m_activity = Select;
    m_instrumentWindow->getSurface()->setInteractionMode(ProjectionSurface::DrawMode);
  }
  else if (m_ellipse->isChecked())
  {
    m_activity = DrawEllipse;
    m_instrumentWindow->getSurface()->startCreatingShape2D("ellipse",borderColor,fillColor);
    m_instrumentWindow->getSurface()->setInteractionMode(ProjectionSurface::DrawMode);
  }
  else if (m_rectangle->isChecked())
  {
    m_activity = DrawEllipse;
    m_instrumentWindow->getSurface()->startCreatingShape2D("rectangle",borderColor,fillColor);
    m_instrumentWindow->getSurface()->setInteractionMode(ProjectionSurface::DrawMode);
  }
  else if (m_ring_ellipse->isChecked())
  {
    m_activity = DrawEllipse;
    m_instrumentWindow->getSurface()->startCreatingShape2D("ring ellipse",borderColor,fillColor);
    m_instrumentWindow->getSurface()->setInteractionMode(ProjectionSurface::DrawMode);
  }
  else if (m_ring_rectangle->isChecked())
  {
    m_activity = DrawEllipse;
    m_instrumentWindow->getSurface()->startCreatingShape2D("ring rectangle",borderColor,fillColor);
    m_instrumentWindow->getSurface()->setInteractionMode(ProjectionSurface::DrawMode);
  }
}

void InstrumentWindowMaskTab::shapeCreated()
{
  setSelectActivity();
  enableApply(true);
}

void InstrumentWindowMaskTab::shapeSelected()
{
  setProperties();
}

void InstrumentWindowMaskTab::shapesDeselected()
{
  clearProperties();
}

void InstrumentWindowMaskTab::shapeChanged()
{
  if (!m_left) return; // check that everything is ok
  m_userEditing = false; // this prevents resetting shape proeprties by doubleChanged(...)
  QRectF rect = m_instrumentWindow->getSurface()->getCurrentBoundingRect();
  m_doubleManager->setValue(m_left,rect.left());
  m_doubleManager->setValue(m_top,rect.top());
  m_doubleManager->setValue(m_right,rect.right());
  m_doubleManager->setValue(m_bottom,rect.bottom());
  for(QMap<QtProperty *,QString>::iterator it = m_doublePropertyMap.begin(); it != m_doublePropertyMap.end(); ++it)
  {
    m_doubleManager->setValue(it.key(),m_instrumentWindow->getSurface()->getCurrentDouble(it.value()));
  }
  for(QMap<QString,QtProperty *>::iterator it = m_pointPropertyMap.begin(); it != m_pointPropertyMap.end(); ++it)
  {
    QtProperty* prop = it.value();
    QList<QtProperty*> subs = prop->subProperties();
    if (subs.size() != 2) continue;
    QPointF p = m_instrumentWindow->getSurface()->getCurrentPoint(it.key());
    m_doubleManager->setValue(subs[0],p.x());
    m_doubleManager->setValue(subs[1],p.y());
  }
  m_userEditing = true;
}

/**
  * Removes the mask shapes from the screen.
  */
void InstrumentWindowMaskTab::clearShapes()
{
    m_instrumentWindow->getSurface()->clearMask();
}

void InstrumentWindowMaskTab::showEvent (QShowEvent *)
{
  setActivity();
  m_instrumentWindow->setMouseTracking(true);
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
  m_left = m_doubleManager->addProperty("left");
  m_top = m_doubleManager->addProperty("top");
  m_right = m_doubleManager->addProperty("right");
  m_bottom = m_doubleManager->addProperty("bottom");
  boundingRectGroup->addSubProperty(m_left);
  boundingRectGroup->addSubProperty(m_top);
  boundingRectGroup->addSubProperty(m_right);
  boundingRectGroup->addSubProperty(m_bottom);

  // point properties
  QStringList pointProperties = m_instrumentWindow->getSurface()->getCurrentPointNames();
  foreach(QString name,pointProperties)
  {
    QtProperty* point = m_groupManager->addProperty(name);
    QtProperty* prop_x = m_doubleManager->addProperty("x");
    QtProperty* prop_y = m_doubleManager->addProperty("y");
    point->addSubProperty(prop_x);
    point->addSubProperty(prop_y);
    m_browser->addProperty(point);
    m_pointComponentsMap[prop_x] = name;
    m_pointComponentsMap[prop_y] = name;
    m_pointPropertyMap[name] = point;
  }

  // double properties
  QStringList doubleProperties = m_instrumentWindow->getSurface()->getCurrentDoubleNames();
  foreach(QString name,doubleProperties)
  {
    QtProperty* prop = m_doubleManager->addProperty(name);
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
    m_instrumentWindow->getSurface()->setCurrentBoundingRect(rect);
  }
  else
  {
    QString name = m_doublePropertyMap[prop];
    if (!name.isEmpty())
    {
      m_instrumentWindow->getSurface()->setCurrentDouble(name,m_doubleManager->value(prop));
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
        m_instrumentWindow->getSurface()->setCurrentPoint(name,p);
      }
    }
  }
  m_instrumentWindow->update();
}

/**
  * Apply the constructed mask to the data workspace. This operation cannot be reverted.
  */
void InstrumentWindowMaskTab::applyMask()
{
  storeMask();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_instrumentWindow->getInstrumentActor()->applyMaskWorkspace();
  enableApply(false);
  QApplication::restoreOverrideCursor();
}

/**
  * Remove all masking that has not been applied to the data workspace.
  */
void InstrumentWindowMaskTab::clearMask()
{
  clearShapes();
  m_instrumentWindow->getInstrumentActor()->clearMaskWorkspace();
  m_instrumentWindow->updateInstrumentView();
  enableApply(false);
}

/**
 * Create a MaskWorkspace from the mask defined in this tab.
 * @param invertMask ::  if true, the selected mask will be inverted; if false, the mask will be used as is
 * @param temp :: Set true to create a temporary workspace with a fixed name. If false the name will be unique.
 */
Mantid::API::MatrixWorkspace_sptr InstrumentWindowMaskTab::createMaskWorkspace(bool invertMask, bool temp)
{
  m_instrumentWindow->updateInstrumentView(); // to refresh the pick image
  Mantid::API::MatrixWorkspace_sptr inputWS = m_instrumentWindow->getInstrumentActor()->getMaskMatrixWorkspace();
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
    QString saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));
    QString fileName = QFileDialog::getSaveFileName(m_instrumentWindow,"Select location for the mas file",saveDir);

    if (!fileName.isEmpty())
    {
      Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveMask",-1);
      alg->setProperty("InputWorkspace",boost::dynamic_pointer_cast<Mantid::API::Workspace>(outputWS));
      alg->setPropertyValue("OutputFile",fileName.toStdString());
      alg->setProperty("GroupedDetectors",true);
      alg->execute();
    }
    Mantid::API::AnalysisDataService::Instance().remove( outputWS->name() );
  }
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
  * enables/disables the Apply and ClearAll buttons.
  */
void InstrumentWindowMaskTab::enableApply(bool on)
{
    m_hasMaskToApply = on;
    m_apply->setEnabled(on);
    m_clear_all->setEnabled(on);
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
 * Store the mask defined by the shape tools to the helper m_maskWorkspace.
 */
void InstrumentWindowMaskTab::storeMask()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_pointer->setChecked(true);
    setActivity();
    m_instrumentWindow->updateInstrumentView(); // to refresh the pick image

    QList<int> dets;
    m_instrumentWindow->getSurface()->getMaskedDetectors(dets);
    if (!dets.isEmpty())
    {
      std::set<Mantid::detid_t> detList;
      foreach(int id,dets)
      {
        detList.insert( id );
      }
      if ( !detList.empty() )
      {
        m_instrumentWindow->getInstrumentActor()->getMaskWorkspace()->setMasked( detList );
        m_instrumentWindow->getInstrumentActor()->update();
        m_instrumentWindow->updateInstrumentDetectors();
      }
    }
    clearShapes();
    QApplication::restoreOverrideCursor();
}
