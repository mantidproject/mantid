// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef COMPONENTTYPE_H
#define COMPONENTTYPE_H
#include "MantidBeamline/DllConfig.h"

namespace Mantid {
namespace Beamline {
enum class ComponentType {
  Generic,
  Infinite,
  Rectangular,
  Structured,
  Unstructured,
  Detector,
  OutlineComposite
};
}
} // namespace Mantid
#endif // COMPONENTTYPE_H
