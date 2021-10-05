// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidParallel/ThreadingBackend.h"

namespace Mantid::Parallel::detail {

ThreadingBackend::ThreadingBackend(const int size) : m_size(size) {}

int ThreadingBackend::size() const { return m_size; }

} // namespace Mantid::Parallel::detail
