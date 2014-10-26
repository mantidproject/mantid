#ifndef MANTID_GEOMETRY_SCATTERERFACTORY_H_
#define MANTID_GEOMETRY_SCATTERERFACTORY_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidGeometry/Crystal/IScatterer.h"

namespace Mantid
{
namespace Geometry
{

/** ScattererFactory :

    This class implements a factory for concrete IScatterer classes.
    When a new scatterer is derived from IScatterer, it should be registered
    in the factory. Like other factories in Mantid, a macro is provided for
    this purpose:

        DECLARE_SCATTERER(NewScattererClass)

    At runtime, instances of this class can be created like this:

        IScatterer_sptr scatterer = ScattererFactory::Instance().createScatterer("NewScattererClass");

    The returned object is initialized, which is required for using the
    Kernel::Property-based system of setting parameters for the scatterer.
    To make creation of scatterers more convenient, it's possible to provide
    a string with "name=value" pairs, separated by semi-colons, which assigns
    property values. This is similar to the way FunctionFactory::createInitialized works:

        IScatterer_sptr s = ScattererFactory::Instance()
                                               .createScatterer(
                                                    "NewScatterer",
                                                    "SpaceGroup=F m -3 m; Position=[0.1,0.2,0.3]");

    If you choose to use the raw create/createUnwrapped methods, you have to
    make sure to call IScatterer::initialize() on the created instance.

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
class MANTID_GEOMETRY_DLL ScattererFactoryImpl : public Kernel::DynamicFactory<IScatterer>
{
public:
    IScatterer_sptr createScatterer(const std::string &name, const std::string &properties = "");

    /// Subscribes a scatterer class into the factory.
    template<class C>
    void subscribeScatterer()
    {
        Kernel::Instantiator<C, IScatterer> *instantiator = new Kernel::Instantiator<C, IScatterer>;
        IScatterer_sptr scatterer = instantiator->createInstance();

        subscribe(scatterer->name(), instantiator);
    }

private:
    friend struct Mantid::Kernel::CreateUsingNew<ScattererFactoryImpl>;

    ScattererFactoryImpl();
};

// This is taken from FuncMinimizerFactory
#ifdef _WIN32
    template class MANTID_GEOMETRY_DLL Mantid::Kernel::SingletonHolder<ScattererFactoryImpl>;
#endif

typedef Mantid::Kernel::SingletonHolder<ScattererFactoryImpl> ScattererFactory;


} // namespace Geometry
} // namespace Mantid

#define DECLARE_SCATTERER(classname) \
        namespace { \
    Mantid::Kernel::RegistrationHelper register_scatterer_##classname( \
  ((Mantid::Geometry::ScattererFactory::Instance().subscribeScatterer<classname>()) \
    , 0)); \
    }

#endif  /* MANTID_GEOMETRY_ISCATTERERFACTORY_H_ */
