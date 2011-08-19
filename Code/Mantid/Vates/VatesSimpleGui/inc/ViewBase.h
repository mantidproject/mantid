#ifndef VIEWBASE_H_
#define VIEWBASE_H_

#include "ColorUpdater.h"

#include <QtGui/QWidget>
#include <QPointer>

class pqColorMapModel;
class pqObjectBuilder;
class pqPipelineSource;
class pqPipelineRepresentation;
class pqRenderView;

class QString;
/**
 *
  This class is an abstract base class for all of the Vates simple GUI's views.

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
class ViewBase : public QWidget 
{
  Q_OBJECT
public:
  /**
   * Default constructor.
   * @param parent the parent widget for the view
   */
  ViewBase(QWidget *parent = 0);
  /// Default destructor.
  virtual ~ViewBase() {}
  /**
   * Function that creates a single view instance.
   * @param container the UI widget to associate the view with
   * @return the created view
   */
  virtual pqRenderView *createRenderView(QWidget *container);
  /**
   * This function removes all filters of a given name: i.e. Slice.
   * @param builder the ParaView object builder
   * @param name the class name of the filters to remove
   */
  virtual void destroyFilter(pqObjectBuilder *builder, const QString &name);
  /**
   * The function gets the main view.
   * @return the main view
   */
  virtual pqRenderView *getView() = 0;
  /**
   * This function makes the view render itself.
   */
  virtual void render() = 0;
  /**
   * This function only calls the render command for the view(s).
   */
  virtual void renderAll() = 0;
  /**
   * This function resets the display(s) for the view(s).
   */
  virtual void resetDisplay() = 0;

  /// Enumeration for Cartesian coordinates
  enum Direction {X, Y, Z};

  QPointer<pqPipelineSource> origSource; ///< The current source
  QPointer<pqPipelineRepresentation> originSourceRepr; ///< The current source representation

public slots:
  /// Set the color scale back to the original bounds.
  void onAutoScale();
  /**
   * Set the requested color map on the data.
   * @param model the color map to use
   */
  void onColorMapChange(const pqColorMapModel *model);
  /**
   * Set the data color scale range to the requested bounds.
   * @param min the minimum bound for the color scale
   * @param max the maximum bound for the color scale
   */
  void onColorScaleChange(double min, double max);
  /**
   * Set logarithmic color scaling on the data.
   * @param state flag to determine whether or not to use log color scaling
   */
  void onLogScale(int state);

signals:
  /**
   * Signal to get the range of the data.
   * @param min the minimum value of the data
   * @param max the maximum value of the data
   */
  void dataRange(double min, double max);

private:
  Q_DISABLE_COPY(ViewBase);
  ColorUpdater colorUpdater; /// Handle to the color updating delegator
};

#endif // VIEWBASE_H_
