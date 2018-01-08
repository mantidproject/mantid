#ifndef COMPONENTTYPE_H
#define COMPONENTTYPE_H
#include "MantidBeamline/DllConfig.h"

namespace Mantid {
namespace Beamline {
enum class ComponentType {
  Generic,
  Rectangular,
  Structured,
  BankOfTube,
  Tube,
  Detector
};
}
} // namespace Mantid
#endif // COMPONENTTYPE_H
