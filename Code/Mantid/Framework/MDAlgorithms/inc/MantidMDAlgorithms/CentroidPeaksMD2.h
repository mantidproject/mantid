#ifndef MANTID_MDALGORITHMS_CENTROIDPEAKSMD2_H_
#define MANTID_MDALGORITHMS_CENTROIDPEAKSMD2_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** Find the centroid of single-crystal peaks in a MDEventWorkspace, in order to
 *refine their positions.
 *
 * @author Janik Zikovsky
 * @date 2011-06-01
 */
class DLLExport CentroidPeaksMD2 : public API::Algorithm {
public:
  CentroidPeaksMD2();
  ~CentroidPeaksMD2();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "CentroidPeaksMD"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Find the centroid of single-crystal peaks in a MDEventWorkspace, "
           "in order to refine their positions.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 2; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  template <typename MDE, size_t nd>
  void integrate(typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Input MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr inWS;
};

} // namespace Mantid
} // namespace MDEvents

#endif /* MANTID_MDEVENTS_CENTROIDPEAKSMD2_H_ */
