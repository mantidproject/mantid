#ifndef MANTID_GEOMETRY_BRAGGSCATTERERFACTORY_H_
#define MANTID_GEOMETRY_BRAGGSCATTERERFACTORY_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidGeometry/Crystal/BraggScatterer.h"

namespace Mantid {
namespace Geometry {

/**
    @class BraggScattererFactory

    This class implements a factory for concrete BraggScatterer classes.
    When a new scatterer is derived from BraggScatterer, it should be registered
    in the factory. Like other factories in Mantid, a macro is provided for
    this purpose:

        DECLARE_BRAGGSCATTERER(NewScattererClass)

    At runtime, instances of this class can be created like this:

        BraggScatterer_sptr scatterer =
        BraggScattererFactory::Instance().createScatterer("NewScattererClass");

    The returned object is initialized, which is required for using the
    Kernel::Property-based system of setting parameters for the scatterer.
    To make creation of scatterers more convenient, it's possible to provide
    a string with "name=value" pairs, separated by semi-colons, which assigns
    property values. This is similar to the way
    FunctionFactory::createInitialized works:

        BraggScatterer_sptr s = BraggScattererFactory::Instance()
                                               .createScatterer(
                                                    "NewScatterer",
                                                    "SpaceGroup=F m -3 m;"
                                                    "Position=[0.1,0.2,0.3]");

    If you choose to use the raw create/createUnwrapped methods, you have to
    make sure to call BraggScatterer::initialize() on the created instance.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 26/10/2014

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
class MANTID_GEOMETRY_DLL BraggScattererFactoryImpl
    : public Kernel::DynamicFactory<BraggScatterer> {
public:
  BraggScatterer_sptr createScatterer(const std::string &name,
                                      const std::string &properties = "");

  /// Subscribes a scatterer class into the factory.
  template <class C> void subscribeScatterer() {
    Kernel::Instantiator<C, BraggScatterer> *instantiator =
        new Kernel::Instantiator<C, BraggScatterer>;
    BraggScatterer_sptr scatterer = instantiator->createInstance();

    subscribe(scatterer->name(), instantiator);
  }

private:
  friend struct Mantid::Kernel::CreateUsingNew<BraggScattererFactoryImpl>;

  BraggScattererFactoryImpl();
};

// This is taken from FuncMinimizerFactory
#ifdef _WIN32
template class MANTID_GEOMETRY_DLL
    Mantid::Kernel::SingletonHolder<BraggScattererFactoryImpl>;
#endif

typedef Mantid::Kernel::SingletonHolder<BraggScattererFactoryImpl>
    BraggScattererFactory;

} // namespace Geometry
} // namespace Mantid

#define DECLARE_BRAGGSCATTERER(classname)                                      \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_scatterer_##classname(           \
      ((Mantid::Geometry::BraggScattererFactory::Instance()                    \
            .subscribeScatterer<classname>()),                                 \
       0));                                                                    \
  }

#endif /* MANTID_GEOMETRY_BRAGGSCATTERERFACTORY_H_ */
