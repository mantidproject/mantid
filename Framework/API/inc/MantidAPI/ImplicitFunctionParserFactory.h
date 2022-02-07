// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/** @class ImplicitFunctionParserFactory ImplicitFunctionParserFactory.h
   Kernel/ImplicitFunctionParserFactory.h

    This dynamic factory implementation generates concrete instances of
   ImplicitFunctionParsers.

    @author Owen Arnold, Tessella Support Services plc
    @date 27/10/2010
    */

#include "ImplicitFunctionParameterParserFactory.h"
#include "ImplicitFunctionParser.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace API {
class MANTID_API_DLL ImplicitFunctionParserFactoryImpl : public Kernel::DynamicFactory<ImplicitFunctionParser> {
public:
  ImplicitFunctionParserFactoryImpl(const ImplicitFunctionParserFactoryImpl &) = delete;
  ImplicitFunctionParserFactoryImpl &operator=(const ImplicitFunctionParserFactoryImpl &) = delete;
  std::shared_ptr<ImplicitFunctionParser> create(const std::string &xmlString) const override;
  ImplicitFunctionParser *createImplicitFunctionParserFromXML(const std::string &functionXML) const;

  ImplicitFunctionParser *createImplicitFunctionParserFromXML(Poco::XML::Element *functionElement) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<ImplicitFunctionParserFactoryImpl>;

  /// Private Constructor for singleton class
  ImplicitFunctionParserFactoryImpl() = default;
  /// Private Destructor
  ~ImplicitFunctionParserFactoryImpl() override = default;
};

using ImplicitFunctionParserFactory = Mantid::Kernel::SingletonHolder<ImplicitFunctionParserFactoryImpl>;
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::ImplicitFunctionParserFactoryImpl>;
}
} // namespace Mantid
