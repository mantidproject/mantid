#ifndef MPLCPP_FIGURECANVASQT_H
#define MPLCPP_FIGURECANVASQT_H
/*
  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Figure.h"

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
class MANTID_MPLCPP_DLL FigureCanvasQt : public QWidget,
                                         public Python::InstanceHolder {
  Q_OBJECT
public:
  FigureCanvasQt(int subplotspec, QWidget *parent = nullptr);
  FigureCanvasQt(Figure fig, QWidget *parent = nullptr);

  void installEventFilterToMplCanvas(QObject *filter);

  /// Access to the current figure instance.
  inline Figure gcf() { return m_figure; }
  /// Access to the current active axes instance.
  inline Axes gca() { return m_figure.gca(); }

  /// Redraw the canvas
  inline void draw() { pyobj().attr("draw")(); }

private: // members
  Figure m_figure;
  // A pointer to the C++ widget extract from the Python FigureCanvasQT object
  QWidget *m_mplCanvas;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
#endif // MPLCPP_FIGURECANVASQT_H
