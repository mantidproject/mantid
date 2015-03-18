#ifndef MANTID_API_PEAKTRANSFORMFACTORY_H_
#define MANTID_API_PEAKTRANSFORMFACTORY_H_

#include "MantidAPI/PeakTransform.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
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
  virtual ~PeakTransformFactory() {}
};

/// Factory Shared Pointer typedef.
typedef boost::shared_ptr<PeakTransformFactory> PeakTransformFactory_sptr;
}
}

#endif
