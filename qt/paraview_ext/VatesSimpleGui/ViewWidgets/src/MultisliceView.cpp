#include "MantidVatesSimpleGuiViewWidgets/MultisliceView.h"
#include "MantidGeometry/MDGeometry/MDPlaneImplicitFunction.h"
#include "MantidQtWidgets/Factory/WidgetFactory.h"
#include "MantidQtWidgets/SliceViewer/SliceViewerWindow.h"
#include "MantidVatesAPI/VatesKnowledgeSerializer.h"
#include "MantidVatesSimpleGuiQtWidgets/GeometryParser.h"
#include "MantidVatesSimpleGuiViewWidgets/RebinnedSourcesManager.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqModelTransformSupportBehavior.h>
#include <pqMultiSliceView.h>
#include <pqObjectBuilder.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <pqServerManagerModel.h>
#include <vtkContextMouseEvent.h>
#include <vtkDataObject.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPVArrayInformation.h>
#include <vtkPVChangeOfBasisHelper.h>
#include <vtkPVDataInformation.h>
#include <vtkSMMultiSliceViewProxy.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMSourceProxy.h>
#include <vtkVector.h>

#include <QString>

using namespace Mantid::Geometry;
using namespace MantidQt::SliceViewer;

namespace Mantid {
namespace Vates {
namespace SimpleGui {

MultiSliceView::MultiSliceView(QWidget *parent,
                               RebinnedSourcesManager *rebinnedSourcesManager,
                               bool createRenderProxy)
    : ViewBase(parent, rebinnedSourcesManager),
      m_contextMenu(new QMenu(tr("Context menu"), this)),
      m_edit(new QLineEdit(this)) {
  this->m_ui.setupUi(this);

  this->setContextMenuPolicy(Qt::CustomContextMenu);
  m_edit->setPlaceholderText(QString("Slice Position"));
  auto action = new QWidgetAction(this);
  action->setDefaultWidget(m_edit);
  m_contextMenu->addAction(action);

  QObject::connect(this->m_edit, SIGNAL(textChanged(const QString &)), this,
                   SLOT(checkState(const QString &)));
  QObject::connect(this->m_edit, SIGNAL(editingFinished()), this,
                   SLOT(setSlicePosition()));

  if (createRenderProxy) {
    pqRenderView *tmp =
        this->createRenderView(this->m_ui.renderFrame, QString("MultiSlice"));
    this->setupData();
    this->m_mainView = qobject_cast<pqMultiSliceView *>(tmp);
    QObject::connect(this->m_mainView,
                     SIGNAL(sliceClicked(int, double, int, int)), this,
                     SLOT(checkSliceClicked(int, double, int, int)));
  }
}

MultiSliceView::~MultiSliceView() {}

void MultiSliceView::destroyView() {
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->destroy(this->m_mainView);
}

pqRenderView *MultiSliceView::getView() {
  return qobject_cast<pqRenderView *>(this->m_mainView.data());
}

void MultiSliceView::setupData() {
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

  // Make sure that origsrc exists
  if (this->origSrc) {
    pqDataRepresentation *drep = builder->createDataRepresentation(
        this->origSrc->getOutputPort(0), this->m_mainView,
        "CompositeAlignedGeometrySliceRepresentation");
    vtkSMPropertyHelper(drep->getProxy(), "Representation").Set("Slices");
    if (!this->isPeaksWorkspace(this->origSrc)) {
      vtkSMPVRepresentationProxy::SetScalarColoring(
          drep->getProxy(), "signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);
      vtkSMMultiSliceViewProxy *viewPxy =
          vtkSMMultiSliceViewProxy::SafeDownCast(this->m_mainView->getProxy());
      viewPxy->CreateDefaultRepresentation(this->origSrc->getProxy(), 0);
    }
    drep->getProxy()->UpdateVTKObjects();
  }
}

void MultiSliceView::render() {
  this->origSrc = pqActiveObjects::instance().activeSource();
  if (this->origSrc == nullptr) {
    return;
  }
  this->checkSliceViewCompat();
  this->setupData();
  this->resetDisplay();
}

void MultiSliceView::renderAll() { this->m_mainView->render(); }

void MultiSliceView::resetDisplay() { this->m_mainView->resetDisplay(); }

void MultiSliceView::setView(pqRenderView *view) {
  clearRenderLayout(this->m_ui.renderFrame);
  this->m_mainView = qobject_cast<pqMultiSliceView *>(view);
  QHBoxLayout *hbox = new QHBoxLayout(this->m_ui.renderFrame);
  hbox->setMargin(0);
  hbox->addWidget(m_mainView->widget());
  QObject::connect(this->m_mainView,
                   SIGNAL(sliceClicked(int, double, int, int)), this,
                   SLOT(checkSliceClicked(int, double, int, int)));
}

ModeControlWidget::Views MultiSliceView::getViewType() {
  return ModeControlWidget::Views::MULTISLICE;
}

void MultiSliceView::resetCamera() { this->m_mainView->resetCamera(); }

/**
 * This function checks the signal coming from the MultiSliceView when a slice
 * indicator is clicked. It then calls for the slice to be shown in the
 * SliceViewer.
 * @param axisIndex : index for the axis on which the clicked indicator resides
 * @param sliceOffsetOnAxis : location of slice along axis
 * @param button : which mouse button is being used
 * @param modifier : which modifier key is being used
 */
void MultiSliceView::checkSliceClicked(int axisIndex, double sliceOffsetOnAxis,
                                       int button, int modifier) {
  if (button == vtkContextMouseEvent::LEFT_BUTTON ||
      button == vtkContextMouseEvent::RIGHT_BUTTON) {
    if (modifier == vtkContextMouseEvent::SHIFT_MODIFIER) {
      this->showCutInSliceViewer(axisIndex, sliceOffsetOnAxis);
    } else if (modifier == vtkContextMouseEvent::ALT_MODIFIER) {
      this->editSlicePosition(axisIndex, sliceOffsetOnAxis);
    }
  }
}

void MultiSliceView::editSlicePosition(int axisIndex,
                                       double sliceOffsetOnAxis) {

  m_axisIndex = axisIndex;
  m_sliceOffsetOnAxis = sliceOffsetOnAxis;

  double bounds[6];
  vtkSMMultiSliceViewProxy::GetDataBounds(this->origSrc->getSourceProxy(), 0,
                                          bounds);
  if (axisIndex == 0) {
    m_edit->setValidator(new QDoubleValidator(bounds[0], bounds[1], 5, this));
  } else if (axisIndex == 1) {
    m_edit->setValidator(new QDoubleValidator(bounds[2], bounds[3], 5, this));
  } else if (axisIndex == 2) {
    m_edit->setValidator(new QDoubleValidator(bounds[4], bounds[5], 5, this));
  }
  m_contextMenu->exec(QCursor::pos());
}

void MultiSliceView::checkState(const QString &input) {
  auto validator = m_edit->validator();
  // temporary required to convert between const reference and reference.
  QString tmp_input = input;
  int ignored = 0;
  auto state = validator->validate(tmp_input, ignored);
  QString color = "QLineEdit { background-color: ";
  if (state == QValidator::Acceptable) {
    color.append("#c4df9b"); // green
  } else if (state == QValidator::Intermediate) {
    color.append("#fff79a"); // yellow
  } else {
    color.append("#f6989d"); // red
  }
  color.append(" }");
  m_edit->setStyleSheet(color);
}

void MultiSliceView::setSlicePosition() {

  double newPosition = m_edit->text().toDouble();

  std::string name;
  if (m_axisIndex == 0) {
    name = "XSlicesValues";
  } else if (m_axisIndex == 1) {
    name = "YSlicesValues";
  } else if (m_axisIndex == 2) {
    name = "ZSlicesValues";
  }

  auto viewProxy = m_mainView->getViewProxy();
  std::vector<double> slices =
      vtkSMPropertyHelper(viewProxy, name.c_str()).GetDoubleArray();
  auto it = std::find(slices.begin(), slices.end(), m_sliceOffsetOnAxis);
  if (it != slices.end()) {
    *it = newPosition;
  }
  vtkSMPropertyHelper(viewProxy, name.c_str())
      .Set(slices.data(), static_cast<int>(slices.size()));
  viewProxy->UpdateVTKObjects();

  m_contextMenu->hide();
  m_edit->clear();
}

/**
 * This function checks the sources for the WorkspaceName property. If not
 * found,
 * the ability to show a given cut in the SliceViewer will be deactivated.
 */
void MultiSliceView::checkSliceViewCompat() {
  QString wsName = this->getWorkspaceName();
  if (wsName.isEmpty()) {
    QObject::disconnect(this->m_mainView, nullptr, this, nullptr);
  }
}

void MultiSliceView::changedSlicePoint(Mantid::Kernel::VMD selectedPoint) {
  vtkSMPropertyHelper(this->m_mainView->getProxy(), "XSlicesValues")
      .Set(selectedPoint[0]);
  this->m_mainView->getProxy()->UpdateVTKObjects();
  this->m_mainView->render();
}

/**
 * This function is responsible for opening the given cut in SliceViewer.
 * It will gather all of the necessary information and create an XML
 * representation of the current dataset and cut parameters. That will then
 * be handed to the SliceViewer.
 * @param axisIndex the index of the slice to be opened in SliceViewer
 * @param sliceOffsetOnAxis position of the slice along given axis
 */
void MultiSliceView::showCutInSliceViewer(int axisIndex,
                                          double sliceOffsetOnAxis) {
  // Get the associated workspace name
  QString wsName = this->getWorkspaceName();

  // Have to jump through some hoops since a rebinner could be used.
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> srcs = smModel->findItems<pqPipelineSource *>();
  pqPipelineSource *src1 = nullptr;
  pqPipelineSource *src2 = nullptr;
  foreach (pqPipelineSource *src, srcs) {
    const QString name(src->getProxy()->GetXMLName());

    if (name.contains("ScaleWorkspace")) {
      src2 = src;
    }
  }

  src1 = smModel->getItemAtIndex<pqPipelineSource *>(0);

  // Get the current dataset characteristics
  const char *inGeomXML =
      vtkSMPropertyHelper(src1->getProxy(), "InputGeometryXML").GetAsString();
  // Check for timesteps and insert the value into the XML if necessary
  std::string geomXML;
  if (this->srcHasTimeSteps(src1)) {
    GeometryParser parser(inGeomXML);
    geomXML = parser.addTDimValue(this->getCurrentTimeStep());
  } else {
    geomXML = std::string(inGeomXML);
  }

  if (src2) {
    // Need to see if scaling is applied to axis
    QString scalingProperty("Scaling Factor");
    switch (axisIndex) {
    case 0:
      scalingProperty.prepend("X ");
      break;
    case 1:
      scalingProperty.prepend("Y ");
      break;
    case 2:
      scalingProperty.prepend("Z ");
      break;
    default:
      break;
    }

    std::vector<double> scaling =
        vtkSMPropertyHelper(src2->getProxy(),
                            scalingProperty.toLatin1().constData(), true)
            .GetDoubleArray();

    if (!scaling.empty()) {
      sliceOffsetOnAxis /= scaling[0];
    }
  }

  // Construct origin vector from orientation vector
  double origin[3] = {0.0, 0.0, 0.0};
  origin[axisIndex] = sliceOffsetOnAxis;

  vtkVector3d sliceNormalsInBasis[3] = {
      {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};

  // Create the XML holder
  VATES::VatesKnowledgeSerializer rks;
  rks.setWorkspaceName(wsName.toStdString());
  rks.setGeometryXML(geomXML);

  rks.setImplicitFunction(boost::make_shared<MDPlaneImplicitFunction>(
      3, sliceNormalsInBasis[axisIndex].GetData(), origin));
  QString titleAddition = "";

  // Use the WidgetFactory to create the slice viewer window
  SliceViewerWindow *w =
      MantidQt::Factory::WidgetFactory::Instance()->createSliceViewerWindow(
          wsName, titleAddition);
  try {
    // Set the slice points, etc, using the XML definition of the plane function
    w->getSlicer()->openFromXML(QString::fromStdString(rks.createXMLString()));
    w->show();
    this->connect(w->getSlicer(),
                  SIGNAL(changedSlicePoint(Mantid::Kernel::VMD)),
                  SLOT(changedSlicePoint(Mantid::Kernel::VMD)));
  } catch (std::runtime_error &e) {
    QString message =
        "The slice could not be shown because of the following error:\n" +
        QString(e.what());
    QMessageBox::warning(this, tr("MantidPlot"),
                         tr(message.toLatin1().constData()), QMessageBox::Ok,
                         QMessageBox::Ok);
    delete w;
  }
}

/**
 * This function closes user requested SliceViewer windows when the view is
 * closed. The function is a no-op (except for factory creation) when no
 * SliceViewer windows were requested.
 */
void MultiSliceView::closeSubWindows() {
  MantidQt::Factory::WidgetFactory::Instance()->closeAllSliceViewerWindows();
}

} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
