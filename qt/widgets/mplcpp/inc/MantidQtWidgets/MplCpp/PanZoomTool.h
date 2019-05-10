// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_PANZOOMTOOL_H
#define MPLCPP_PANZOOMTOOL_H

#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Python/Object.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
class FigureCanvasQt;

/**
 * @brief The PanZoomTool class allows pan and zoom tools to be
 * enabled on a canvas.
 *
 * This object holds a pointer to the FigureCanvasQt object but
 * it will not keep it alive. It is assumed that the canvas lifetime
 * is handled separately.
 */
class MANTID_MPLCPP_DLL PanZoomTool : public Python::InstanceHolder {
public:
  explicit PanZoomTool(FigureCanvasQt *canvas);
  /// @name Zoom tools
  /// @{
  bool isZoomEnabled() const;
  void enableZoom(bool on);
  void zoomOut();
  ///@}

  /// @name Zoom tools
  /// @{
  bool isPanEnabled() const;
  void enablePan(bool on);
  ///@}

private:
  FigureCanvasQt *m_canvas;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_PANZOOMTOOL_H
