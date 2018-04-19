#ifndef MANTID_ALGORITHMS_SOFQW_COMMON_H_
#define MANTID_ALGORITHMS_SOFQW_COMMON_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDetector.h"
// Routines used by all SofQW algorithms intended to provide united
// user interface to all SofQ algorihtms.
namespace Mantid {
namespace Algorithms {

struct DLLExport SofQCommon {

  /// E Mode
  int m_emode;
  /// EFixed has been provided
  bool m_efixedGiven;
  /// EFixed
  double m_efixed;

  // Constructor
  SofQCommon() : m_emode(0), m_efixedGiven(false), m_efixed(0.0) {}
  // init the class parameters, defined above
  void initCachedValues(const API::MatrixWorkspace &workspace,
                        API::Algorithm *const hostAlgorithm);

  /// Get the efixed value for the given detector
  double getEFixed(const Geometry::IDetector &det) const;

  /// Get global minimum and maximum X from given ws.
  std::pair<double, double> eBinHints(const API::MatrixWorkspace &ws) const;
  /// Estimate minimum and maximum momentum transfer.
  std::pair<double, double> qBinHints(const API::MatrixWorkspace &ws, const double minE, const double maxE) const;

};
}
}

#endif
