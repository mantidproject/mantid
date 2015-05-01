#ifndef SPLATTERPLOTVIEW_H_
#define SPLATTERPLOTVIEW_H_

#include "ui_SplatterPlotView.h"
#include "MantidVatesSimpleGuiViewWidgets/ViewBase.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidVatesSimpleGuiViewWidgets/CameraManager.h"
#include "MantidVatesSimpleGuiViewWidgets/PeaksTableControllerVsi.h"
#include <boost/shared_ptr.hpp>

#include <string>
#include <QList>
#include <QPointer>

class QWidget;
class QAction;

class pqPipelineRepresentation;
class pqPipelineSource;
class pqRenderView;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
  class RebinnedSourcesManager;
/**
 *
 This class creates a scatter plot using the SplatterPlot ParaView plugin. The
 view will allow thresholding of the data and the ability to overlay peaks workspaces.

 @author Michael Reuter
 @date 10/10/2011

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS SplatterPlotView : public ViewBase
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget for the threeslice view
   * @param rebinnedSourcesManager Pointer to a RebinnedSourcesManager
   */
  explicit SplatterPlotView(QWidget *parent = 0, RebinnedSourcesManager* rebinnedSourcesManager = 0);
  /// Default destructor
  virtual ~SplatterPlotView();

  /// @see ViewBase::checkView
  void checkView(ModeControlWidget::Views initialView);
  /**
   * ViewBase::destroyView
   */
  void destroyView();
  /**
   * ViewBase::getView
   */
  pqRenderView* getView();
  /**
   * ViewBase::render
   */
  void render();
  /**
   * ViewBase::renderAll
   */
  void renderAll();
  /// ViewBase::resetCamera()
  void resetCamera();
  /**
   * ViewBase::resetDisplay()
   */
  void resetDisplay();
  /**
   * Destroy all sources in the view. 
   */
  virtual void destroyAllSourcesInView();

signals:
  /// Reset to the Standard View
  void resetToStandardView();
  /// Change the state of the orthographic projection mode
  void toggleOrthographicProjection(bool state);

public slots:
  /// Check the coordinates for the peaks overlay if necessary
  void checkPeaksCoordinates();
  /// Remove the visible peaks table.
  void onRemovePeaksTable();
  /// Show all peaks in table.
  void onShowAllPeaksTable();

protected slots:
  /// Check state of toggle button with respect to peak coordinates.
  void onOverridePeakCoordToggled(bool state);
  /// Check state of toggle button for pick mode
  void onPickModeToggled(bool state);
  /**
   * Create and apply a threshold filter to the data.
   */
  void onThresholdButtonClicked();
  /// On peaks filter destroyed
  void onPeaksFilterDestroyed();
  /// On peaks source destroyed
  void onPeakSourceDestroyed();

private:
  Q_DISABLE_COPY(SplatterPlotView)

  /// Check the source for the right dimensions
  bool checkForBadDimensions(pqPipelineSource *src);
  /// Destroy all peak sources.
  void destroyPeakSources();
  /// Filter events for pick mode.
  bool eventFilter(QObject *obj, QEvent *ev);
  /// Read the coordinates and send to service.
  void readAndSendCoordinates();
  /// Setup the buttons for the visible peaks
  void setupVisiblePeaksButtons();
  /// Create the peaks filter
  void createPeaksFilter();
  /// Set the state of the peak button
  void setPeakButton(bool state);
  /// Set the frame for the peaks
  void setPeakSourceFrame(pqPipelineSource* source);
  /// Check if a peaks workspace is already part of the recorded peaks sources.
  bool checkIfPeaksWorkspaceIsAlreadyBeingTracked(pqPipelineSource* source);
  /// Update the peaks filter
  void updatePeaksFilter(pqPipelineSource* filter);
  /// Destroy splatter plot specific sources and filters
  void destroyFiltersForSplatterPlotView();

  bool noOverlay; ///< Flag to respond to overlay situation correctly
  QList<QPointer<pqPipelineSource> > peaksSource; ///< A list of peaks sources
  QPointer<pqPipelineSource> probeSource; ///< The VTK probe filter
  QPointer<pqPipelineRepresentation> splatRepr; ///< The splatter plot representation
  QPointer<pqPipelineSource> splatSource; ///< The splatter plot source
  QPointer<pqPipelineSource> threshSource; ///< The thresholding filter source
  QPointer<pqPipelineSource> m_peaksFilter; ///< The peaks filter
  Ui::SplatterPlotView ui; ///< The splatter plot view'a UI form
  QPointer<pqRenderView> view; ///< The main view area
  boost::shared_ptr<CameraManager> m_cameraManager; ///< The camera manager
  PeaksTableControllerVsi* m_peaksTableController; ///< The peaks table controller
  QAction* m_allPeaksAction;///<The action for showing all peaks in the table.
  QAction* m_removePeaksAction; ///<The action for removing the peaks table.
  std::string m_peaksWorkspaceNameDelimiter;///<Delimiter for peaks workspace strings.
};

} // SimpleGui
} // Vates
} // Mantid

#endif // SPLATTERPLOTVIEW_H_
