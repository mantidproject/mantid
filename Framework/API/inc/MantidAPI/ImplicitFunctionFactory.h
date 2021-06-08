// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/** @class ImplicitFunctionFactory ImplicitFunctionFactory.h
Kernel/ImplicitFunctionFactory.h

This dynamic factory implementation generates concrete instances of
ImplicitFunctions.

    @author Owen Arnold, Tessella Support Services plc
    @date 27/10/2010
*/

#include "ImplicitFunctionParserFactory.h"
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace API {
class MANTID_API_DLL ImplicitFunctionFactoryImpl : public Kernel::DynamicFactory<Mantid::Geometry::MDImplicitFunction> {
public:
  ImplicitFunctionFactoryImpl(const ImplicitFunctionFactoryImpl &) = delete;
  ImplicitFunctionFactoryImpl &operator=(const ImplicitFunctionFactoryImpl &) = delete;
  Mantid::Geometry::MDImplicitFunction_sptr create(const std::string &className) const override;

  virtual Mantid::Geometry::MDImplicitFunction *createUnwrapped(Poco::XML::Element *processXML) const;

  Mantid::Geometry::MDImplicitFunction *createUnwrapped(const std::string &processXML) const override;

  friend struct Mantid::Kernel::CreateUsingNew<ImplicitFunctionFactoryImpl>;

private:
  /// Private Constructor for singleton class
  ImplicitFunctionFactoryImpl() = default;
  /// Private Destructor
  ~ImplicitFunctionFactoryImpl() override = default;
};

using ImplicitFunctionFactory = Mantid::Kernel::SingletonHolder<ImplicitFunctionFactoryImpl>;
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::ImplicitFunctionFactoryImpl>;
}
} // namespace Mantid
