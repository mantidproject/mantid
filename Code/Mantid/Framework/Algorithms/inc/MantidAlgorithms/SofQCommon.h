#ifndef MANTID_ALGORITHMS_SOFQW_COMMON_H_
#define MANTID_ALGORITHMS_SOFQW_COMMON_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IDetector.h"
// Two small routines used by all SofQW algorithms intended to provide united
// user interface to all SofQ algorihtms.
namespace Mantid {
namespace Algorithms {

struct SofQCommon {

  /// E Mode
  int m_emode;
  /// EFixed has been provided
  bool m_efixedGiven;
  /// EFixed
  double m_efixed;

  // Constructor
  SofQCommon() : m_emode(0), m_efixedGiven(false), m_efixed(0.0) {}
  // init the class parameters, defined above
  void initCachedValues(API::MatrixWorkspace_const_sptr workspace,
                        API::Algorithm *const hostAlgorithm);

  /// Get the efixed value for the given detector
  double getEFixed(Geometry::IDetector_const_sptr det) const;
};
}
}

#endif
