// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_BRAGGSCATTERERFACTORY_H_
#define MANTID_GEOMETRY_BRAGGSCATTERERFACTORY_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

#include "MantidGeometry/Crystal/BraggScatterer.h"
#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"

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
  */
class MANTID_GEOMETRY_DLL BraggScattererFactoryImpl
    : public Kernel::DynamicFactory<BraggScatterer> {
public:
  BraggScatterer_sptr createScatterer(const std::string &name,
                                      const std::string &properties = "") const;

  /// Subscribes a scatterer class into the factory.
  template <class C> void subscribeScatterer() {
    auto instantiator =
        std::make_unique<Kernel::Instantiator<C, BraggScatterer>>();
    BraggScatterer_sptr scatterer = instantiator->createInstance();

    subscribe(scatterer->name(), std::move(instantiator));
  }

private:
  friend struct Mantid::Kernel::CreateUsingNew<BraggScattererFactoryImpl>;

  BraggScattererFactoryImpl();
};

using BraggScattererFactory =
    Mantid::Kernel::SingletonHolder<BraggScattererFactoryImpl>;

} // namespace Geometry
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_GEOMETRY template class MANTID_GEOMETRY_DLL Mantid::Kernel::
    SingletonHolder<Mantid::Geometry::BraggScattererFactoryImpl>;
}
} // namespace Mantid

#define DECLARE_BRAGGSCATTERER(classname)                                      \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_scatterer_##classname(           \
      ((Mantid::Geometry::BraggScattererFactory::Instance()                    \
            .subscribeScatterer<classname>()),                                 \
       0));                                                                    \
  }

#endif /* MANTID_GEOMETRY_BRAGGSCATTERERFACTORY_H_ */
