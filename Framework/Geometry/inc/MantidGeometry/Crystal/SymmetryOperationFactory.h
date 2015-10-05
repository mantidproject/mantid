#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORY_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORY_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidGeometry/Crystal/SymmetryOperationSymbolParser.h"
#include "MantidKernel/RegistrationHelper.h"

#include <list>
#include <map>
#include <iostream>

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

    Copyright © 2014 PSI-MSS

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_GEOMETRY_DLL SymmetryOperationFactoryImpl {
public:
  SymmetryOperation createSymOp(const std::string &identifier);
  std::vector<SymmetryOperation> createSymOps(const std::string &identifiers);
  std::vector<SymmetryOperation>
  createSymOps(const std::vector<std::string> &identifiers);

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

// This is taken from FuncMinimizerFactory
#ifdef _WIN32
template class MANTID_GEOMETRY_DLL
    Mantid::Kernel::SingletonHolder<SymmetryOperationFactoryImpl>;
#endif

typedef Mantid::Kernel::SingletonHolder<SymmetryOperationFactoryImpl>
    SymmetryOperationFactory;

} // namespace Geometry
} // namespace Mantid

#define DECLARE_SYMMETRY_OPERATION(operation, name)                            \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_symop_##name(                    \
      ((Mantid::Geometry::SymmetryOperationFactory::Instance().subscribeSymOp( \
           operation)),                                                        \
       0));                                                                    \
  }

#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORY_H_ */
