#ifndef MANTID_API_CONCRETEPEAKTRANSFORMFACTORY_H_
#define MANTID_API_CONCRETEPEAKTRANSFORMFACTORY_H_

#include "MantidAPI/PeakTransformFactory.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace API {
/**
@class ConcretePeakTransformFactory
Concrete PeakTransformFactory producing PeakTransforms of type provided by type
arguement
*/
template <typename PeakTransformProduct>
class DLLExport ConcretePeakTransformFactory : public PeakTransformFactory {
public:
  /**
  Constructor
  */
  ConcretePeakTransformFactory() {}

  /**
  Overriden Factory Method.
  @param xPlotLabel : X-axis plot label
  @param yPlotLabel : Y-axis plot label
  */
  virtual PeakTransform_sptr
  createTransform(const std::string &xPlotLabel,
                  const std::string &yPlotLabel) const {
    return boost::make_shared<PeakTransformProduct>(xPlotLabel, yPlotLabel);
  }

  /**
  Overriden Factory Method.
  */
  virtual PeakTransform_sptr createDefaultTransform() const {
    return boost::make_shared<PeakTransformProduct>();
  }

  /// Destructor
  virtual ~ConcretePeakTransformFactory() {}
};
}
}

#endif
