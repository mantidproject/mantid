#ifndef VIEWBASE_H_
#define VIEWBASE_H_

#include "MantidVatesSimpleGuiViewWidgets/ColorUpdater.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include <QPointer>
#include <QWidget>

class pqColorMapModel;
class pqObjectBuilder;
class pqPipelineBrowserWidget;
class pqPipelineSource;
class pqPipelineRepresentation;
class pqRenderView;
class vtkSMDoubleVectorProperty;

class QString;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS ViewBase : public QWidget
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

  /// Poll the view to set status for mode control buttons.
  virtual void checkView();
  /// Close view generated sub-windows.
  virtual void closeSubWindows();
  /**
   * Function used to correct post-accept visibility issues. Most
   * views won't need to do anything.
   */
  virtual void correctVisibility(pqPipelineBrowserWidget *pbw);
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
   * Destroy sources and view relevant to mode switching.
   */
  virtual void destroyView() = 0;
  /// Retrieve the current time step.
  virtual double getCurrentTimeStep();
  /// Get the active ParaView source.
  pqPipelineSource *getPvActiveSrc();
  /**
   * The function gets the main view.
   * @return the main view
   */
  virtual pqRenderView *getView() = 0;
  /// Get the workspace name from the original source object.
  virtual QString getWorkspaceName();
  /// Check if file/workspace is a Peaks one.
  virtual bool isPeaksWorkspace(pqPipelineSource *src);
  /// Prints properties for given source.
  virtual void printProxyProps(pqPipelineSource *src);
  /**
   * This function makes the view render itself.
   */
  virtual void render() = 0;
  /**
   * This function only calls the render command for the view(s).
   */
  virtual void renderAll() = 0;
  /// Reset the camera for a given view.
  virtual void resetCamera() = 0;
  /**
   * This function resets the display(s) for the view(s).
   */
  virtual void resetDisplay() = 0;
  /// Setup axis scales
  virtual void setAxisScales();
  /// Create source for plugin mode.
  virtual void setPluginSource(QString pluginName, QString wsName);
  /// Determines if source has timesteps (4D).
  virtual bool srcHasTimeSteps(pqPipelineSource *src);

  /// Enumeration for Cartesian coordinates
  enum Direction {X, Y, Z};

  QPointer<pqPipelineSource> origSrc; ///< The original source
  QPointer<pqPipelineRepresentation> origRep; ///< The original source representation

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
  /// Set the view to use a parallel projection.
  void onParallelProjection(bool state);
  /// Reset center of rotation to center of data volume.
  void onResetCenterToData();
  /// Reset center of rotation to given point.
  void onResetCenterToPoint(double x, double y, double z);
  /// Setup the animation controls.
  void setTimeSteps(bool withUpdate = false);

signals:
  /**
   * Signal to get the range of the data.
   * @param min the minimum value of the data
   * @param max the maximum value of the data
   */
  void dataRange(double min, double max);
  /// Signal to trigger pipeline update.
  void triggerAccept();
  /**
   * Signal to update state of animation controls.
   * @param state flag to enable/disable animantion controls
   */
  void setAnimationControlState(bool state);
  /**
   * Signal to update animation control information.
   * @param start the value of start "time"
   * @param stop the value of the end "time"
   * @param numSteps the number of "time" steps
   */
  void setAnimationControlInfo(double start, double stop, int numSteps);
  /**
   * Signal to set the status of the view mode buttons.
   * @param state whether or not to enable to view mode buttons
   */
  void setViewsStatus(bool state);

private:
  Q_DISABLE_COPY(ViewBase)

  /// Return the active representation determined by ParaView.
  pqPipelineRepresentation *getPvActiveRep();
  /// Return the active view determined by ParaView.
  pqRenderView *getPvActiveView();
  /// Return the appropriate representation.
  pqPipelineRepresentation *getRep();
  /// Find the number of true sources in the pipeline.
  unsigned int getNumSources();
  /// Collect time information for animation controls.
  void handleTimeInfo(vtkSMDoubleVectorProperty *dvp, bool doUpdate);

  ColorUpdater colorUpdater; ///< Handle to the color updating delegator
};

}
}
}

#endif // VIEWBASE_H_
