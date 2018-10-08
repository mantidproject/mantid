#ifndef MPLCPP_ZOOMER_H
#define MPLCPP_ZOOMER_H
/*
 Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
*/
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Python/Object.h"

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
