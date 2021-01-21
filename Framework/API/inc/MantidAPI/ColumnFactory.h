// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include <vector>

namespace Mantid {

namespace API {
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class Column;

/** @class ColumnFactoryImpl

    The ColumnFactory class is in charge of the creation of concrete
    instances of columns. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 31/10/2008
*/
class MANTID_API_DLL ColumnFactoryImpl : public Kernel::DynamicFactory<Column> {
public:
  ColumnFactoryImpl(const ColumnFactoryImpl &) = delete;
  ColumnFactoryImpl &operator=(const ColumnFactoryImpl &) = delete;
  /// Creates an instance of a column
  std::shared_ptr<Column> create(const std::string &type) const override;

private:
  friend struct Mantid::Kernel::CreateUsingNew<ColumnFactoryImpl>;

  /// Private Constructor for singleton class
  ColumnFactoryImpl() = default;
  /// Private Destructor
  ~ColumnFactoryImpl() override = default;
};

using ColumnFactory = Mantid::Kernel::SingletonHolder<ColumnFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::ColumnFactoryImpl>;
}
} // namespace Mantid
