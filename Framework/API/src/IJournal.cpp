// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/IJournal.h"

namespace Mantid {
namespace API {

IJournal::~IJournal() = default;

IJournal::IJournal(IJournal &&rhs) = default;

IJournal &IJournal::operator=(IJournal &&rhs) = default;
} // namespace API
} // namespace Mantid
