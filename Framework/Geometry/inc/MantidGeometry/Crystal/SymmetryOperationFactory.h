// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <list>
#include <map>

#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidGeometry/Crystal/SymmetryOperationSymbolParser.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/RegistrationHelper.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace Geometry {
/**
  @class SymmetryOperationFactoryImpl

  A factory for symmetry operations. Symmetry operations are stored
  with respect to their identifier (see SymmetryOperation for details).

  Creation of symmetry operations is then performed like this:

      SymmetryOperations inversion =
          SymmetryOperationFactory::Instance().createSymOp("x,y,z");

  Creating a symmetry operation object automatically registers the object
  as a prototype and subsequent creations are much more efficient because
  the symbol does not need to be parsed.

  Available symmetry operations may be queried with
  SymmetryOperation::subscribedSymbols.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 10/09/2014
*/
class MANTID_GEOMETRY_DLL SymmetryOperationFactoryImpl {
public:
  SymmetryOperation createSymOp(const std::string &identifier);
  std::vector<SymmetryOperation> createSymOps(const std::string &identifiers);
  std::vector<SymmetryOperation> createSymOps(const std::vector<std::string> &identifiers);

  void subscribeSymOp(const std::string &identifier);
  void unsubscribeSymOp(const std::string &identifier);

  bool isSubscribed(const std::string &identifier) const;

  std::vector<std::string> subscribedSymbols() const;

protected:
  void subscribe(const std::string &alias, const SymmetryOperation &prototype);

  std::map<std::string, SymmetryOperation> m_prototypes;

private:
  friend struct Mantid::Kernel::CreateUsingNew<SymmetryOperationFactoryImpl>;

  SymmetryOperationFactoryImpl();
};

using SymmetryOperationFactory = Mantid::Kernel::SingletonHolder<SymmetryOperationFactoryImpl>;

} // namespace Geometry
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_GEOMETRY template class MANTID_GEOMETRY_DLL
    Mantid::Kernel::SingletonHolder<Mantid::Geometry::SymmetryOperationFactoryImpl>;
}
} // namespace Mantid

#define DECLARE_SYMMETRY_OPERATION(operation, name)                                                                    \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper                                                                                   \
      register_symop_##name(((Mantid::Geometry::SymmetryOperationFactory::Instance().subscribeSymOp(operation)), 0));  \
  }
