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

#include "ImplicitFunctionParameterParser.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace API {
class MANTID_API_DLL ImplicitFunctionParameterParserFactoryImpl
    : public Kernel::DynamicFactory<ImplicitFunctionParameterParser> {
public:
  ImplicitFunctionParameterParserFactoryImpl(const ImplicitFunctionParameterParserFactoryImpl &) = delete;
  ImplicitFunctionParameterParserFactoryImpl &operator=(const ImplicitFunctionParameterParserFactoryImpl &) = delete;
  std::shared_ptr<ImplicitFunctionParameterParser> create(const std::string &xmlString) const override;
  ImplicitFunctionParameterParser *
  createImplicitFunctionParameterParserFromXML(Poco::XML::Element *parametersElement) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<ImplicitFunctionParameterParserFactoryImpl>;

  /// Private Constructor for singleton class
  ImplicitFunctionParameterParserFactoryImpl() = default;

  /// Private Destructor
  ~ImplicitFunctionParameterParserFactoryImpl() override = default;
};

using ImplicitFunctionParameterParserFactory =
    Mantid::Kernel::SingletonHolder<ImplicitFunctionParameterParserFactoryImpl>;
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::ImplicitFunctionParameterParserFactoryImpl>;
}
} // namespace Mantid
