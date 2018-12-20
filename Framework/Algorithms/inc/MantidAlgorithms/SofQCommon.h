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

  /// Calculate the Q value
  double q(const double deltaE, const double twoTheta,
           const Geometry::IDetector *det) const;

  /// Estimate minimum and maximum momentum transfer.
  std::pair<double, double> qBinHints(const API::MatrixWorkspace &ws,
                                      const double minE,
                                      const double maxE) const;

private:
  double directQ(const double deltaE, const double twoTheta) const;
  double indirectQ(const double deltaE, const double twoTheta,
                   const Geometry::IDetector *det) const;
  std::pair<double, double> qBinHintsDirect(const API::MatrixWorkspace &ws,
                                            const double minE,
                                            const double maxE) const;
  std::pair<double, double> qBinHintsIndirect(const API::MatrixWorkspace &ws,
                                              const double minE,
                                              const double maxE) const;
};
} // namespace Algorithms
} // namespace Mantid

#endif
