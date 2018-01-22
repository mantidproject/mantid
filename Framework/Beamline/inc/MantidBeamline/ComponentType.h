#ifndef COMPONENTTYPE_H
#define COMPONENTTYPE_H
#include "MantidBeamline/DllConfig.h"

namespace Mantid {
namespace Beamline {
enum class ComponentType {
  Generic,
  Rectangular,
  Structured,
  Unstructured,
  Detector,
  OutlineComposite
};
}
} // namespace Mantid
#endif // COMPONENTTYPE_H
