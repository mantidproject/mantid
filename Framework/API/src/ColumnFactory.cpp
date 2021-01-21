// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ColumnFactory.h"
#include "MantidAPI/Column.h"
#include <sstream>

namespace Mantid {
namespace API {

std::shared_ptr<Column> ColumnFactoryImpl::create(const std::string &type) const {
  std::shared_ptr<Column> c = Kernel::DynamicFactory<Column>::create(type);
  c->m_type = type;
  return c;
}

} // namespace API
} // namespace Mantid
