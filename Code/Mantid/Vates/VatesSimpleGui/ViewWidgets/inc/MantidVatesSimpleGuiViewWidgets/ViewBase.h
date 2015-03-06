#ifndef VIEWBASE_H_
#define VIEWBASE_H_

#include "MantidVatesSimpleGuiViewWidgets/BackgroundRgbProvider.h"
#include "MantidVatesSimpleGuiViewWidgets/ColorUpdater.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidVatesSimpleGuiQtWidgets/ModeControlWidget.h"

#include <QPointer>
#include <QWidget>

class pqColorMapModel;
class pqDataRepresentation;
class pqObjectBuilder;
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

class ColorSelectionWidget;

/**
 *
  This class is an abstract base class for all of the Vates simple GUI's views.

  @date 24/05/2011

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS ViewBase : public QWidget
{
  Q_OBJECT
public:
  /// Default constructor.
  ViewBase(QWidget *parent = 0);
  /// Default destructor.
  virtual ~ViewBase() {}

  /// Poll the view to set status for mode control buttons.
  virtual void checkView(ModeControlWidget::Views initialView);
  /// Poll the view to set status for mode control buttons on view switch.
  virtual void checkViewOnSwitch();
  /// Close view generated sub-windows.
  virtual void closeSubWindows();
  /// Creates a single view instance.
  virtual pqRenderView *createRenderView(QWidget *container,
                                         QString viewName=QString(""));
  /// Remove all filters of a given name: i.e. Slice.
  virtual void destroyFilter(pqObjectBuilder *builder, const QString &name);
  /// Destroy sources and view relevant to mode switching.
  virtual void destroyView() = 0;
  /// Retrieve the current time step.
  virtual double getCurrentTimeStep();
  /// Find the number of true sources in the pipeline.
  unsigned int getNumSources();
  /// Get the active ParaView source.
  pqPipelineSource *getPvActiveSrc();
  /**
   * The function gets the main view.
   * @return the main view
   */
  virtual pqRenderView *getView() = 0;
  /// Get the workspace name from the original source object.
  virtual QString getWorkspaceName();
  /// Check if pipeline has filter.
  virtual bool hasFilter(const QString &name);
  /// Check if pipeline has given workspace.
  virtual pqPipelineSource *hasWorkspace(const QString &name);
  /// Check if pipeline has a given workspace type.
  virtual bool hasWorkspaceType(const QString &wsTypeName);
  /// Check if file/workspace is a MDHistoWorkspace.
  virtual bool isMDHistoWorkspace(pqPipelineSource *src);
  /// Check if file/workspace is a temporary workspace
  virtual bool isTemporaryWorkspace(pqPipelineSource* src);
  /// Check if file/workspace is a Peaks one.
  virtual bool isPeaksWorkspace(pqPipelineSource *src);
  /// Prints properties for given source.
  virtual void printProxyProps(pqPipelineSource *src);
  /// This function makes the view render itself.
  virtual void render() = 0;
  /// This function only calls the render command for the view(s).
  virtual void renderAll() = 0;
  /// Reset the camera for a given view.
  virtual void resetCamera() = 0;
  /// This function resets the display(s) for the view(s).
  virtual void resetDisplay() = 0;
  /// Set the current color scale state
  virtual void setColorScaleState(ColorSelectionWidget *cs);
  /// Create source for plugin mode.
  virtual pqPipelineSource* setPluginSource(QString pluginName, QString wsName);
  /// Determines if source has timesteps (4D).
  virtual bool srcHasTimeSteps(pqPipelineSource *src);
  /// Set the the background color for the view
  virtual void setColorForBackground(bool viewSwitched);
  /// Sets the splatterplot button to the desired visibility.
  virtual void setSplatterplot(bool visibility);
  /// Initializes the settings of the color scale 
  virtual void initializeColorScale();
  /// Sets the standard veiw button to the desired visibility.
  virtual void setStandard(bool visibility);
  /// Enumeration for Cartesian coordinates
  enum Direction {X, Y, Z};
  /// Update settings
  virtual void updateSettings();
  // Destroy all sources in the view.
  virtual void destroyAllSourcesInView();
  // Destroy all sources in a single linear pipeline.
  virtual void destroySinglePipeline(pqPipelineSource * source);

  QPointer<pqPipelineSource> origSrc; ///< The original source
  QPointer<pqPipelineRepresentation> origRep; ///< The original source representation

public slots:
  /// Set the color scale back to the original bounds.
  void onAutoScale(ColorSelectionWidget* colorSelectionWidget);
  /// Set the requested color map on the data.
  void onColorMapChange(const pqColorMapModel *model);
  /// Set the data color scale range to the requested bounds.
  void onColorScaleChange(double min, double max);
  /// Set the view to use a LOD threshold.
  void onLodThresholdChange(bool state, double defVal);
  /// Set logarithmic color scaling on the data.
  void onLogScale(int state);
  /// Set the view to use a parallel projection.
  void onParallelProjection(bool state);
  /// Reset center of rotation to given point.
  void onResetCenterToPoint(double x, double y, double z);
  /// Set color scaling for a view.
  void setColorsForView(ColorSelectionWidget *colorScale);
  /// Setup the animation controls.
  void updateAnimationControls();
  /// Provide updates to UI.
  virtual void updateUI();
  /// Provide updates to View
  virtual void updateView();
  /// React when the visibility of a representation changes
  virtual void onVisibilityChanged(pqPipelineSource *source, pqDataRepresentation *representation);
  virtual void onSourceDestroyed();

signals:
  /**
   * Signal to get the range of the data.
   * @param min the minimum value of the data
   * @param max the maximum value of the data
   */
  void dataRange(double min, double max);
  /**
   * Signal to disable all the color selection controls.
   * @param state set to false to lock out all controls
   */
  void lockColorControls(bool state=false);
  /// Signal indicating rendering is done.
  void renderingDone();
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
   * Signal to set the status of a specific view mode button.
   * @param mode the particular requested view
   * @param state flag for setting enable/disable button state
   */
  void setViewStatus(ModeControlWidget::Views mode, bool state);
  /**
   * Signal to set the status of the view mode buttons.
   * @param view The initial view.
   * @param state Whether or not to enable to view mode buttons.
   */
  void setViewsStatus(ModeControlWidget::Views view, bool state);
  /**
   * Signal to perform a possible rebin.
   * @param algorithmType The type of rebinning algorithm.
   */
  void rebin(std::string algorithmType);
   /**
   * Signal to perform a possible unbin on a sources which has been
   * rebinned in the VSI.
   */
  void unbin();
  /**
   * Singal to tell other elements that the log scale was altered programatically
   * @param state flag wheter or not to enable the 
   */
  void setLogScale(bool state);

protected:
  /**
   * Set the color scale for auto color scaling.
   */
  void setAutoColorScale();

private:
  Q_DISABLE_COPY(ViewBase)

  /// Return the active representation determined by ParaView.
  pqPipelineRepresentation *getPvActiveRep();
  /// Return the active view determined by ParaView.
  pqRenderView *getPvActiveView();
  /// Return the appropriate representation.
  pqPipelineRepresentation *getRep();
  /// Collect time information for animation controls.
  void handleTimeInfo(vtkSMDoubleVectorProperty *dvp);

  ColorUpdater colorUpdater; ///< Handle to the color updating delegator
  BackgroundRgbProvider backgroundRgbProvider; /// < Holds the manager for background color related tasks.
   const pqColorMapModel* m_currentColorMapModel;

  QString m_temporaryWorkspaceIdentifier;
};

}
}
}

#endif // VIEWBASE_H_
