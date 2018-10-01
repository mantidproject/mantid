/***************************************************************************
    File                 : CanvasPicker.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Canvas picker

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "Plot.h"
#include <QObject>

class Graph;
class ArrowMarker;

/**
 * \brief Handles parts of the user interaction for a Plot by registering itself
 *as an event filter for its QwtPlotCanvas.
 *
 * CanvasPicker relies heavily on its parent being the Graph that owns the Plot
 *it operates on.
 * Additionally, parts of the code use Graph::plotWidget instead of
 *CanvasPicker::plotWidget.
 */
class CanvasPicker : public QObject {
  Q_OBJECT
public:
  explicit CanvasPicker(Graph *plot);
  bool eventFilter(QObject *, QEvent *) override;
  void selectPoints(int n);
  void selectPeak(const QPoint &p);

  //! Disable editing of #d_editing_marker on a TAB-key event.
  /**\brief Called by Graph::focusNextPrevChild ()
   */
  void disableEditing();

private:
  void drawTextMarker(const QPoint &);
  void drawLineMarker(const QPoint &, bool endArrow);

  //! Selects and highlights the marker at the given position.
  /**
   * @return whether a marker was found at #pointSelected
   */
  bool selectMarker(const QMouseEvent *e);

  /**
   * \brief Return my parent as a Graph.
   *
   * %Note that contrary to the method name, this does NOT return the Plot I
   *operate on.
   */
  Graph *plot();

  /**
   * \brief The Plot I handle user interaction for.
   *
   * %Note that this has to be owned by my parent Graph.
   */
  Plot *plotWidget;
  QPoint startLinePoint, endLinePoint;

signals:
  void showPlotDialog(int);
  void viewLineDialog();
  void viewImageDialog();
  void drawTextOff();
  void showMarkerPopupMenu();
  void modified();
  void selectPlot();

private:
  bool pointSelected;
  /**\brief The marker that is currently being edited, or NULL.
   * Editing does explicitly _not_ include moving and resizing, which are being
   * handled by SelectionMoveResizer (see Graph::d_markers_selector).
   * Currently, only ArrowMarker provides any other form of editing, but this
   * really
   * should be generalized. See ImageMarker for details.
   */
  ArrowMarker *d_editing_marker;
};
