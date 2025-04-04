// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <utility>

#include "MantidGeometry/Crystal/IPeak.h"
#include "Shape2D.h"

namespace MantidQt {
namespace MantidWidgets {

class PeakOverlay;

/**
 * Shape representing a peak marker on un unwrapped surface.
 * A marker consists of a symbol marking location of a peak
 * and a text label.
 */
class PeakMarker2D : public Shape2D {
public:
  enum Symbol { Circle = 0, Diamond, Square };
  struct Style {
    Style(Symbol sb = Circle, const QColor &c = Qt::red, int sz = g_defaultMarkerSize)
        : symbol(sb), color(std::move(c)), size(sz) {}
    Symbol symbol;
    QColor color;
    int size;
  };
  PeakMarker2D(PeakOverlay &peakOverlay, double u, double v, const Style &style = Style());
  /* --- Implemented Shape2D virtual methods --- */
  Shape2D *clone() const override { return new PeakMarker2D(*this); }
  bool selectAt(const QPointF &p) const override;
  bool contains(const QPointF &p) const override { return m_boundingRect.contains(p); }
  void addToPath(QPainterPath &path) const override;
  /* --- Own public methods --- */
  /// Set new marker size to s
  void setMarkerSize(const int &s);
  /// Get marker size
  int getMarkerSize() const { return m_markerSize; }
  /// Get default marker size
  static int getDefaultMarkerSize() { return g_defaultMarkerSize; }
  Symbol getSymbol() const { return m_symbol; }
  void setSymbol(Symbol s) { m_symbol = s; }
  Style getStyle() const;
  void setPeak(const Mantid::Geometry::IPeak &peak, int row = -1);
  const Mantid::Geometry::IPeak &getPeak() const;
  double getH() const { return m_h; }
  double getK() const { return m_k; }
  double getL() const { return m_l; }
  int getDetectorID() const { return m_detID; }
  int getRow() const { return m_row; }
  void setRow(int row) { m_row = row; }
  /// Get label's area on the screen
  const QRectF &getLabelRect() const { return m_labelRect; }
  /// Allows PeakOverlay to move the label to avoid overlapping
  void moveLabelRectTo(const QPointF &p) const { m_labelRect.moveTo(p); }
  const QString &getLabel() const { return m_label; }

protected:
  /* --- Implemented Shape2D protected virtual methods --- */
  void drawShape(QPainter &painter) const override;
  void refit() override {}
  /* --- Own protected methods --- */
  void drawCircle(QPainter &painter) const;
  void drawDiamond(QPainter &painter) const;
  void drawSquare(QPainter &painter) const;

private:
  PeakOverlay &m_peakOverlay; ///< Parent PeakOverlay
  int m_markerSize;           ///< Size of the marker
  static const int g_defaultMarkerSize;
  Symbol m_symbol;            ///< Shape of the marker
  double m_h, m_k, m_l;       ///< Peak's h,k,l
  int m_detID;                ///< Peak's detector ID
  QString m_label;            ///< Label string
  mutable QRectF m_labelRect; ///< label's area on the screen
  int m_row;                  ///< peaks row number in PeaksWorkspace
};

} // namespace MantidWidgets
} // namespace MantidQt
