#ifndef MANTID_GEOMETRY_PEAKTRANSFORMFACTORY_H_
#define MANTID_GEOMETRY_PEAKTRANSFORMFACTORY_H_

#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {
/**
@class PeakTransformFactory
Abstract type defining Factory Method interface for generating PeakTransforms
*/
class DLLExport PeakTransformFactory {
public:
  virtual PeakTransform_sptr createDefaultTransform() const = 0;
  virtual PeakTransform_sptr
  createTransform(const std::string &xPlotLabel,
                  const std::string &yPlotLabel) const = 0;
  virtual ~PeakTransformFactory() = default;
};

/// Factory Shared Pointer typedef.
using PeakTransformFactory_sptr = boost::shared_ptr<PeakTransformFactory>;
} // namespace Geometry
} // namespace Mantid

#endif
