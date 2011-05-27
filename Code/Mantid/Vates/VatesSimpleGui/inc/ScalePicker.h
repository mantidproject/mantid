#ifndef SCALEPICKER_H_
#define SCALEPICKER_H_

#include <qobject.h>

class QEvent;
class QPoint;
class QRect;
class QwtScaleWidget;
/**
 *
 This class handles finding the axis location when a slice indicator is
 created. It also handles the reverse operation of providing the GraphicsView
 coordinate for a given axis location.

 @author Michael Reuter
 @date 24/05/2011

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class ScalePicker: public QObject
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param scale the scale widget from qwt to associate with the picker
   */
  ScalePicker(QwtScaleWidget *scale = 0);
  /// Default destructor.
  virtual ~ScalePicker()
  {
  }
  /**
   * Event filter to catch mouse events from the QwtScaleWidget. This calls
   * the axis location finder.
   * @param object the QObject associated with the event
   * @param e the generated QEvent
   */
  virtual bool eventFilter(QObject *object, QEvent *e);
  /**
   * Function to convert the axis location to a GraphicsView coordinate.
   * @param axisval the axis value to convert
   * @return the x,y coordinates for the corresponding GraphicsView point
   */
  QPoint *getLocation(double axisval);

  signals:
  /**
   * Signal emitted when the scale picker is invoked.
   * @param value the axis location found by the picker
   */
  void clicked(double value);
  /**
   * Signal emitted to make the axis interactor create a slice indicator.
   * @param point the x,y coordinates to set the indicator at
   */
  void makeIndicator(const QPoint &point);

private:
  /**
   * Function fired when the scale picker is invoked to produce a slice
   * position.
   * @param scale the associated QwtScaleWidget
   * @param pos the x,y coordinates of the mouse click event
   */
  void mouseClicked(const QwtScaleWidget *scale, const QPoint &pos);
  /**
   * A function that returns the bounding box of the QwtScaleWidget without
   * the axis title.
   * @param scale the associated QwtScaleWidget
   * @return the rectangle for the bounding box
   */
  QRect scaleRect(const QwtScaleWidget *scale) const;
};

#endif // SCALEPICKER_H_
