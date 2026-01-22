// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "RowExceptions.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

RowNotFoundException::~RowNotFoundException() = default;
MultipleRowsFoundException::~MultipleRowsFoundException() = default;
InvalidTableException::~InvalidTableException() = default;

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
