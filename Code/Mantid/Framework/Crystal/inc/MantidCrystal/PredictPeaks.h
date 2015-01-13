#ifndef MANTID_CRYSTAL_PREDICTPEAKS_H_
#define MANTID_CRYSTAL_PREDICTPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidKernel/System.h"
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Crystal {

/** Using a known crystal lattice and UB matrix, predict where single crystal
 *peaks
 * should be found in detector/TOF space. Creates a PeaksWorkspace containing
 * the peaks at the expected positions.
 *
 * @author Janik Zikovsky
 * @date 2011-04-29 16:30:52.986094
 */
class DLLExport PredictPeaks : public API::Algorithm {
public:
  PredictPeaks();
  ~PredictPeaks();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "PredictPeaks"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Using a known crystal lattice and UB matrix, predict where single "
           "crystal peaks should be found in detector/TOF space. Creates a "
           "PeaksWorkspace containing the peaks at the expected positions.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Crystal"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  void doHKL(const double h, const double k, const double l, bool doFilter);

private:
  /// Reflection conditions possible
  std::vector<Mantid::Geometry::ReflectionCondition_sptr> m_refConds;

  /// Run number of input workspace
  int runNumber;
  /// Min wavelength parameter
  double wlMin;
  /// Max wavelength parameter
  double wlMax;
  /// Instrument reference
  Geometry::Instrument_const_sptr inst;
  /// Output peaks workspace
  Mantid::DataObjects::PeaksWorkspace_sptr pw;
  /// Counter of possible peaks
  size_t numInRange;
  /// Crystal applied
  Geometry::OrientedLattice crystal;
  /// Min D spacing to apply.
  double minD;
  /// Max D spacing to apply.
  double maxD;
  /// HKL->Q matrix (Goniometer * UB)
  Mantid::Kernel::DblMatrix mat;
  /// Goniometer rotation matrix
  Mantid::Kernel::DblMatrix gonio;
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_PREDICTPEAKS_H_ */
