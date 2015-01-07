#ifndef COLORSELECTIONWIDGET_H_
#define COLORSELECTIONWIDGET_H_

#include "ui_ColorSelectionWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "boost/scoped_ptr.hpp"
#include "MantidVatesSimpleGuiViewWidgets/ColorMapManager.h"
#include <QWidget>

class pqColorMapModel;
class pqColorPresetManager;
class pqColorPresetModel;
class vtkPVXMLParser;

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

public slots:
  /// Set state for all control widgets.
  void enableControls(bool state);
  /// Reset the widget's state.
  void reset();
  /// Set the color scale range into the range widgets.
  void setColorScaleRange(double min, double max);

signals:
  /**
   * Signal to let views know that autoscaling is on.
   */
  void autoScale();
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
  /// Set state of the automatic scaling checkbox.
  void autoOrManualScaling(int state);
  /// Get the new color scale range.
  void getColorScaleRange();
  /// Show available color presets.
  void loadPreset();
  /// Set log color scaling.
  void useLogScaling(int state);

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

  pqColorPresetManager *presets; ///< Dialog for choosing color presets
  Ui::ColorSelectionWidgetClass ui; ///< The mode control widget's UI form

  boost::scoped_ptr<ColorMapManager> colorMapManager; ///< Keeps track of the available color maps.
};

} // SimpleGui
} // Vates
} // Mantid

#endif // COLORSELECTIONWIDGET_H_
