// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_PARCOMPONENT_FACTORY_H_
#define MANTID_GEOMETRY_PARCOMPONENT_FACTORY_H_
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace Geometry {
class ParameterMap;
class IComponent;
class IDetector;
class Detector;
class Instrument;

/**
  @brief A Factory for creating Parameterized component
  from their respective non-parameterized objects.
  @author Nicholas Draper, ISIS RAL
  @date 20/10/2009
*/
class MANTID_GEOMETRY_DLL ParComponentFactory {
public:
  /// Create a parameterized detector from the given base component and
  /// ParameterMap and
  /// return a shared_ptr<IDetector>
  static boost::shared_ptr<IDetector> createDetector(const IDetector *base,
                                                     const ParameterMap *map);

  /// Create a parameterized instrument from the given base and ParameterMap
  static boost::shared_ptr<Instrument>
  createInstrument(boost::shared_ptr<const Instrument> base,
                   boost::shared_ptr<ParameterMap> map);
  /// Create a parameterized component from the given base component and
  /// ParameterMap
  static boost::shared_ptr<IComponent>
  create(boost::shared_ptr<const IComponent> base, const ParameterMap *map);
};

} // Namespace Geometry
} // Namespace Mantid

#endif
