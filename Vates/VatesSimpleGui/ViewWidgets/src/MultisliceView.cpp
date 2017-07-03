#include "MantidVatesSimpleGuiViewWidgets/MultisliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/RebinnedSourcesManager.h"
#include "MantidVatesSimpleGuiQtWidgets/GeometryParser.h"
#include "MantidGeometry/MDGeometry/MDPlaneImplicitFunction.h"
#include "MantidQtSliceViewer/SliceViewerWindow.h"
#include "MantidQtFactory/WidgetFactory.h"
#include "MantidVatesAPI/VatesKnowledgeSerializer.h"

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
#include <vtkSMPropertyHelper.h>
#include <vtkSMSourceProxy.h>
#include <vtkVector.h>

#include <QString>

using namespace Mantid::Geometry;
using namespace MantidQt::SliceViewer;

namespace Mantid {
namespace Vates {
namespace SimpleGui {

static void GetOrientations(vtkSMSourceProxy *producer,
                            vtkVector3d sliceNormals[3]) {
  bool isvalid = false;
  vtkTuple<double, 16> cobm =
      pqModelTransformSupportBehavior::getChangeOfBasisMatrix(producer, 0,
                                                              &isvalid);
  if (isvalid) {
    vtkNew<vtkMatrix4x4> changeOfBasisMatrix;
    std::copy(&cobm[0], &cobm[0] + 16, &changeOfBasisMatrix->Element[0][0]);
    vtkVector3d axisBases[3];
    vtkPVChangeOfBasisHelper::GetBasisVectors(changeOfBasisMatrix.GetPointer(),
                                              axisBases[0], axisBases[1],
                                              axisBases[2]);
    for (int cc = 0; cc < 3; cc++) {
      sliceNormals[cc] = axisBases[(cc + 1) % 3].Cross(axisBases[(cc + 2) % 3]);
      sliceNormals[cc].Normalize();
    }
  } else {
    sliceNormals[0] = vtkVector3d(1, 0, 0);
    sliceNormals[1] = vtkVector3d(0, 1, 0);
    sliceNormals[2] = vtkVector3d(0, 0, 1);
  }
}

MultiSliceView::MultiSliceView(QWidget *parent,
                               RebinnedSourcesManager *rebinnedSourcesManager,
                               bool createRenderProxy)
    : ViewBase(parent, rebinnedSourcesManager) {
  this->m_ui.setupUi(this);
  if (createRenderProxy) {
    pqRenderView *tmp =
        this->createRenderView(this->m_ui.renderFrame, QString("MultiSlice"));

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
        this->origSrc->getOutputPort(0), this->m_mainView);
    vtkSMPropertyHelper(drep->getProxy(), "Representation").Set("Slices");
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
  if (modifier == vtkContextMouseEvent::SHIFT_MODIFIER &&
      (button == vtkContextMouseEvent::LEFT_BUTTON ||
       button == vtkContextMouseEvent::RIGHT_BUTTON)) {
    this->showCutInSliceViewer(axisIndex, sliceOffsetOnAxis);
  }
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
                            scalingProperty.toAscii().constData(),
                            true).GetDoubleArray();

    if (!scaling.empty()) {
      sliceOffsetOnAxis /= scaling[0];
    }
  }

  vtkVector3d sliceNormals[3];
  GetOrientations(vtkSMSourceProxy::SafeDownCast(src1->getProxy()),
                  sliceNormals);
  vtkVector3d &orient = sliceNormals[axisIndex];

  // Construct origin vector from orientation vector
  double origin[3];
  origin[0] = sliceOffsetOnAxis * orient[0];
  origin[1] = sliceOffsetOnAxis * orient[1];
  origin[2] = sliceOffsetOnAxis * orient[2];

  // Create the XML holder
  VATES::VatesKnowledgeSerializer rks;
  rks.setWorkspaceName(wsName.toStdString());
  rks.setGeometryXML(geomXML);

  rks.setImplicitFunction(
      boost::make_shared<MDPlaneImplicitFunction>(3, orient.GetData(), origin));
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
                         tr(message.toAscii().constData()), QMessageBox::Ok,
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
