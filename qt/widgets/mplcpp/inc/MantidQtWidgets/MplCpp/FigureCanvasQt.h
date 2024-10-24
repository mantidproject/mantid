// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Figure.h"

#include <QPointF>
#include <QWidget>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief FigureCanvasQt defines a QWidget that can be embedded within
 * another widget to display a matplotlib figure. It roughly follows
 * the matplotlib example on embedding a matplotlib canvas:
 * https://matplotlib.org/examples/user_interfaces/embedding_in_qt5.html
 */
class MANTID_MPLCPP_DLL FigureCanvasQt : public QWidget, public Common::Python::InstanceHolder {
  Q_OBJECT
public:
  FigureCanvasQt(const int subplotspec, const QString &projection = "", QWidget *parent = nullptr);
  FigureCanvasQt(Figure fig, QWidget *parent = nullptr);

  /// Attach an event filter to the underlying matplotlib canvas
  void installEventFilterToMplCanvas(QObject *filter);
  /// Access to the current figure instance.
  inline Figure gcf() const { return m_figure; }

  /// Access to the current active axes instance.
  template <typename AxesType = Axes> inline AxesType gca() const { return m_figure.gca<AxesType>(); }

  void setTightLayout(QHash<QString, QVariant> const &args);

  /// Convert a point in screen coordinates to data coordinates
  QPointF toDataCoords(QPoint pos) const;

  /// Redraw the canvas
  inline void draw() {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    pyobj().attr("draw")();
  }
  /// Redraw the canvas if nothing else is happening
  inline void drawIdle() {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    pyobj().attr("draw_idle")();
  }

private: // members
  Figure m_figure;
  // A pointer to the C++ widget extract from the Python FigureCanvasQT object
  QWidget *m_mplCanvas;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
