// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_LINEOVERLAY_H_
#define MANTID_SLICEVIEWER_LINEOVERLAY_H_

#include "DllOption.h"
#include "MantidKernel/System.h"
#include <QPainter>
#include <QWidget>
#include <qwt_plot.h>

namespace MantidQt {
namespace SliceViewer {

/** GUI for overlaying a line with a width onto the plot
  in the SliceViewer. Should be generic to overlays on any QwtPlot.
  Drag/droppable.

  @date 2011-11-14
*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER LineOverlay : public QWidget {
  Q_OBJECT

  /// Enum giving IDs to the 4 handles on the widget
  enum eHandleID {
    HandleNone = -1,
    HandleA = 0,
    HandleB = 1,
    HandleWidthTop = 2,
    HandleWidthBottom = 3,
    HandleCenter = 4 // Anywhere inside the center
  };

public:
  LineOverlay(QwtPlot *plot, QWidget *parent);
  ~LineOverlay() override;

  void reset();

  void setPointA(QPointF pointA);
  void setPointB(QPointF pointB);
  void setWidth(double width);

  const QPointF &getPointA() const;
  const QPointF &getPointB() const;
  double getWidth() const;

  void setSnapX(double spacing);
  void setSnapY(double spacing);
  void setSnap(double spacing);
  void setSnapEnabled(bool enabled);
  void setSnapLength(double spacing);
  void setShown(bool shown);
  void setShowHandles(bool shown);
  void setShowLine(bool shown);
  void setCreationMode(bool creation);
  void setAngleSnapMode(bool angleSnap);
  void setAngleSnap(double snapDegrees);

  ///@return whether the overlay is shown
  bool isShown() const;

  ///@return the snap-to X interval
  double getSnapX() { return m_snapX; }

  ///@return the snap-to X interval
  double getSnapY() { return m_snapY; }

  ///@return true if the line is in creation mode (waiting for first click)
  bool getCreationMode() const { return m_creation; }
  /// Load the state of the line overlay from a Mantid project file
  void loadFromProject(const std::string &lines);
  /// Save the state of the line overlay to a Mantid project file
  std::string saveToProject() const;

signals:
  /// Signal sent while the line is being dragged
  void lineChanging(QPointF /*_t1*/, QPointF /*_t2*/, double /*_t3*/);
  /// Signal sent once the drag is completed
  void lineChanged(QPointF /*_t1*/, QPointF /*_t2*/, double /*_t3*/);

private:
  QPoint transform(QPointF coords) const;
  QPointF invTransform(QPoint pixels) const;
  QPointF snap(QPointF original) const;

  QSize sizeHint() const override;
  QSize size() const;
  int height() const;
  int width() const;

  QRect drawHandle(QPainter &painter, QPointF coords, QColor brush);
  void paintEvent(QPaintEvent *event) override;

  eHandleID mouseOverHandle(QPoint pos);
  bool mouseOverCenter(QPoint pos);
  void handleDrag(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

protected:
  /// Marker that we are just creating the line (with the mouse)
  bool m_creation;

  /// QwtPlot containing this
  QwtPlot *m_plot;

  /// First point of the line (in coordinates of the plot)
  QPointF m_pointA;
  /// Second point of the line (in coordinates of the plot)
  QPointF m_pointB;
  /// Width of the line (in coordinates of the plot)
  double m_width;
  /// Rects defining where the 4 handles are
  QVector<QRect> m_handles;

  /// When dragging, this is the handle being dragged
  eHandleID m_dragHandle;
  /// Start point (in plot coords) of the drag
  QPointF m_dragStart;
  /// Original PointA at drag start
  QPointF m_dragStart_PointA;
  /// Original PointB at drag start
  QPointF m_dragStart_PointB;

  /// Marker that the middle mouse button is pressed (panning)
  bool m_rightButton;

  /// Is snap-to-grid enabled?
  bool m_snapEnabled;
  /// Snap to grid spacing in X
  double m_snapX;
  /// Grid spacing in Y
  double m_snapY;
  /// Snap to length of the line
  double m_snapLength;

  /// Is any of the control visible?
  bool m_shown;

  /// Are the mouse handles visible?
  bool m_showHandles;

  /// Show the central line?
  bool m_showLine;

  /// If true, then you are in always-snap mode
  bool m_angleSnapMode;

  /// Angle (in degrees) to snap to.
  double m_angleSnap;
};

} // namespace SliceViewer
} // namespace MantidQt

#endif /* MANTID_SLICEVIEWER_LINEOVERLAY_H_ */
