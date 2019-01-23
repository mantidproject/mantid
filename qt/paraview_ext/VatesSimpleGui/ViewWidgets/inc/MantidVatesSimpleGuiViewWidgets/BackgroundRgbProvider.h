// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef BACKGROUNDRGB_PROVIDER_H_
#define BACKGROUNDRGB_PROVIDER_H_

#include "MantidQtWidgets/Common/MdSettings.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include <map>
#include <string>
#include <vector>

#if defined(__INTEL_COMPILER)
#pragma warning disable 1170
#endif

#include <pqRenderView.h>

#if defined(__INTEL_COMPILER)
#pragma warning enable 1170
#endif

class vtkObject;

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
 *
  This class gets the default color values for the background of the view.

  @date 10/12/2014
*/

class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS BackgroundRgbProvider {
public:
  BackgroundRgbProvider();

  ~BackgroundRgbProvider();

  /**
   * Set the Rgb values for the color of the view's background.
   * @param useCurrentBackgroundColor Is this the initial loading or were the
   * views switched?
   * @param view The view which has its background color set.
   */
  void setBackgroundColor(pqRenderView *view, bool useCurrentBackgroundColor);

  /**
   * Listen to a change in the background color
   *@param view The view which we want to listen to.
   */
  void observe(pqRenderView *view);

  /**
   * Update the last session background color.
   */
  void update();

private:
  /**
   * Get the Rgb values for the color of the view's background from the user
   * setting.
   * @param useCurrentBackgroundColor Is this the initial loading or were the
   * views switched?
   * @returns A vector with the RGB values
   */
  std::vector<double> getRgbFromSetting(bool useCurrentBackgroundColor);

  /**
   * Get the Rgb values for the color of the view's background
   * @param useCurrentBackgroundColor Is this the initial loading or were the
   * views switched?
   * @returns A vector with the RGB values
   */
  std::vector<double> getRgb(bool useCurrentBackgroundColor);

  /**
   * Callback function for background color changing events
   * @param caller Calling object.
   *
   * @param eventID vtkCommand event ID for callbacks, not used here
   * @param clientData vtk client data, not used here
   * @param callData vtk call data, not used here
   */
  static void backgroundColorChangeCallbackFunction(vtkObject *caller,
                                                    long unsigned int eventID,
                                                    void *clientData,
                                                    void *callData);

  static QColor currentBackgroundColor;

  MantidQt::API::MdSettings m_mdSettings;
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
#endif
