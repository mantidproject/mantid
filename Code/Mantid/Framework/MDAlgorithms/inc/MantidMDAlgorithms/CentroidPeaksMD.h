#ifndef MANTID_MDALGORITHMS_CENTROIDPEAKSMD_H_
#define MANTID_MDALGORITHMS_CENTROIDPEAKSMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** Find the centroid of single-crystal peaks in a MDEventWorkspace, in order to
 *refine their positions.
 *
 * @author Janik Zikovsky
 * @date 2011-06-01
 */
class DLLExport CentroidPeaksMD : public API::Algorithm {
public:
  CentroidPeaksMD();
  ~CentroidPeaksMD();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "CentroidPeaksMD"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Find the centroid of single-crystal peaks in a MDEventWorkspace, "
           "in order to refine their positions.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  template <typename MDE, size_t nd>
  void integrate(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Input MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr inWS;
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_MDALGORITHMS_CENTROIDPEAKSMD_H_ */
