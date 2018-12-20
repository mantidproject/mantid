#ifndef MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE2_H_
#define MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE2_H_

#include "MantidMDAlgorithms/BaseConvertToDiffractionMDWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertToDiffractionMDWorkspace2 :
 * Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from
 *an input EventWorkspace.
 *
 * This is the wrapper for ConvertToMD algorithm transferring the properties of
 *the old ConvertToDiffractionMDWorkspace into ConvertToMD properties
 * and running ConvertToMD as a subalgorithm.
 *
 * @date 2013-05-20
 */
class DLLExport ConvertToDiffractionMDWorkspace2
    : public MDAlgorithms::BaseConvertToDiffractionMDWorkspace {
public:
  /// Algorithm's version for identification
  int version() const override { return 2; }

private:
  void init() override;

protected: // for testing
  // method to convert the extents specified for the
  // ConvertToDiffractionMDWorksapce  into the min-max properties names of the
  // ConvertToMD
  void convertExtents(const std::vector<double> &Extents,
                      std::vector<double> &minVal,
                      std::vector<double> &maxVal) override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE_H_ */
