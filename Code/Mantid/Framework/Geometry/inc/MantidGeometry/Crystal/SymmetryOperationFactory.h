#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORY_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORY_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidGeometry/Crystal/SymmetryOperation.h"

#include <list>
#include <iostream>

namespace Mantid
{
namespace Geometry
{
  /** SymmetryOperationFactory

    A factory for symmetry operations. Symmetry operations are stored
    with respect to their identifier (see SymmetryOperations for details).

    Creation of symmetry operations is then performed like this:

        SymmetryOperations_sptr inversion = SymmetryOperationFactory::Instance().createSymOp("-1");

    Available symmetry operations may be queried with DynamicFactory::getKeys().

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
  class MANTID_GEOMETRY_DLL SymmetryOperationFactoryImpl : public Kernel::DynamicFactory<const SymmetryOperation>
  {
  public:
      SymmetryOperation_const_sptr createSymOp(const std::string &identifier) const;

      /// Subscribes a symmetry operation into the factory
      template <class C>
      void subscribeSymOp()
      {
          Kernel::Instantiator<const C, const SymmetryOperation> *instantiator = new Kernel::Instantiator<const C, const SymmetryOperation>;
          SymmetryOperation_const_sptr temporarySymOp = instantiator->createInstance();

          subscribe(temporarySymOp->identifier(), instantiator);
      }

      /// Unsubscribes a symmetry operation from the factory
      void unsubscribeSymOp(const std::string &identifier)
      {
          unsubscribe(identifier);
      }

  private:
      friend struct Mantid::Kernel::CreateUsingNew<SymmetryOperationFactoryImpl>;

      SymmetryOperationFactoryImpl();
  };

// This is taken from FuncMinimizerFactory
#ifdef _WIN32
    template class MANTID_GEOMETRY_DLL Mantid::Kernel::SingletonHolder<SymmetryOperationFactory>;
#endif

typedef Mantid::Kernel::SingletonHolder<SymmetryOperationFactoryImpl> SymmetryOperationFactory;


} // namespace Geometry
} // namespace Mantid

#define DECLARE_SYMMETRY_OPERATION(classname) \
        namespace { \
    Mantid::Kernel::RegistrationHelper register_symop_##classname( \
  ((Mantid::Geometry::SymmetryOperationFactory::Instance().subscribeSymOp<classname>()) \
    , 0)); \
    }

#endif  /* MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORY_H_ */
