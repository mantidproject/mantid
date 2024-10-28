// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/* Used to register classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 */
#define DECLARE_IMPLICIT_FUNCTION_PARAMETER_PARSER(classname)                                                          \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper register_alg_##classname(                                                         \
      ((Mantid::API::ImplicitFunctionParameterParserFactory::Instance().subscribe<classname>(#classname)), 0));        \
  }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <memory>
#include <vector>

#ifndef Q_MOC_RUN
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <memory>
#endif

#include "ImplicitFunctionParameter.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/ArrayProperty.h"

namespace Poco {
namespace DOM {
class Element;
}
} // namespace Poco

/** XML Parser for parameter types for ImplicitFunctions

 @author Owen Arnold, Tessella plc
 @date 01/10/2010
 */

namespace Mantid {
namespace API {

/*
 * Deletion policy for unique pointers.
 */
// clang-format off
template <typename T> class DLLExport DeleterPolicy{
public:
  void operator()(T *pParser){ delete pParser; }
};
// clang-format on

/*
 * ImplicitFunctionParameterParser definition. Used to parse implicit function
 * xml.
 */
class MANTID_API_DLL ImplicitFunctionParameterParser {
public:
  /// Successor type. Unique shared pointer with stack scoped deletion
  /// semantics.
  using SuccessorType =
      boost::interprocess::unique_ptr<ImplicitFunctionParameterParser, DeleterPolicy<ImplicitFunctionParameterParser>>;

  virtual ImplicitFunctionParameter *createParameter(Poco::XML::Element *parameterElement) = 0;
  virtual void setSuccessorParser(ImplicitFunctionParameterParser *paramParser) = 0;
  virtual ~ImplicitFunctionParameterParser() = default;

protected:
  SuccessorType m_successor;
};
} // namespace API
} // namespace Mantid
