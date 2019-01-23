// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/MDLoadingViewSimple.h"

namespace Mantid {
namespace VATES {

void MDLoadingViewSimple::setTime(double time) { m_time = time; }

double MDLoadingViewSimple::getTime() const { return m_time; }

void MDLoadingViewSimple::setRecursionDepth(size_t recursionDepth) {
  m_recursionDepth = recursionDepth;
}

size_t MDLoadingViewSimple::getRecursionDepth() const {
  return m_recursionDepth;
}

void MDLoadingViewSimple::setLoadInMemory(bool loadInMemory) {
  m_loadInMemory = loadInMemory;
}

bool MDLoadingViewSimple::getLoadInMemory() const { return m_loadInMemory; }
} // namespace VATES
} // namespace Mantid
