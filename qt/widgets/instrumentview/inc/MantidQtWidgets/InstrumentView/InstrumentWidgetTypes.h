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
