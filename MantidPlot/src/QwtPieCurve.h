// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2004 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : QwtPieCurve.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "LegendWidget.h"
#include "PlotCurve.h"
#include <qwt_plot.h>

class PieLabel;

//! Pie plot class
class QwtPieCurve : public DataCurve {
public:
  QwtPieCurve(Table *t, const QString &name, int startRow, int endRow);

  using DataCurve::clone; // Unhide base class method (avoids Intel compiler
                          // warning)
  void clone(QwtPieCurve *c);

  double viewAngle() { return d_view_angle; };
  void setViewAngle(double a) { d_view_angle = a; };

  double thickness() { return d_thickness; };
  void setThickness(double t) { d_thickness = t; };

  double horizontalOffset() { return d_horizontal_offset; };
  void setHorizontalOffset(double d) { d_horizontal_offset = d; };

  bool counterClockwise() { return d_counter_clockwise; };
  void setCounterClockwise(bool on) { d_counter_clockwise = on; };

  double startAzimuth() { return d_start_azimuth; };
  void setStartAzimuth(double angle) { d_start_azimuth = angle; };

  double labelsEdgeDistance() { return d_edge_dist; };
  void setLabelsEdgeDistance(double d) { d_edge_dist = d; };

  bool labelsAutoFormat() { return d_auto_labeling; };
  void setLabelsAutoFormat(bool on) { d_auto_labeling = on; };

  bool labelsValuesFormat() { return d_values; };
  void setLabelValuesFormat(bool on) { d_values = on; };

  bool labelsPercentagesFormat() { return d_percentages; };
  void setLabelPercentagesFormat(bool on) { d_percentages = on; };

  bool labelCategories() { return d_categories; };
  void setLabelCategories(bool on) { d_categories = on; };

  bool fixedLabelsPosition() { return d_fixed_labels_pos; };
  void setFixedLabelsPosition(bool on) { d_fixed_labels_pos = on; };

  QColor color(int i) const;

  int radius() { return d_pie_ray; };
  void setRadius(int size) { d_pie_ray = size; };

  Qt::BrushStyle pattern() { return QwtPlotCurve::brush().style(); };
  void setBrushStyle(const Qt::BrushStyle &style);

  void setFirstColor(int index) { d_first_color = index; };
  int firstColor() { return d_first_color; };

  void loadData() override;
  void initLabels();

  void addLabel(PieLabel *l, bool clone = false);
  void removeLabel(PieLabel *l);

  QList<PieLabel *> labelsList() { return d_texts_list; };

private:
  using DataCurve::draw; // Unhide base class method (avoids Intel compiler
                         // warning)
  void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            int from, int to) const override;
  void drawSlices(QPainter *painter, const QwtScaleMap &xMap,
                  const QwtScaleMap &yMap, int from, int to) const;
  void drawDisk(QPainter *painter, const QwtScaleMap &xMap,
                const QwtScaleMap &yMap) const;

  int d_pie_ray;
  int d_first_color;
  double d_start_azimuth;
  double d_view_angle;
  double d_thickness;
  double d_horizontal_offset;
  double d_edge_dist;
  bool d_counter_clockwise;
  bool d_auto_labeling;
  bool d_values;
  bool d_percentages;
  bool d_categories;
  bool d_fixed_labels_pos;
  QList<PieLabel *> d_texts_list;
  //! Stores table row indices to be displayed in PieLabels if d_categories is
  // true.
  QVarLengthArray<int> d_table_rows;
};

class PieLabel : public LegendWidget {
  Q_OBJECT

public:
  PieLabel(Plot *, QwtPieCurve *pie = nullptr);

  QString customText();
  void setCustomText(const QString &s) { d_custom_text = s; };

  void setPieCurve(QwtPieCurve *pie) { d_pie_curve = pie; };

private:
  void closeEvent(QCloseEvent *e) override;

  QwtPieCurve *d_pie_curve;
  QString d_custom_text;
};
