// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/InstrumentView/InstrumentWidgetDecoder.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"

InstrumentWidgetDecoder::InstrumentWidgetDecoder()
    : m_projectPath(""), m_workspaceName("") {}

void InstrumentWidgetDecoder::decoder(const QMap<QString, QVariant> &map,
                                      InstrumentWidget &obj,
                                      const QString &projectPath) {
  m_projectPath = projectPath;

  m_workspaceName = map[QString("workspaceName")].toString();

  const auto surfaceType = map[QString("surfaceType")].toString();
  obj.setSurfaceType(surfaceType);

  const auto currentTab = map[QString("currentTab")].toInt();
  obj.selectTab(currentTab);

  const auto energyTransferList = map[QString("energyTransfer")].toList();
  const auto min = energyTransferList[0];
  const auto max = energyTransferList[1];
  obj.setBinRange(min, max);

  this->decodeSurface(map[QString("surface")], obj.getSurface());
  this->decodeActor(map[QString("actor")], obj.m_instrumentActor);
  this->decodeTabs(map[QString("tabs")], obj);
}

void InstrumentWidgetDecoder::decodeTabs(const QMap<QString, QVariant> &map,
                                         InstrumentWidget &obj) {
  this->decodeMaskTab(map[QString("maskTab")], obj.m_maskTab);
  this->decodeRenderTab(map[QString("renderTab")], obj.m_renderTab);
  this->decodeTreeTab(map[QString("treeTab")], obj.m_treeTab);
  this->decodePickTab(map[QString("pickTab")], obj.m_pickTab);
}

void InstrumentWidgetDecoder::decodeMaskTab(const QMap<QString, QVariant> &map,
                                            InstrumentWidgetMaskTab *obj) {}

void InstrumentWidgetDecoder::decodeRenderTab(
    const QMap<QString, QVariant> &map, InstrumentWidgetRenderTab *obj) {}

void InstrumentWidgetDecoder::decodeColorBar(const QMap<QString, QVariant> &map, ColorBar *bar){

}

void InstrumentWidgetDecoder::decodeTreeTab(const QMap<QString, QVariant> &map,
                                            InstrumentWidgetTreeTab *obj) {
  obj->selectComponentByName(
      map[QString("selectedComponent")].toString().toStdString());

  auto names = map[QString("expandedItems")].toList();
  for (auto &name : names) {
    auto index = obj->m_instrumentTree->findComponentByName(name.toString());
    m_instrumentTree->setExpanded(index, true);
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
    const QMap<QString, QVariant> &map,
    const std::unique_ptr<InstrumentActor> &obj) {
  obj->loadColormap(map[QString("fileName")].getString());

  this->decodeBinMasks(map[QString("binMasks")], obj->m_maskBinsData);
}

void InstrumentWidgetDecoder::decodeBinMasks(const QList<QVariant> &list,
                                             MaskBinsData &obj) {
  for (const auto &item : list) {
    const auto range = item[QString("range")];
    double start = range[0] double end = range[1]

        const auto spectraList = item["spectra"];
    std::vector<size_t> spectra;
    for (const auto &spec : spectraList) {
      spectra.push_back(spec).value<size_t>());
    }
    obj.addXRange(start, end, spectra);
  }
}

void InstrumentWidgetDecoder::decodeSurface(const QMap<QString, QVariant> &map,
                                            ProjectionSurface_sptr &obj) {
  QMap<QString, QVariant> color = map[QString("backgroundColor")];
  QColor qColor(color[QString("red")].toInt(), color[QString("green")].toInt(),
                color[QString("blue").toInt()],
                color[QString("alpha").toInt()]);

  obj->m_backgroundColor = qColor;

  this->decodeMaskShapes(map[QString("shapes")], obj->m_maskShapes);
  this->decodeAlignmentInfo(map, obj);
}

void InstrumentWidgetDecoder::decodeMaskShapes(const QList<QVariant> &list,
                                               const Shape2DCollection &obj) {
  connect(this, SIGNAL(this->shapeCreated()), obj, SLOT(shapeCreated()));
  for (const auto &shape : list) {
    Shape2D *shape = this->decodeShape(QMap<QString, QList> shape);
    m_shapes.push_back(shape);
    emit shapeCreated();
  }
}

Shape2D *shape
InstrumentWidgetDecoder::decodeShape(const QMap<QString, QVariant> &map) {
  if (!map[QString("type")]) {
    return nullptr;
  }
  const auto type = map[QString("type")].toString().toStdString();

  Shape2D *shape = nullptr;
  if (type == "ellipse") {
    shape = this->decodeEllipse(map);
  } else if (type == "rectangle") {
    shape = this->decodeRectangle(map);
  } else if (type == "ring") {
    shape = this->decodeRing(map);
  } else if (type == "free") {
    shape = this->decodeFree(map);
  } else {
    throw std::runtime_error("InstrumentView - Could not decode shape");
  }

  shape->setScalable(map[QString("scalable")].toBool());
  shape->edit(map[QString("editing")].toBool());
  shape->setSelected(map[QString("selected")].toBool());
  shape->setVisible(map[QString("visible")].toBool());

  QMap<QString, QVariant> color = map[QString("color")];
  QColor qColor(color[QString("red")].toInt(), color[QString("green")].toInt(),
                color[QString("blue").toInt()],
                color[QString("alpha").toInt()]);

  shape->setColor(qColor);

  QMap<QString, QVariant> color = map[QString("fillColor")];
  QColor qFillColor(
      color[QString("red")].toInt(), color[QString("green")].toInt(),
      color[QString("blue").toInt()], color[QString("alpha").toInt()]);
  shape->setFillColor(qFillColor);

  return shape;
}

Shape2D *shape
InstrumentWidgetDecoder::decodeEllipse(const QMap<QString, QVariant> &map) {
  const auto radius1 = map[QString("radius1")].toDouble();
  const auto radius2 = map[QString("radius2")].toDouble();
  const auto x = map[QString("x")].toDouble();
  const auto y = map[QString("y")].toDouble();

  return new Shape2DEllipse(QPointF(x, y), radius1, radius2);
}

Shape2D *shape
InstrumentWidgetDecoder::decodeRectangle(const QMap<QString, QVariant> &map) {
  const auto x0 = map[QString("x0")].toDouble();
  const auto y0 = map[QString("y0")].toDouble();
  const auto x1 = map[QString("x1")].toDouble();
  const auto y1 = map[QString("y1")].toDouble();

  const QPointF point1(x0, y0);
  const QPointF point2(x1, y1);
  return new Shape2DRectangle(point1, point2);
}

Shape2D *shape
InstrumentWidgetDecoder::decodeRing(const QMap<QString, QVariant> &map) {
  const auto xWidth = map[QString("xWidth")].toDouble();
  const auto yWidth = map[QString("yWidth")].toDouble();

  auto baseShape = this->decodeShape(map[QString("shape")]);
  return new Shape2DRing(baseShape, xWidth, yWidth);
}

Shape2D *shape
InstrumentWidgetDecoder::decodeFree(const QMap<QString, QVariant> &map) {
  QPolygonF polygon;

  for (const auto param : map[QString("paramaters")].toList()) {
    double x = param[0];
    double y = param[1];

    polygon << QPointF(x, y);
  }

  return new Shape2DFree(polygon);
}

void InstrumentWidgetDecoder::decodeAlignmentInfo(const QList<QVariant> &list,
                                                  const InstrumentWidget &obj) {

  std::vector<std::pair<Mantid::Kernel::V3D, QPointF>> alignmentPlane;
  for (const auto &item : list) {
    const auto qLabMap = item[0];
    const auto marker = item[1];
    Mantid::Kernel::V3D qValue(qLabMap[QString("x")].toDouble(),
                               qLabMap[QString("y")].toDouble(),
                               qLabMap[QString("z")].toDouble());

    alignmentPlane.push_back(std::make_pair(qValue, marker));
  }
  obj->m_selectedAlignmentPlane = alignmentPlane;
}