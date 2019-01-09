// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/InstrumentView/InstrumentWidgetDecoder.h"

#include "MantidQtWidgets/InstrumentView/ColorBar.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentTreeWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h"
#include "MantidQtWidgets/InstrumentView/MaskBinsData.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtWidgets/InstrumentView/Shape2D.h"
#include "MantidQtWidgets/InstrumentView/Shape2DCollection.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPushButton>
#include <QRadioButton>
#include <QString>

namespace MantidQt {
namespace MantidWidgets {

InstrumentWidgetDecoder::InstrumentWidgetDecoder()
    : m_projectPath(""), m_workspaceName("") {}

void InstrumentWidgetDecoder::decode(const QMap<QString, QVariant> &map,
                                     InstrumentWidget &obj,
                                     const QString &projectPath) {
  m_projectPath = projectPath;

  m_workspaceName = map[QString("workspaceName")].toString();

  const auto surfaceType = map[QString("surfaceType")].toString();
  obj.setSurfaceType(surfaceType);

  const auto currentTab = map[QString("currentTab")].toInt();
  obj.selectTab(currentTab);

  const auto energyTransferList = map[QString("energyTransfer")].toList();
  const auto min = energyTransferList[0].toDouble();
  const auto max = energyTransferList[1].toDouble();
  obj.setBinRange(min, max);

  this->decodeSurface(map[QString("surface")].toMap(), obj.getSurface());
  this->decodeActor(map[QString("actor")].toMap(), obj.m_instrumentActor);
  this->decodeTabs(map[QString("tabs")].toMap(), obj);
}

void InstrumentWidgetDecoder::decodeTabs(const QMap<QString, QVariant> &map,
                                         InstrumentWidget &obj) {
  this->decodeMaskTab(map[QString("maskTab")].toMap(), obj.m_maskTab);
  this->decodeRenderTab(map[QString("renderTab")].toMap(), obj.m_renderTab);
  this->decodeTreeTab(map[QString("treeTab")].toMap(), obj.m_treeTab);
  this->decodePickTab(map[QString("pickTab")].toMap(), obj.m_pickTab);
}

void InstrumentWidgetDecoder::decodeMaskTab(const QMap<QString, QVariant> &map,
                                            InstrumentWidgetMaskTab *obj) {
  const auto activeTools = map[QString("activeTools")].toMap();
  const auto activeType = map[QString("activeType")].toMap();

  // Decode the active tools
  obj->m_move->setChecked(activeTools["moveButton"].toBool());
  obj->m_pointer->setChecked(activeTools["pointerButton"].toBool());
  obj->m_ellipse->setChecked(activeTools["ellipseButton"].toBool());
  obj->m_ring_ellipse->setChecked(activeTools["ringEllipseButton"].toBool());
  obj->m_ring_rectangle->setChecked(
      activeTools["ringRectangleButton"].toBool());
  obj->m_free_draw->setChecked(activeTools["freeFrawButton"].toBool());

  // Decode the active type
  obj->m_masking_on->setChecked(activeType["maskingOn"].toBool());
  obj->m_grouping_on->setChecked(activeType["groupingOn"].toBool());
  obj->m_roi_on->setChecked(activeType["roiOn"].toBool());

  // Load the masks applied to view but not saved to a workspace (But also
  // including those saved to the workspace)
  const auto success = map["maskWorkspaceSaved"].toBool();
  if (success) {
    const auto wsName = map["maskWorkspaceName"].toString().toStdString();
    obj->loadMaskViewFromProject(wsName);
  }
}

void InstrumentWidgetDecoder::decodeRenderTab(
    const QMap<QString, QVariant> &map, InstrumentWidgetRenderTab *obj) {
  // Load buttons/settings
  obj->mAxisCombo->setCurrentIndex(map[QString("axesView")].toInt());
  obj->m_autoscaling->setChecked(map[QString("autoScaling")].toBool());
  obj->m_displayAxes->setChecked(map[QString("displayAxis")].toBool());
  obj->m_flipCheckBox->setChecked(map[QString("flipView")].toBool());
  obj->m_displayDetectorsOnly->setChecked(
      map[QString("displayDetectorsOnly")].toBool());
  obj->m_wireframe->setChecked(map[QString("displayWireframe")].toBool());
  obj->m_lighting->setChecked(map[QString("displayLighting")].toBool());
  obj->m_GLView->setChecked(map[QString("useOpenGL")].toBool());
  obj->m_UCorrection->setChecked(map[QString("useUCorrection")].toBool());

  // Load the surface
  auto surface = obj->getSurface();
  surface->setShowPeakLabelsFlag(map[QString("showLabels")].toBool());
  surface->setShowPeakRowsFlag(map[QString("showRows")].toBool());
  surface->setPeakLabelPrecision(map[QString("labelPrecision")].toBool());
  surface->setShowPeakRelativeIntensityFlag(
      map[QString("showRelativeIntensity")].toBool());

  // Load color bar
  this->decodeColorBar(map[QString("colormap")].toMap(), obj->m_colorBarWidget);
}

void InstrumentWidgetDecoder::decodeColorBar(const QMap<QString, QVariant> &map,
                                             ColorBar *bar) {
  bar->setScaleType(map[QString("scaleType")].toInt());
  bar->setNthPower(map[QString("power")].toDouble());
  bar->setMinValue(map[QString("min")].toDouble());
  bar->setMaxValue(map[QString("max")].toDouble());
}

void InstrumentWidgetDecoder::decodeTreeTab(const QMap<QString, QVariant> &map,
                                            InstrumentWidgetTreeTab *obj) {
  obj->selectComponentByName(map[QString("selectedComponent")].toString());

  auto names = map[QString("expandedItems")].toList();
  for (auto &name : names) {
    auto index = obj->m_instrumentTree->findComponentByName(name.toString());
    obj->m_instrumentTree->setExpanded(index, true);
  }
}

void InstrumentWidgetDecoder::decodePickTab(const QMap<QString, QVariant> &map,
                                            InstrumentWidgetPickTab *obj) {
  obj->m_zoom->setChecked(map[QString("zoom")].toBool());
  obj->m_edit->setChecked(map[QString("edit")].toBool());
  obj->m_ellipse->setChecked(map[QString("ellipse")].toBool());
  obj->m_rectangle->setChecked(map[QString("rectangle")].toBool());
  obj->m_ring_ellipse->setChecked(map[QString("ringEllipse")].toBool());
  obj->m_ring_rectangle->setChecked(map[QString("ringRectangle")].toBool());
  obj->m_free_draw->setChecked(map[QString("freeDraw")].toBool());
  obj->m_one->setChecked(map[QString("one")].toBool());
  obj->m_tube->setChecked(map[QString("tube")].toBool());
  obj->m_peak->setChecked(map[QString("peak")].toBool());
  obj->m_peakSelect->setChecked(map[QString("peakSelect")].toBool());
}

void InstrumentWidgetDecoder::decodeActor(
    const QMap<QString, QVariant> &map, std::unique_ptr<InstrumentActor> &obj) {
  obj->loadColorMap(map[QString("fileName")].toString());

  this->decodeBinMasks(map[QString("binMasks")].toList(), obj->m_maskBinsData);
}

void InstrumentWidgetDecoder::decodeBinMasks(const QList<QVariant> &list,
                                             MaskBinsData &obj) {
  for (const auto &item : list) {
    const auto itemMap = item.toMap();
    const auto range = itemMap[QString("range")].toList();
    double start = range[0].toDouble();
    double end = range[1].toDouble();

    const auto spectraList = itemMap["spectra"].toList();
    std::vector<size_t> spectra;
    for (const auto &spec : spectraList) {
      spectra.emplace_back(spec.value<size_t>());
    }
    obj.addXRange(start, end, spectra);
  }
}

void InstrumentWidgetDecoder::decodeSurface(
    const QMap<QString, QVariant> &map,
    boost::shared_ptr<ProjectionSurface> obj) {
  QMap<QString, QVariant> color = map[QString("backgroundColor")].toMap();
  QColor qColor(color[QString("red")].toInt(), color[QString("green")].toInt(),
                color[QString("blue")].toInt(),
                color[QString("alpha")].toInt());

  obj->m_backgroundColor = qColor;

  this->decodeMaskShapes(map[QString("shapes")].toList(), obj->m_maskShapes);
  this->decodeAlignmentInfo(map[QString("alignmentInfo")].toList(), obj);
}

void InstrumentWidgetDecoder::decodeMaskShapes(const QList<QVariant> &list,
                                               Shape2DCollection &obj) {
  connect(this, SIGNAL(this->shapeCreated()), &obj, SLOT(obj->shapeCreated()));
  for (const auto &shape : list) {
    Shape2D *created_shape = this->decodeShape(shape.toMap());
    obj.m_shapes.push_back(created_shape);
    emit shapeCreated();
  }
}

Shape2D *
InstrumentWidgetDecoder::decodeShape(const QMap<QString, QVariant> &map) {
  const auto type = map[QString("type")].toString().toStdString();

  Shape2D *shape = nullptr;
  if (type == "ellipse") {
    shape = this->decodeEllipse(map[QString("subShapeMap")].toMap());
  } else if (type == "rectangle") {
    shape = this->decodeRectangle(map[QString("subShapeMap")].toMap());
  } else if (type == "ring") {
    shape = this->decodeRing(map[QString("subShapeMap")].toMap());
  } else if (type == "free") {
    shape = this->decodeFree(map[QString("subShapeMap")].toMap());
  } else {
    throw std::runtime_error("InstrumentView - Could not decode shape");
  }

  shape->setScalable(map[QString("scalable")].toBool());
  shape->edit(map[QString("editing")].toBool());
  shape->setSelected(map[QString("selected")].toBool());
  shape->setVisible(map[QString("visible")].toBool());

  QMap<QString, QVariant> color1 = map[QString("color")].toMap();
  QColor qColor(
      color1[QString("red")].toInt(), color1[QString("green")].toInt(),
      color1[QString("blue")].toInt(), color1[QString("alpha")].toInt());

  shape->setColor(qColor);

  QMap<QString, QVariant> color2 = map[QString("fillColor")].toMap();
  QColor qFillColor(
      color2[QString("red")].toInt(), color2[QString("green")].toInt(),
      color2[QString("blue")].toInt(), color2[QString("alpha")].toInt());
  shape->setFillColor(qFillColor);

  return shape;
}

Shape2D *
InstrumentWidgetDecoder::decodeEllipse(const QMap<QString, QVariant> &map) {
  const auto radius1 = map[QString("radius1")].toDouble();
  const auto radius2 = map[QString("radius2")].toDouble();
  const auto x = map[QString("x")].toDouble();
  const auto y = map[QString("y")].toDouble();

  return new Shape2DEllipse(QPointF(x, y), radius1, radius2);
}

Shape2D *
InstrumentWidgetDecoder::decodeRectangle(const QMap<QString, QVariant> &map) {
  const auto x0 = map[QString("x0")].toDouble();
  const auto y0 = map[QString("y0")].toDouble();
  const auto x1 = map[QString("x1")].toDouble();
  const auto y1 = map[QString("y1")].toDouble();

  const QPointF point1(x0, y0);
  const QPointF point2(x1, y1);
  return new Shape2DRectangle(point1, point2);
}

Shape2D *
InstrumentWidgetDecoder::decodeRing(const QMap<QString, QVariant> &map) {
  const auto xWidth = map[QString("xWidth")].toDouble();
  const auto yWidth = map[QString("yWidth")].toDouble();

  const auto baseShape = this->decodeShape(map[QString("shape")].toMap());
  return new Shape2DRing(baseShape, xWidth, yWidth);
}

Shape2D *
InstrumentWidgetDecoder::decodeFree(const QMap<QString, QVariant> &map) {
  QPolygonF polygon;

  for (const auto param : map[QString("paramaters")].toList()) {
    const auto paramList = param.toList();
    const double x = paramList[0].toDouble();
    const double y = paramList[1].toDouble();

    polygon << QPointF(x, y);
  }

  return new Shape2DFree(polygon);
}

void InstrumentWidgetDecoder::decodeAlignmentInfo(
    const QList<QVariant> &list, boost::shared_ptr<ProjectionSurface> &obj) {

  std::vector<std::pair<Mantid::Kernel::V3D, QPointF>> alignmentPlane;
  for (const auto &item : list) {
    const auto itemList = item.toList();
    const auto qLabMap = itemList[0].toMap();
    const auto marker = itemList[1].toPointF();
    Mantid::Kernel::V3D qValue(qLabMap[QString("x")].toDouble(),
                               qLabMap[QString("y")].toDouble(),
                               qLabMap[QString("z")].toDouble());

    alignmentPlane.push_back(std::make_pair(qValue, marker));
  }
  obj->m_selectedAlignmentPlane = alignmentPlane;
}
} // namespace MantidWidgets
} // namespace MantidQt