#ifndef MANTID_MANTIDWIDGETS_INPUTCONTROLLER_H
#define MANTID_MANTIDWIDGETS_INPUTCONTROLLER_H

#include "DllOption.h"
#include <QColor>
#include <QObject>
#include <QPolygonF>
#include <QRect>

class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QPainter;
class QPixmap;

namespace MantidQt {
namespace MantidWidgets {

/**
    The base class for the mouse and keyboard controllers to work
    with ProjectionSurfaces. Surfaces can be in different interaction
    modes and the same mode on different surfaces can involve
    different inputs.

    A projection surface keeps a list of controllers: one per interaction mode.
    The current controller emits signals which are connected to the relevant
   slots
    on the surface.

  */
class EXPORT_OPT_MANTIDQT_COMMON InputController : public QObject {
  Q_OBJECT
public:
  explicit InputController(QObject *parent, bool contextAllowed = true);
  ~InputController() override {}

  virtual void mousePressEvent(QMouseEvent *) {}
  virtual void mouseMoveEvent(QMouseEvent *) {}
  virtual void mouseReleaseEvent(QMouseEvent *) {}
  virtual void wheelEvent(QWheelEvent *) {}
  virtual void keyPressEvent(QKeyEvent *) {}
  virtual void enterEvent(QEvent *) {}
  virtual void leaveEvent(QEvent *) {}
  /// To be called after the owner widget has drawn its content
  virtual void onPaint(QPainter &) {}
  /// To be called when this controller takes control of the input. By default
  /// emits enabled() signal.
  virtual void onEnabled() { emit enabled(); }
  /// To be called when this controller looses control. By default emits
  /// disabled() signal.
  virtual void onDisabled() { emit disabled(); }

  /// Returns true if a surface using this controller can show
  /// a context menu on right-click
  bool canShowContextMenu() const { return m_canShowContextMenu; }

signals:
  void enabled();
  void disabled();

private:
  bool m_canShowContextMenu;
};

/**
    Controller for moving the instrument on Projection3D surface:
    translation, rotation and zooming.

  */
class EXPORT_OPT_MANTIDQT_COMMON InputController3DMove
    : public InputController {
  Q_OBJECT

public:
  InputController3DMove(QObject *parent);
  void mousePressEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void wheelEvent(QWheelEvent *) override;

signals:
  /// Init zooming. x and y is the zoom starting point on the screen.
  void initZoom(int x, int y);
  /// Init rotation. x and y is the starting point on the screen.
  void initRotation(int x, int y);
  /// Init translation. x and y is the starting point on the screen.
  void initTranslation(int x, int y);
  /// Zoom
  void zoom(int x, int y);
  /// Wheel zoom
  void wheelZoom(int x, int y, int d);
  /// Rotate
  void rotate(int x, int y);
  /// Translate
  void translate(int x, int y);
  /// Finish movement
  void finish();

private:
  bool m_isButtonPressed;
};

/**
    Controller for picking detectors.
  */
class EXPORT_OPT_MANTIDQT_COMMON InputControllerPick : public InputController {
  Q_OBJECT

public:
  InputControllerPick(QObject *parent);
  void mousePressEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;

signals:
  void pickPointAt(int, int);
  void touchPointAt(int, int);
  void setSelection(const QRect &);
  void finishSelection();

private:
  bool m_isButtonPressed;
  QRect m_rect;
};

/**
    Controller for drawing mask shapes.
  */
class EXPORT_OPT_MANTIDQT_COMMON InputControllerDrawShape
    : public InputController {
  Q_OBJECT

public:
  InputControllerDrawShape(QObject *parent);
  void mousePressEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void keyPressEvent(QKeyEvent *) override;
  void leaveEvent(QEvent *) override;

signals:
  /// Deselect all selected shapes
  void deselectAll();
  /// Add a new shape
  void addShape(const QString &type, int x, int y, const QColor &borderColor,
                const QColor &fillColor);
  /// Resize the current shape by moving the right-bottom control point to a
  /// location on the screen
  void moveRightBottomTo(int, int);
  /// Select a shape or a conrol point at a location on the screen.
  void selectAt(int, int);
  /// Select a shape with ctrl key pressed at a location on the screen.
  void selectCtrlAt(int, int);
  /// Move selected shape or a control point by a displacement vector.
  void moveBy(int, int);
  /// Sent when the mouse is moved to a new position with the buttons up
  void touchPointAt(int, int);
  /// Remove the selected shapes
  void removeSelectedShapes();
  /// Restore the cursor to its default image
  void restoreOverrideCursor();
  /// Update the rubber band selection
  void setSelection(const QRect &);
  /// Rubber band selection is done
  void finishSelection(const QRect &);

public slots:
  void startCreatingShape2D(const QString &type, const QColor &borderColor,
                            const QColor &fillColor);
  void onDisabled() override;

private:
  bool m_creating; ///< a shape is being created with a mouse
  int m_x, m_y;
  QString m_shapeType;
  QColor m_borderColor, m_fillColor;
  bool m_isButtonPressed;
  QRect m_rect;
};

/**
    Controller for moving the instrument on an unwrapped surface.
  */
class EXPORT_OPT_MANTIDQT_COMMON InputControllerMoveUnwrapped
    : public InputController {
  Q_OBJECT

public:
  InputControllerMoveUnwrapped(QObject *parent);
  void mousePressEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;

signals:
  void setSelectionRect(const QRect &);
  void zoom();
  void resetZoom();
  void unzoom();

private:
  bool m_isButtonPressed;
  QRect m_rect;
};

/**
    Controller for free drawing on an unwrapped surface.
  */
class EXPORT_OPT_MANTIDQT_COMMON InputControllerDraw : public InputController {
  Q_OBJECT

public:
  InputControllerDraw(QObject *parent);
  ~InputControllerDraw() override;
  void mousePressEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void wheelEvent(QWheelEvent *) override;

  void enterEvent(QEvent *) override;
  void leaveEvent(QEvent *) override;

protected:
  int cursorSize() const { return m_size; }
  bool isLeftButtonPressed() const { return m_isLeftButtonPressed; }
  bool isRightButtonPressed() const { return m_isRightButtonPressed; }
  bool isActive() const { return m_isActive; }

private:
  void redrawCursor();
  virtual void signalLeftClick() = 0;
  virtual void signalRightClick();
  virtual void drawCursor(QPixmap *cursor) = 0;
  virtual void setPosition(const QPoint &pos) = 0;
  virtual void resize() = 0;

  const int m_max_size;
  int m_size; ///< Size of the cursor
  bool m_isLeftButtonPressed;
  bool m_isRightButtonPressed;
  bool m_isActive;
  QPixmap *m_cursor;
};

/**
    Controller for erasing peaks on an unwrapped surface.
  */
class EXPORT_OPT_MANTIDQT_COMMON InputControllerSelection
    : public InputControllerDraw {
  Q_OBJECT

public:
  InputControllerSelection(QObject *parent, QPixmap *icon);
  ~InputControllerSelection() override;
  void onPaint(QPainter &) override;

signals:
  void selection(const QRect &);

private:
  void drawCursor(QPixmap *cursor) override;
  void setPosition(const QPoint &pos) override;
  void resize() override;
  void signalLeftClick() override;

  QPixmap *m_image;
  QRect m_rect;
};

/**
    Controller for drawing and erasing arbitrary shapes on an unwrapped surface.
  */
class EXPORT_OPT_MANTIDQT_COMMON InputControllerDrawAndErase
    : public InputControllerDraw {
  Q_OBJECT

public:
  InputControllerDrawAndErase(QObject *parent);

signals:
  void draw(const QPolygonF &);
  void erase(const QPolygonF &);
  void addShape(const QPolygonF &poly, const QColor &borderColor,
                const QColor &fillColor);

public slots:
  void startCreatingShape2D(const QColor &borderColor, const QColor &fillColor);

private:
  void drawCursor(QPixmap *cursor) override;
  void signalLeftClick() override;
  void signalRightClick() override;
  void setPosition(const QPoint &pos) override;
  void resize() override;
  void makePolygon();

  QPoint m_pos;
  QPolygonF m_rect;
  QColor m_borderColor, m_fillColor;
  bool m_creating;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTID_MANTIDWIDGETS_INPUTCONTROLLER_H
