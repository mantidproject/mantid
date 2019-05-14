// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_ZOOMER_H
#define MPLCPP_ZOOMER_H

#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"

using namespace MantidQt::Widgets::Common;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
class FigureCanvasQt;

/**
 * @brief The Zoomer class adds zooming capabilities to
 * an existing FigureCanvasQt object. The implementation relies on
 * the matplotlib NavigationToolbar2 class corresponding to the backend.
 *
 * This object holds a pointer to the FigureCanvasQt object but
 * it will not keep it alive. It is assumed that the canvas lifetime
 * is handled separately.
 */
class MANTID_MPLCPP_DLL Zoomer : public Python::InstanceHolder {
public:
  explicit Zoomer(FigureCanvasQt *canvas);

  bool isZoomEnabled() const;
  void enableZoom(bool requestOn);
  void zoomOut();

private:
  FigureCanvasQt *m_canvas;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_ZOOMER_H
