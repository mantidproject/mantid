// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace API {
class IArchiveSearch;

/**
Creates instances of IArchiveSearch

@author Roman Tolchenov, Tessella plc
@date 27/07/2010
*/
class MANTID_API_DLL ArchiveSearchFactoryImpl : public Kernel::DynamicFactory<IArchiveSearch> {
public:
  ArchiveSearchFactoryImpl(const ArchiveSearchFactoryImpl &) = delete;
  ArchiveSearchFactoryImpl &operator=(const ArchiveSearchFactoryImpl &) = delete;

private:
  friend struct Mantid::Kernel::CreateUsingNew<ArchiveSearchFactoryImpl>;

  /// Private Constructor for singleton class
  ArchiveSearchFactoryImpl();
  /// Private Destructor
  ~ArchiveSearchFactoryImpl() override = default;
};

using ArchiveSearchFactory = Mantid::Kernel::SingletonHolder<ArchiveSearchFactoryImpl>;
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::ArchiveSearchFactoryImpl>;
}
} // namespace Mantid
