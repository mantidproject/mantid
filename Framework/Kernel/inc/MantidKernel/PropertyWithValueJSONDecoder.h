// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROPERTYWITHVALUEJSONDECODER_H
#define PROPERTYWITHVALUEJSONDECODER_H

#include "MantidKernel/DllConfig.h"
#include <memory>

namespace Json {
class Value;
}

namespace Mantid {
namespace Kernel {
class Property;

/// Attempt to create a Property from a Json value object
MANTID_KERNEL_DLL std::unique_ptr<Property> decode(const Json::Value &value);

} // namespace Kernel
} // namespace Mantid

#endif // PROPERTYWITHVALUEJSONDECODER_H
