// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : ScalePicker.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include <QObject>

class QRect;
class QPoint;
class QwtPlot;
class QwtScaleWidget;

/**\brief Handles user interaction with a QwtScaleWidget.
 *
 * This class is used by Graph to catch events for the scales on its Plot.
 * ScalePicker doesn't take any actions beyond emitting signals, which are then
 *processed by Graph.
 */
class ScalePicker : public QObject {
  Q_OBJECT
public:
  explicit ScalePicker(QwtPlot *plot);

  //! Returns the bounding rectangle of a scale without the title.
  QRect scaleRect(const QwtScaleWidget *) const;

  //! Returns the bounding rectangle of a scale excluding the title and the tick
  // labels.
  QRect scaleTicksRect(const QwtScaleWidget *scale) const;

  //! Returns the bounding rectangle of a scale's title.
  QRect titleRect(const QwtScaleWidget *scale) const;

  /** Install myself as event filter for all axes of my parent.
   * For every axis of plot(), add myself to the corresponding QwtScaleWidget.
   * \sa QwtPlot::axisWidget()
   */
  void refresh();

  //! Return my parent casted to QwtPlot.
  QwtPlot *plot();

  void deselect();

  bool titleSelected() { return d_title_selected; };
  void selectTitle(QwtScaleWidget *scale, bool select = true);

  bool labelsSelected() { return d_labels_selected; };
  void selectLabels(QwtScaleWidget *scale, bool select = true);

  /** Returns a pointer to the selected axis in the plot layer.
   * The selected axis has selected title or selected tick labels (blue frame
   * around texts).
   */
  QwtScaleWidget *selectedAxis() { return d_selected_axis; };
  //! Returns a pointer to the active axis in the plot layer.
  QwtScaleWidget *currentAxis() { return d_current_axis; };

signals:
  //! Emitted when the user clicks on one of the monitored axes.
  void clicked();

  /** Emitted when the user right-clicks on an axis (but not its title).
   * The argument specifies the axis' QwtScaleDraw::Alignment.
   */
  void axisRightClicked(int);
  //! Emitted when the user right-clicks on the title of an axis.
  void axisTitleRightClicked();

  /** Emitted when the user double-clicks on an axis (but not its title).
   * The argument specifies the axis' QwtScaleDraw::Alignment.
   */
  void axisDblClicked(int);

  /** Emitted when the user double-clicks on an the axis title.
   */
  void axisTitleDblClicked();

private:
  bool eventFilter(QObject *, QEvent *) override;

  void mouseDblClicked(const QwtScaleWidget *, const QPoint &);
  void mouseClicked(const QwtScaleWidget *scale, const QPoint &pos);
  void mouseRightClicked(const QwtScaleWidget *scale, const QPoint &pos);

  bool d_title_selected;
  bool d_labels_selected;
  QwtScaleWidget *d_selected_axis, *d_current_axis;
};
