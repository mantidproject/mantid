#ifndef COLORSELECTIONWIDGET_H_
#define COLORSELECTIONWIDGET_H_

#include "ui_ColorSelectionWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/ColorMapManager.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidVatesAPI/ColorScaleGuard.h"
#include "MantidQtAPI/MdConstants.h"
#include "boost/scoped_ptr.hpp"
#include <QWidget>

class pqColorMapModel;
class pqColorPresetManager;
class pqColorPresetModel;
class vtkPVXMLParser;
class QDoubleValidator;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
/**
 *
  This class controls the color scale for the main level program viewed
  datasets.

  @date 07/06/2011

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS ColorSelectionWidget : public QWidget
{
  Q_OBJECT

public:
  /// Default constructor.
  ColorSelectionWidget(QWidget *parent = 0);
  /// Default destructor.
  virtual ~ColorSelectionWidget() {}

  /// Get the auto color scaling state
  bool getAutoScaleState();
  /// Get the log scale state
  bool getLogScaleState();
  /// Get the minimum color range value
  double getMinRange();
  /// Get the maximum color range value
  double getMaxRange();
  /// Load the default color map
  void loadColorMap(bool viewSwitched);
  /// Programmatically enable/disable auto scaling of color range
  void setAutoScale(bool autoScale);
  /// Set min smaller max, can be used to programmatically set the widgets
  void setMinMax(double& min, double& max);
  /// Others need to know if this widget is in the process of updating colors at user's request
  bool inProcessUserRequestedAutoScale() { return m_inProcessUserRequestedAutoScale; };
  /// To effectively block callbacks from external (Paraview) color changes
  void ignoreColorChangeCallbacks(bool ignore);
  bool isIgnoringColorCallbacks();
  /// Set the color scale lock
  void setColorScaleLock(Mantid::VATES::ColorScaleLock* lock);
  /// Is the color scale locked
  bool isColorScaleLocked() const;

public slots:
  /// Set state for all control widgets.
  void enableControls(bool state);
  /// Reset the widget's state.
  void reset();
  /// Set the color scale range into the range widgets (only in autoscale mode).
  void setColorScaleRange(double min, double max);
  /// Slot for when the user clicks on the auto-scale check box
  void autoCheckBoxClicked(bool wasOnn);
  /// Set log scaling button
  void onSetLogScale(bool state);

signals:
  /**
   * Signal to let views know that autoscaling is on.
   */
  void autoScale(ColorSelectionWidget*);
  /**
   * Signal to pass on information about a change to the color map.
   * @param model the color map to send
   */
  void colorMapChanged(const pqColorMapModel *model);
  /**
   * Signal to pass on information that the color scale has changed.
   *
   * @param min the minimum value of the color scale
   * @param max the maximum value of the color scale
   */
  void colorScaleChanged(double min, double max);
  /**
   * Signal to pass on use of log color scaling.
   * @param state flag for whether or not to use log color scaling
   */
  void logScale(int state);

protected slots:
  /// Get the new color scale range.
  void getColorScaleRange();
  /// Show available color presets.
  void loadPreset();
  /// Set log color scaling.
  void useLogScaling(int state);
  /// Set log color scaling, on user click
  void useLogScalingClicked(bool wasOn);

private:
  /// Add color maps from XML files.
  void addColorMapsFromFile(std::string fileName, vtkPVXMLParser *parser,
                            pqColorPresetModel *model);
  /// Add color maps from XML fragments.
  void addColorMapsFromXML(vtkPVXMLParser *parser, pqColorPresetModel *model);
  /// Set up various color maps.
  void loadBuiltinColorPresets();
  /// Set status of the color selection editor widgets.
  void setEditorStatus(bool status);
  /// Set up the behaviour for with or without log scale.
  void setupLogScale(int state);
  /// Set min smaller max, can be used to programmatically set the widgets
  void setMinSmallerMax(double& min, double& max);

  boost::scoped_ptr<ColorMapManager> colorMapManager; ///< Keeps track of the available color maps.

  QDoubleValidator* m_minValidator;
  QDoubleValidator* m_maxValidator;
  double m_minHistoric;
  double m_maxHistoric;

  MantidQt::API::MdConstants m_mdConstants;

  pqColorPresetManager *m_presets; ///< Dialog for choosing color presets
  Ui::ColorSelectionWidgetClass m_ui; ///< The mode control widget's UI form
  bool m_ignoreColorChangeCallbacks; ///< Effectively blocks/disables callbacks

  /// this is a flag that is set while updating the color scale triggered by the user clicking on the auto-scale box
  bool m_inProcessUserRequestedAutoScale;

  Mantid::VATES::ColorScaleLock* m_colorScaleLock;
};

} // SimpleGui
} // Vates
} // Mantid

#endif // COLORSELECTIONWIDGET_H_
