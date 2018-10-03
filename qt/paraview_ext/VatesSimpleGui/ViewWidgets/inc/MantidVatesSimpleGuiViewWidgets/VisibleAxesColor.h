// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VISIBLEAXESCOLOR_H_
#define MANTID_VISIBLEAXESCOLOR_H_
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "pqRenderView.h"

namespace Mantid {
namespace Vates {
namespace SimpleGui {



class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS VisibleAxesColor {
public:
  /**
   * Set the Rgb values for the color of the view's orientation axes Label.
   * @param view The view which has its color set.
   */
  unsigned long setAndObserveAxesColor(pqView *view);
  void setOrientationAxesLabelColor(pqView *view,
                                    const std::array<double, 3> &color);
  void setGridAxesColor(pqView *view, const std::array<double, 3> &color);
  void setScalarBarColor(pqView *view, const std::array<double, 3> &color);
  unsigned long observe(pqView *view);

private:
  void backgroundColorChangeCallback(vtkObject *caller, unsigned long, void *);
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif // MANTID_VISIBLEAXESCOLOR_H_
