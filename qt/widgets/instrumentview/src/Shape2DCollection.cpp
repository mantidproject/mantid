// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/Shape2DCollection.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStringList>
#include <QWheelEvent>

#include <cmath>
#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {

Shape2DCollection::Shape2DCollection()
    : Shape2D(), m_wx(0), m_wy(0), m_h(0), m_currentShape(nullptr),
      m_currentCP(0), m_overridingCursor(false) {}

Shape2DCollection::~Shape2DCollection() {
  foreach (Shape2D *shape, m_shapes) { delete shape; }
}

/**
 * Draw the collection on screen.
 */
void Shape2DCollection::draw(QPainter &painter) const {
  if (m_shapes.isEmpty())
    return;

  // separate scalable and nonscalable shapes
  QList<Shape2D *> scalable;
  QList<Shape2D *> nonscalable;
  foreach (Shape2D *shape, m_shapes) {
    if (!shape->isVisible())
      continue;
    if (shape->isScalable()) {
      scalable << shape;
    } else {
      nonscalable << shape;
    }
  }

  // first draw the scalable ones
  painter.save();
  painter.setTransform(m_transform);
  foreach (const Shape2D *shape, scalable) {
    painter.save();
    shape->draw(painter);
    painter.restore();
  }
  painter.restore();

  // now the nonscalable
  foreach (const Shape2D *shape, nonscalable) {
    QPointF p0 = shape->origin();
    QPointF p1 = m_transform.map(p0);
    QPointF dp = p1 - p0;
    painter.save();
    painter.translate(dp);
    shape->draw(painter);
    painter.restore();
  }
  // std::cerr << m_transform.m11() << ' ' << m_transform.m22() << ' ' <<
  // m_transform.m33() << '\n';
}

/**
 * Add a new shape to collection.
 * @param shape :: A pointer to the new shape.
 * @param slct :: A bool flag to select the shape after it's added.
 */
void Shape2DCollection::addShape(Shape2D *shape, bool slct) {
  m_shapes.push_back(shape);
  m_boundingRect.unite(shape->getBoundingRect());
  if (slct) {
    addToSelection(shape);
  }
  emit shapeCreated();
}

/**
 * Remove a shape from collection
 * @param shape :: Pointer to the shape to remove.
 * @param sendSignal :: Flag to send shapesRemoved() signal.
 */
void Shape2DCollection::removeShape(Shape2D *shape, bool sendSignal) {
  if (shape && m_shapes.contains(shape)) {
    m_shapes.removeOne(shape);
    m_selectedShapes.removeOne(shape);
    delete shape;
  }
  if (sendSignal) {
    if (m_shapes.isEmpty()) {
      emit cleared();
    } else {
      emit shapesRemoved();
    }
  }
}

/**
 * Remove a list of shapes.
 * @param shapeList :: A list of pointers to the shapes to be removed
 */
void Shape2DCollection::removeShapes(const QList<Shape2D *> &shapeList) {
  foreach (Shape2D *shape, shapeList) {
    if (shape == m_currentShape) {
      m_currentShape = nullptr;
    }
    removeShape(shape, false);
  }
  if (m_shapes.isEmpty()) {
    emit cleared();
  } else {
    emit shapesRemoved();
  }
}

/**
 */
void Shape2DCollection::setWindow(const RectF &surface,
                                  const QRect &viewport) const {
  m_viewport = viewport;
  m_surfaceRect = surface;
  m_surfaceRect.findTransform(m_transform, viewport);
}

void Shape2DCollection::refit() {}

void Shape2DCollection::resetBoundingRect() {
  m_boundingRect = RectF();
  foreach (const Shape2D *shape, m_shapes) {
    m_boundingRect.unite(shape->getBoundingRect());
  }
}

void Shape2DCollection::keyPressEvent(QKeyEvent *e) {
  switch (e->key()) {
  case Qt::Key_Delete:
  case Qt::Key_Backspace:
    removeSelectedShapes();
    break;
  }
}

void Shape2DCollection::addShape(const QString &type, int x, int y,
                                 const QColor &borderColor,
                                 const QColor &fillColor) {
  deselectAll();
  Shape2D *shape = createShape(type, x, y);
  if (!shape) {
    emit shapeSelected();
    return;
  }
  shape->setColor(borderColor);
  shape->setFillColor(fillColor);
  addShape(shape);
  addToSelection(shape);
  m_currentCP = 2;
  emit shapeSelected();
}

Shape2D *Shape2DCollection::createShape(const QString &type, int x,
                                        int y) const {
  QPointF p = m_transform.inverted().map(QPointF(x, y));

  if (type.toLower() == "ellipse") {
    return new Shape2DEllipse(p, 0.0);
  } else if (type.toLower() == "rectangle") {
    return new Shape2DRectangle(p, QSizeF(0, 0));
  } else if (type.toLower() == "free") {
    return new Shape2DFree(p);
  }

  QStringList complexType = type.split(' ', QString::SkipEmptyParts);

  if (complexType.size() < 2)
    return nullptr;

  QString mainType = complexType[0];

  if (mainType.toLower() == "ring") {
    double xWidth = 10.0 / fabs(m_transform.m11());
    double yWidth = 10.0 / fabs(m_transform.m22());
    Shape2D *child = createShape(complexType[1], x, y);
    return new Shape2DRing(child, xWidth, yWidth);
  }

  throw std::invalid_argument("Shape " + type.toStdString() +
                              " cannot be created");
}

/**
 * Deselect all selected shapes.
 */
void Shape2DCollection::deselectAll() {
  foreach (Shape2D *shape, m_shapes) {
    shape->edit(false);
    shape->setSelected(false);
  }
  m_selectedShapes.clear();
  m_currentShape = nullptr;
  if (m_overridingCursor) {
    m_overridingCursor = false;
    QApplication::restoreOverrideCursor();
  }
  emit shapesDeselected();
}

/**
 * Resize the current shape by moving the right-bottom control point to a
 * loaction on the screen.
 */
void Shape2DCollection::moveRightBottomTo(int x, int y) {
  if (m_currentShape && m_currentShape->isEditing()) {
    QPointF p = m_transform.inverted().map(QPointF(x, y));
    m_currentShape->setControlPoint(3, p);
    emit shapeChanged();
  }
}

/**
 * Select a shape or a control point at a location on the screen.
 * The control points of the currently selected shape are checked first.
 * If (x,y) doesn't point to anything deselect all currently selected shapes.
 */
void Shape2DCollection::selectShapeOrControlPointAt(int x, int y) {
  if (isOverSelectionAt(x, y))
    return;
  bool ret = selectControlPointAt(x, y) || selectAtXY(x, y);
  if (!ret) {
    deselectAll();
  }
}

/**
 * Add a shape under the cursor to the selection.
 * @param x :: Mouse x coordinate.
 * @param y :: Mouse y coordinate.
 */
void Shape2DCollection::addToSelectionShapeAt(int x, int y) {
  // if there is a selected shape under the cursor deselect it
  if (isOverSelectionAt(x, y)) {
    deselectAtXY(x, y);
    return;
  }
  // try selecting a shape without editing it
  if (!selectAtXY(x, y, false)) {
    deselectAll();
  }
}

/**
 * Move the current control point or entire shape by (dx,dy).
 * @param dx :: Shift in the x direction in screen pixels.
 * @param dy :: Shift in the y direction in screen pixels.
 */
void Shape2DCollection::moveShapeOrControlPointBy(int dx, int dy) {
  if (!hasSelection())
    return;
  if (m_currentShape && m_currentCP < m_currentShape->getNControlPoints()) {
    QPointF p = m_currentShape->getControlPoint(m_currentCP);
    QPointF screenP = m_transform.map(p) + QPointF(dx, dy);
    p = m_transform.inverted().map(screenP);
    m_currentShape->setControlPoint(m_currentCP, p);
  } else {
    QPointF p0 = m_selectedShapes[0]->getControlPoint(0);
    QPointF screenP0 = m_transform.map(p0);
    QPointF screenP1 = screenP0 + QPointF(dx, dy);
    QPointF p1 = m_transform.inverted().map(screenP1);
    QPointF dp = p1 - p0;
    foreach (Shape2D *shape, m_selectedShapes) { shape->moveBy(dp); }
  }
  if (!m_overridingCursor) {
    m_overridingCursor = true;
    QApplication::setOverrideCursor(Qt::SizeAllCursor);
  }
  emit shapeChanged();
}

/**
 * If mouse pointer at (x,y) touches the current shape or its control points
 * override the cursor image.
 */
void Shape2DCollection::touchShapeOrControlPointAt(int x, int y) {
  if (selectControlPointAt(x, y) || isOverSelectionAt(x, y)) {
    if (!m_overridingCursor) {
      m_overridingCursor = true;
      QApplication::setOverrideCursor(Qt::SizeAllCursor);
    }
  } else if (m_overridingCursor) {
    deselectControlPoint();
    m_overridingCursor = false;
    QApplication::restoreOverrideCursor();
  }
}

/**
 * Select a shape which contains a point (x,y) of the screen.
 */
bool Shape2DCollection::selectAtXY(int x, int y, bool edit) {
  const auto point = m_transform.inverted().map(QPointF(x, y));
  return selectAtXY(point, edit);
}

/**
 * Select a shape which contains a point (x, y) of the world.
 */
bool Shape2DCollection::selectAtXY(const QPointF &point, bool edit) {
  if (edit) {
    // if shape has to be edited (resized) it must be the only selection
    deselectAll();
  }
  foreach (Shape2D *shape, m_shapes) {
    bool picked = shape->selectAt(point);
    if (picked) {
      addToSelection(shape);
      return true;
    }
  }
  return false;
}

/**
 * Deselect a shape under the cursor.
 * @param x :: Mouse x coordinate.
 * @param y :: Mouse y coordinate.
 */
void Shape2DCollection::deselectAtXY(int x, int y) {
  const QPointF p = m_transform.inverted().map(QPointF(x, y));
  deselectAtXY(p);
}

/**
 * Deselect a shape under the cursor.
 * @param point :: point where peaks should be deselected
 */
void Shape2DCollection::deselectAtXY(const QPointF &point) {
  foreach (Shape2D *shape, m_shapes) {
    bool picked = shape->selectAt(point);
    if (picked) {
      removeFromSelection(shape);
      return;
    }
  }
}

/**
 * Select all shapes included in a rectangle.
 * @param rect :: Rectangle in current screen coordinates containing selected
 * shapes.
 * @return :: True if any of the shapes is selected.
 */
bool Shape2DCollection::selectIn(const QRect &rect) {
  RectF untransformedRect = RectF(QRectF(rect));
  RectF r(m_transform.inverted().mapRect(QRectF(rect)));
  bool selected = false;
  deselectAll();
  foreach (Shape2D *shape, m_shapes) {
    bool sel = false;
    if (shape->isScalable()) {
      sel = r.contains(shape->getBoundingRect());
    } else {
      QPointF dp = m_transform.map(shape->origin()) - shape->origin();
      RectF br = shape->getBoundingRect();
      br.translate(dp);
      sel = untransformedRect.contains(br);
    }
    if (sel) {
      addToSelection(shape);
      selected = true;
    }
  }
  return selected;
}

/**
 * Select a shape with index i.
 */
void Shape2DCollection::addToSelection(int i) {
  if (i < static_cast<int>(size())) {
    addToSelection(m_shapes[i]);
  }
}

/**
 * Check if any of the shapes is selected.
 * @return :: True if there is a selection.
 */
bool Shape2DCollection::hasSelection() const {
  foreach (Shape2D *shape, m_shapes) {
    if (shape->isSelected()) {
      return true;
    }
  }
  return false;
}

/**
 * Add a shape to selection. If it's the only selection start editing it.
 * @param shape :: Pointer to a shape which is to become select.
 */
void Shape2DCollection::addToSelection(Shape2D *shape) {
  if (!m_selectedShapes.contains(shape)) {
    if (m_selectedShapes.size() == 1)
      finishEdit();
    shape->setSelected(true);
    m_selectedShapes.append(shape);
    if (m_selectedShapes.size() == 1)
      edit(shape);
  }
}

/**
 * Remove a shape from selection.
 * @param shape :: Pointer to a shape to deselect.
 */
void Shape2DCollection::removeFromSelection(Shape2D *shape) {
  foreach (Shape2D *s, m_selectedShapes) {
    if (s == shape) {
      shape->setSelected(false);
      shape->edit(false);
      m_selectedShapes.removeOne(shape);
      return;
    }
  }
}

/**
 * Start editing a shape.
 * @param shape :: A shape to edit.
 */
void Shape2DCollection::edit(Shape2D *shape) {
  if (m_currentShape) {
    m_currentShape->edit(false);
  }
  m_currentShape = shape;
  m_currentShape->edit(true);
  m_currentCP = m_currentShape->getNControlPoints(); // no current cp until it
                                                     // is selected explicitly
  emit shapeSelected();
}

/**
 * Finish editing the current shape. The shape remains selected.
 */
void Shape2DCollection::finishEdit() {
  if (m_currentShape) {
    m_currentShape->edit(false);
    m_currentShape = nullptr;
  }
}

/**
 * Checks if the screen point (x,y) is inside the current shape.
 */
bool Shape2DCollection::isOverCurrentAt(int x, int y) {
  if (!m_currentShape)
    return false;
  QPointF p = m_transform.inverted().map(QPointF(x, y));
  return m_currentShape->selectAt(p);
}

/**
 * Checks if the screen point (x,y) is inside any of the selected shapes.
 */
bool Shape2DCollection::isOverSelectionAt(int x, int y) {
  if (m_selectedShapes.isEmpty())
    return false;
  QPointF p = m_transform.inverted().map(QPointF(x, y));
  foreach (Shape2D *shape, m_selectedShapes) {
    if (shape->selectAt(p))
      return true;
  }
  return false;
}

bool Shape2DCollection::selectControlPointAt(int x, int y) {
  // QPointF p = m_transform.inverted().map(QPointF(x,y));
  QPointF p = QPointF(x, y);
  if (!m_currentShape)
    return false;
  for (size_t i = 0; i < m_currentShape->getNControlPoints(); ++i) {
    // QPointF cp = m_currentShape->getControlPoint(i) - p;
    QPointF cp = m_transform.map(m_currentShape->getControlPoint(i)) - p;
    if (fabs(cp.x()) + fabs(cp.y()) <= sizeCP + 2)
    // if (cp.manhattanLength() <= sizeCP + 2)
    {
      m_currentCP = i;
      return true;
    }
  }
  // deselect control points
  m_currentCP = m_currentShape->getNControlPoints();
  return false;
}

void Shape2DCollection::deselectControlPoint() {
  if (m_currentShape) {
    m_currentCP = m_currentShape->getNControlPoints();
  }
}

void Shape2DCollection::removeCurrentShape() {
  if (m_currentShape) {
    this->removeShape(m_currentShape);
    m_currentShape = nullptr;
    emit shapesDeselected();
  }
}

/**
 * Removes the selected shapes from this collection.
 */
void Shape2DCollection::removeSelectedShapes() {
  auto shapeList = getSelectedShapes();
  if (!shapeList.isEmpty()) {
    removeShapes(shapeList);
    emit shapesDeselected();
  }
}

/**
 * Restore the cursor image to default.
 */
void Shape2DCollection::restoreOverrideCursor() {
  if (m_overridingCursor) {
    m_overridingCursor = false;
    QApplication::restoreOverrideCursor();
  }
}

void Shape2DCollection::clear() {
  foreach (Shape2D *shape, m_shapes) { delete shape; }
  m_shapes.clear();
  m_selectedShapes.clear();
  m_currentShape = nullptr;
  emit shapesDeselected();
}

QStringList Shape2DCollection::getCurrentDoubleNames() const {
  if (m_currentShape) {
    return m_currentShape->getDoubleNames();
  }
  return QStringList();
}

double Shape2DCollection::getCurrentDouble(const QString &prop) const {
  if (m_currentShape) {
    return m_currentShape->getDouble(prop);
  }
  return 0.0;
}

void Shape2DCollection::setCurrentDouble(const QString &prop, double value) {
  if (m_currentShape) {
    return m_currentShape->setDouble(prop, value);
  }
}

QStringList Shape2DCollection::getCurrentPointNames() const {
  if (m_currentShape) {
    return m_currentShape->getPointNames();
  }
  return QStringList();
}

QPointF Shape2DCollection::getCurrentPoint(const QString &prop) const {
  if (m_currentShape) {
    return m_currentShape->getPoint(prop);
  }
  return QPointF();
}

void Shape2DCollection::setCurrentPoint(const QString &prop,
                                        const QPointF &value) {
  if (m_currentShape) {
    return m_currentShape->setPoint(prop, value);
  }
}

RectF Shape2DCollection::getCurrentBoundingRect() const {
  if (m_currentShape) {
    return m_currentShape->getBoundingRect();
  }
  return RectF();
}

void Shape2DCollection::setCurrentBoundingRect(const RectF &rect) {
  if (m_currentShape) {
    m_currentShape->setBoundingRect(rect);
  }
}

bool Shape2DCollection::isMasked(double x, double y) const {
  QPointF p(x, y);
  foreach (Shape2D *shape, m_shapes) {
    if (shape->isMasked(p)) {
      return true;
    }
  }
  return false;
}

QList<QPoint> Shape2DCollection::getMaskedPixels() const {
  QList<QPoint> pixels;
  QTransform inv = m_transform.inverted();
  for (int i = m_viewport.left(); i <= m_viewport.right(); ++i) {
    for (int j = m_viewport.top(); j <= m_viewport.bottom(); ++j) {
      QPoint p = QPoint(i, j);
      QPointF p0 = inv.map(QPointF(p));
      foreach (Shape2D *shape, m_shapes) {
        if (shape->isMasked(p0)) {
          pixels.append(p);
        }
      }
    }
  }
  return pixels;
}

/**
 * Set the bounding rect of the current shape in real coordinates.
 */
void Shape2DCollection::setCurrentBoundingRectReal(const QRectF &rect) {
  if (!m_currentShape)
    return;
  m_currentShape->setBoundingRect(RectF(rect));
}

/**
 * Change border color.
 */
void Shape2DCollection::changeBorderColor(const QColor &color) {
  foreach (Shape2D *shape, m_shapes) { shape->setColor(color); }
}

/**
 * Save this shape collection to a table workspace
 *
 * This will create a table workspace called MaskShapes with one column for the
 * index of the shape and one containing the serialised parameters of the shape
 */
void Shape2DCollection::saveToTableWorkspace() {
  using namespace Mantid::API;
  auto table = WorkspaceFactory::Instance().createTable();
  table->addColumn("str", "Index");
  table->addColumn("str", "Parameters");

  size_t count = 0;
  for (auto shape : m_shapes) {
    auto shapeStr = shape->saveToProject();
    TableRow row = table->appendRow();
    row << std::to_string(count) << shapeStr;
    ++count;
  }

  AnalysisDataService::Instance().addOrReplace("MaskShapes", table);
}

/**
 * Load a collection of shapes from a table workspace
 *
 * This expects a table workspace with a column called parameters from which to
 * load collection of shapes from.
 *
 * @param ws :: table workspace to load shapes from.
 */
void Shape2DCollection::loadFromTableWorkspace(
    Mantid::API::ITableWorkspace_const_sptr ws) {
  using namespace Mantid::API;
  auto columnNames = ws->getColumnNames();

  // Check if the column exists
  if (std::find(columnNames.cbegin(), columnNames.cend(), "Parameters") ==
      columnNames.cend())
    return;

  ConstColumnVector<std::string> col = ws->getVector("Parameters");
  for (size_t i = 0; i < ws->rowCount(); ++i) {
    const auto params = col[i];
    auto shape = Shape2D::loadFromProject(params);
    if (shape != nullptr) {
      m_shapes.append(shape);
    }
  }
  emit shapeCreated();
}

/**
 * Add a Shape2D object allowing free drawing.
 * @param poly :: Initial shape.
 * @param borderColor :: The border colour.
 * @param fillColor :: The fill colour.
 */
void Shape2DCollection::addFreeShape(const QPolygonF &poly,
                                     const QColor &borderColor,
                                     const QColor &fillColor) {
  auto freeShape = dynamic_cast<Shape2DFree *>(m_currentShape);
  if (!freeShape) {
    if (poly.isEmpty())
      throw std::logic_error("Cannot create a shape from empty polygon.");
    auto p = m_transform.inverted().map(poly[0]);
    addShape("free", static_cast<int>(p.x()), static_cast<int>(p.y()),
             borderColor, fillColor);
  }
  drawFree(poly);
}

/**
 * Draw the shape by adding a polygon to it.
 */
void Shape2DCollection::drawFree(const QPolygonF &polygon) {
  auto freeShape = dynamic_cast<Shape2DFree *>(m_currentShape);
  if (freeShape) {
    auto transform = m_transform.inverted();
    freeShape->addPolygon(transform.map(polygon));
    emit shapeChanged();
  }
}

/**
 * Erase part of the shape by subtracting a polygon from it.
 */
void Shape2DCollection::eraseFree(const QPolygonF &polygon) {
  auto freeShape = dynamic_cast<Shape2DFree *>(m_currentShape);
  if (freeShape) {
    auto transform = m_transform.inverted();
    freeShape->subtractPolygon(transform.map(polygon));
    emit shapeChanged();
  }
}

/** Load shape 2D collection state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 */
void Shape2DCollection::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);
  for (const auto &shapeLines : tsv.sections("shape")) {
    Shape2D *shape = Shape2D::loadFromProject(shapeLines);
    m_shapes.push_back(shape);
    emit shapeCreated();
  }
}

/** Save the state of the shape 2D collection to a Mantid project file
 * @return a string representing the state of the shape 2D collection
 */
std::string Shape2DCollection::saveToProject() const {
  API::TSVSerialiser tsv;
  for (auto shape : m_shapes) {
    tsv.writeSection("shape", shape->saveToProject());
  }
  return tsv.outputLines();
}

} // namespace MantidWidgets
} // namespace MantidQt
