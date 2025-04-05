// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

namespace Mantid::NeXus {
class NXClass;
struct NXClassInfo;
class NXData;
class NXEntry;
class NXRoot;
template <typename T> class NXDataSetTyped;
using NXInt = NXDataSetTyped<int32_t>;
using NXFloat = NXDataSetTyped<float>;
using NXDouble = NXDataSetTyped<double>;
} // namespace Mantid::NeXus
