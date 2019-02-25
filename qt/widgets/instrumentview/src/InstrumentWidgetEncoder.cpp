// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/InstrumentView/InstrumentWidgetEncoder.h"
#include "MantidQtWidgets/InstrumentView/ColorBar.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentTreeWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h"
#include "MantidQtWidgets/InstrumentView/MaskBinsData.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtWidgets/InstrumentView/Shape2D.h"
#include "MantidQtWidgets/InstrumentView/XIntegrationControl.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QRadioButton>

namespace MantidQt {
namespace MantidWidgets {

InstrumentWidgetEncoder::InstrumentWidgetEncoder()
    : m_projectPath(""), m_saveMask(true) {}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encode(const InstrumentWidget &obj,
                                const QString &projectPath,
                                const bool saveMask) {
  QMap<QString, QVariant> map;
  m_projectPath = projectPath.toStdString();
  m_saveMask = saveMask;

  map.insert(QString("workspaceName"), QVariant(obj.getWorkspaceName()));

  map.insert(QString("surfaceType"), QVariant(obj.getSurfaceType()));

  map.insert(QString("currentTab"), QVariant(obj.getCurrentTab()));

  QList<QVariant> energyTransferList;
  energyTransferList.append(QVariant(obj.m_xIntegration->getMinimum()));
  energyTransferList.append(QVariant(obj.m_xIntegration->getMaximum()));
  map.insert(QString("energyTransfer"), QVariant(energyTransferList));

  map.insert(QString("surface"),
             QVariant(this->encodeSurface(obj.getSurface())));
  map.insert(QString("actor"),
             QVariant(this->encodeActor(obj.m_instrumentActor)));
  map.insert(QString("tabs"), QVariant(this->encodeTabs(obj)));

  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeTabs(const InstrumentWidget &obj) {
  QMap<QString, QVariant> tabs;

  tabs.insert(QString("maskTab"), QVariant(encodeMaskTab(obj.m_maskTab)));
  tabs.insert(QString("renderTab"), QVariant(encodeRenderTab(obj.m_renderTab)));
  tabs.insert(QString("treeTab"), QVariant(encodeTreeTab(obj.m_treeTab)));
  tabs.insert(QString("pickTab"), QVariant(encodePickTab(obj.m_pickTab)));

  return tabs;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeTreeTab(const InstrumentWidgetTreeTab *tab) {
  auto index = tab->m_instrumentTree->currentIndex();
  auto model = index.model();

  QMap<QString, QVariant> map;

  if (model) {
    auto item = model->data(index);
    QString name = item.value<QString>();
    map.insert(QString("selectedComponent"), QVariant(name));
  }

  QList<QString> list;
  const auto names = tab->m_instrumentTree->findExpandedComponents();
  for (const auto name : names) {
    list.append(name);
  }
  map.insert(QString("expandedItems"), QVariant(list));

  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeRenderTab(const InstrumentWidgetRenderTab *tab) {
  QMap<QString, QVariant> map;
  map.insert(QString("axesView"), QVariant(tab->mAxisCombo->currentIndex()));
  map.insert(QString("autoScaling"), QVariant(tab->m_autoscaling->isChecked()));
  map.insert(QString("displayAxes"), QVariant(tab->m_displayAxes->isChecked()));
  map.insert(QString("flipView"), QVariant(tab->m_flipCheckBox->isChecked()));
  map.insert(QString("displayDetectorsOnly"),
             QVariant(tab->m_displayDetectorsOnly->isChecked()));
  map.insert(QString("displayWireframe"),
             QVariant(tab->m_wireframe->isChecked()));
  map.insert(QString("displayLighting"),
             QVariant(tab->m_lighting->isChecked()));
  map.insert(QString("useOpenGL"), QVariant(tab->m_GLView->isChecked()));
  map.insert(QString("useUCorrection"),
             QVariant(tab->m_UCorrection->isChecked()));

  const auto surface = tab->getSurface();
  map.insert(QString("showLabels"), QVariant(surface->getShowPeakRowsFlag()));
  map.insert(QString("showRows"), QVariant(surface->getShowPeakRowsFlag()));
  map.insert(QString("labelPrecision"),
             QVariant(surface->getPeakLabelPrecision()));
  map.insert(QString("showRelativeIntensity"),
             QVariant(surface->getShowPeakRelativeIntensityFlag()));

  const auto colorBar = encodeColorBar(tab->m_colorBarWidget);
  map.insert(QString("colorBar"), QVariant(colorBar));

  return map;
}

QMap<QString, QVariant> InstrumentWidgetEncoder::encodeColorBar(
    MantidQt::MantidWidgets::ColorBar *bar) {
  QMap<QString, QVariant> map;

  map.insert(QString("scaleType"), QVariant(bar->getScaleType()));
  map.insert(QString("power"), QVariant(bar->getNthPower()));
  map.insert(QString("min"), QVariant(bar->getMinValue()));
  map.insert(QString("max"), QVariant(bar->getMaxValue()));

  return map;
}

// This is the tab labelled draw
QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeMaskTab(const InstrumentWidgetMaskTab *tab) {
  QMap<QString, QVariant> map;
  QMap<QString, QVariant> activeTools;
  QMap<QString, QVariant> activeType;

  activeTools.insert(QString("moveButton"), QVariant(tab->m_move->isChecked()));
  activeTools.insert(QString("pointerButton"),
                     QVariant(tab->m_pointer->isChecked()));
  activeTools.insert(QString("ellipseButton"),
                     QVariant(tab->m_ellipse->isChecked()));
  activeTools.insert(QString("ringEllipseButton"),
                     QVariant(tab->m_ring_ellipse->isChecked()));
  activeTools.insert(QString("ringRectangleButton"),
                     QVariant(tab->m_ring_rectangle->isChecked()));
  activeTools.insert(QString("freeDrawButton"),
                     QVariant(tab->m_free_draw->isChecked()));
  map.insert(QString("activeTools"), QVariant(activeTools));

  activeType.insert(QString("maskingOn"),
                    QVariant(tab->m_masking_on->isChecked()));
  activeType.insert(QString("groupingOn"),
                    QVariant(tab->m_grouping_on->isChecked()));
  activeType.insert(QString("roiOn"), QVariant(tab->m_roi_on->isChecked()));
  map.insert(QString("activeType"), QVariant(activeType));

  if (m_saveMask) {
    // Save the masks applied to view but not saved to a workspace
    auto wsName = tab->m_instrWidget->getWorkspaceName() + "MaskView.xml";
    bool success =
        tab->saveMaskViewToProject(wsName.toStdString(), m_projectPath);
    map.insert(QString("maskWorkspaceSaved"), QVariant(success));
    if (success) {
      map.insert(QString("maskWorkspaceName"), QVariant(wsName));
    }
  } else {
    map.insert(QString("maskWorkspaceSaved"), QVariant(false));
  }

  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodePickTab(const InstrumentWidgetPickTab *tab) {
  QMap<QString, QVariant> map;

  // Save whether a button is active or not
  map.insert(QString("zoom"), QVariant(tab->m_zoom->isChecked()));
  map.insert(QString("edit"), QVariant(tab->m_edit->isChecked()));
  map.insert(QString("ellipse"), QVariant(tab->m_ellipse->isChecked()));
  map.insert(QString("rectangle"), QVariant(tab->m_rectangle->isChecked()));
  map.insert(QString("ringEllipse"),
             QVariant(tab->m_ring_ellipse->isChecked()));
  map.insert(QString("ringRectangle"),
             QVariant(tab->m_ring_rectangle->isChecked()));
  map.insert(QString("freeDraw"), QVariant(tab->m_free_draw->isChecked()));
  map.insert(QString("one"), QVariant(tab->m_one->isChecked()));
  map.insert(QString("tube"), QVariant(tab->m_tube->isChecked()));
  map.insert(QString("peak"), QVariant(tab->m_peak->isChecked()));
  map.insert(QString("peakSelect"), QVariant(tab->m_peakSelect->isChecked()));

  return map;
}

QMap<QString, QVariant> InstrumentWidgetEncoder::encodeActor(
    const std::unique_ptr<InstrumentActor> &obj) {
  QMap<QString, QVariant> map;

  map.insert(QString("fileName"), QVariant(obj->getCurrentColorMap()));
  map.insert(QString("binMasks"),
             QVariant(this->encodeMaskBinsData(obj->m_maskBinsData)));

  return map;
}

QList<QVariant>
InstrumentWidgetEncoder::encodeMaskBinsData(const MaskBinsData &obj) {
  QList<QVariant> list;

  for (const auto &binMask : obj.m_masks) {
    list.append(QVariant(this->encodeBinMask(binMask)));
  }

  return list;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeBinMask(const BinMask &obj) {
  QMap<QString, QVariant> map;

  QList<QVariant> range;
  // Convert Doubles to QString
  range.append(QVariant(QString("%1").arg(obj.start)));
  range.append(QVariant(QString("%1").arg(obj.end)));
  map.insert(QString("range"), QVariant(range));

  QList<QVariant> spectra;
  for (const auto &spectrum : obj.spectra) {
    spectra.append(QVariant(quint64(spectrum)));
  }
  map.insert(QString("spectra"), QVariant(spectra));

  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeSurface(const ProjectionSurface_sptr &obj) {
  QMap<QString, QVariant> map;

  auto projection3D = boost::dynamic_pointer_cast<Projection3D>(obj);
  if (projection3D) {
    map.insert(QString("projection3DSuccess"), QVariant(true));
    map.insert(QString("projection3D"),
               QVariant(this->encodeProjection3D(*projection3D)));
  } else {
    map.insert(QString("projection3DSuccess"), QVariant(false));
  }

  map.insert(QString("backgroundColor"), QVariant(obj->m_backgroundColor));
  map.insert(QString("shapes"),
             QVariant(this->encodeMaskShapes(obj->m_maskShapes)));
  map.insert(QString("alignmentInfo"),
             QVariant(this->encodeAlignmentInfo(obj)));

  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeProjection3D(const Projection3D &obj) {
  QMap<QString, QVariant> map;
  map.insert(QString("viewport"),
             QVariant(this->encodeViewPort(obj.m_viewport)));
  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeViewPort(const Viewport &obj) {
  QMap<QString, QVariant> map;
  QMap<QString, QVariant> translationMap;

  translationMap.insert(QString("xTrans"), QVariant(obj.m_xTrans));
  translationMap.insert(QString("yTrans"), QVariant(obj.m_yTrans));
  map.insert(QString("translation"), QVariant(translationMap));

  map.insert(QString("zoom"), QVariant(obj.m_zoomFactor));

  QList<QVariant> rotation;
  for (auto i = 0; i < 4; ++i) {
    rotation.append(obj.m_quaternion[i]);
  }
  map.insert(QString("rotation"), QVariant(rotation));

  return map;
}

QList<QVariant>
InstrumentWidgetEncoder::encodeMaskShapes(const Shape2DCollection &obj) {
  QList<QVariant> list;

  for (const auto &shape : obj.m_shapes) {
    list.append(QVariant(this->encodeShape(shape)));
  }

  return list;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeShape(const Shape2D *obj) {
  QMap<QString, QVariant> map;

  map.insert(QString("properties"), QVariant(this->encodeShapeProperties(obj)));

  const auto color = obj->getColor();
  QMap<QString, QVariant> colorMap;
  colorMap.insert(QString("red"), QVariant(color.red()));
  colorMap.insert(QString("green"), QVariant(color.green()));
  colorMap.insert(QString("blue"), QVariant(color.blue()));
  colorMap.insert(QString("alpha"), QVariant(color.alpha()));
  map.insert(QString("color"), QVariant(colorMap));

  const auto fillColor = obj->getFillColor();
  QMap<QString, QVariant> fillColorMap;
  fillColorMap.insert(QString("red"), QVariant(fillColor.red()));
  fillColorMap.insert(QString("green"), QVariant(fillColor.green()));
  fillColorMap.insert(QString("blue"), QVariant(fillColor.blue()));
  fillColorMap.insert(QString("alpha"), QVariant(fillColor.alpha()));
  map.insert(QString("fillColor"), QVariant(fillColorMap));

  QMap<QString, QVariant> subShapeMap;
  if (obj->type() == "ellipse") {
    subShapeMap = this->encodeEllipse(static_cast<const Shape2DEllipse *>(obj));
    map.insert(QString("type"), QVariant(QString("ellipse")));
  } else if (obj->type() == "rectangle") {
    subShapeMap =
        this->encodeRectangle(static_cast<const Shape2DRectangle *>(obj));
    map.insert(QString("type"), QVariant(QString("rectangle")));
  } else if (obj->type() == "ring") {
    subShapeMap = this->encodeRing(static_cast<const Shape2DRing *>(obj));
    map.insert(QString("type"), QVariant(QString("ring")));
  } else if (obj->type() == "free") {
    subShapeMap = this->encodeFree(static_cast<const Shape2DFree *>(obj));
    map.insert(QString("type"), QVariant(QString("free")));
  } else {
    throw std::runtime_error("InstrumentView - Could not encode shape");
  }

  map.insert(QString("subShapeMap"), QVariant(subShapeMap));

  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeEllipse(const Shape2DEllipse *obj) {
  const double radius1 = obj->getDouble("radius1");
  const double radius2 = obj->getDouble("radius2");
  const auto centre = obj->getPoint("centre");

  QMap<QString, QVariant> map;

  map.insert(QString("radius1"), QVariant(radius1));
  map.insert(QString("radius2"), QVariant(radius2));
  map.insert(QString("x"), QVariant(centre.x()));
  map.insert(QString("y"), QVariant(centre.y()));

  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeRectangle(const Shape2DRectangle *obj) {
  const auto x0 = obj->m_boundingRect.x0();
  const auto x1 = obj->m_boundingRect.x1();
  const auto y0 = obj->m_boundingRect.y0();
  const auto y1 = obj->m_boundingRect.y1();

  QMap<QString, QVariant> map;

  map.insert(QString("x0"), QVariant(x0));
  map.insert(QString("y0"), QVariant(y0));
  map.insert(QString("x1"), QVariant(x1));
  map.insert(QString("y1"), QVariant(y1));

  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeRing(const Shape2DRing *obj) {
  const auto xWidth = obj->getDouble("xwidth");
  const auto yWidth = obj->getDouble("ywidth");
  auto baseShape = obj->getOuterShape()->clone();

  QMap<QString, QVariant> map;
  map.insert(QString("xWidth"), QVariant(xWidth));
  map.insert(QString("yWidth"), QVariant(yWidth));
  map.insert(QString("shape"), QVariant(this->encodeShape(baseShape)));

  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeFree(const Shape2DFree *obj) {
  const auto polygon = obj->m_polygon;

  QList<QVariant> parameters;

  for (const auto &point : polygon) {
    QList<QVariant> list;
    list.append(QVariant(point.x()));
    list.append(QVariant(point.y()));
    parameters.append(QVariant(list));
  }

  QMap<QString, QVariant> map;
  map.insert(QString("paramaters"), QVariant(parameters));
  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeShapeProperties(const Shape2D *obj) {
  QMap<QString, QVariant> map;

  map.insert(QString("scalable"), QVariant(obj->m_scalable));
  map.insert(QString("editing"), QVariant(obj->m_editing));
  map.insert(QString("selected"), QVariant(obj->m_selected));
  map.insert(QString("visible"), QVariant(obj->m_visible));

  return map;
}

QList<QVariant> InstrumentWidgetEncoder::encodeAlignmentInfo(
    const ProjectionSurface_sptr &obj) {
  QList<QVariant> list;

  for (const auto &item : obj->m_selectedAlignmentPlane) {
    const auto qLab = item.first;
    QMap<QString, QVariant> qLabMap;
    qLabMap.insert(QString("x"), QVariant(qLab.X()));
    qLabMap.insert(QString("y"), QVariant(qLab.Y()));
    qLabMap.insert(QString("z"), QVariant(qLab.Z()));

    QList<QVariant> itemList;
    itemList.append(QVariant(qLabMap));     // qLabMap
    itemList.append(QVariant(item.second)); // marker

    list.append(itemList);
  }

  return list;
}
} // namespace MantidWidgets
} // namespace MantidQt