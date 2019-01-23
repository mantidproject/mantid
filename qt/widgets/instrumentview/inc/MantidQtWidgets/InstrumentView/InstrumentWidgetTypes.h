// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INSTRUMENTWIDGETTYPES_H_
#define INSTRUMENTWIDGETTYPES_H_

#include "DllOption.h"

namespace MantidQt {
namespace MantidWidgets {
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentWidgetTypes {

public:
  enum SurfaceType {
    FULL3D = 0,
    CYLINDRICAL_X,
    CYLINDRICAL_Y,
    CYLINDRICAL_Z,
    SPHERICAL_X,
    SPHERICAL_Y,
    SPHERICAL_Z,
    SIDE_BY_SIDE,
    RENDERMODE_SIZE
  };
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /*INSTRUMENTWIDGETTYPES_H_*/
