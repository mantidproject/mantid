#ifndef MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE3_H_
#define MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE3_H_

#include "MantidMDAlgorithms/BaseConvertToDiffractionMDWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertToDiffractionMDWorkspace3 :
 * Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from
 * an input EventWorkspace.
 *
 * This is the wrapper for ConvertToMD algorithm transferring the properties of
 * the old ConvertToDiffractionMDWorkspace into ConvertToMD properties
 * and running ConvertToMD as a subalgorithm.
 *
 * @date 2013-05-20
 */
class DLLExport ConvertToDiffractionMDWorkspace3
    : public MDAlgorithms::BaseConvertToDiffractionMDWorkspace {
public:
  /// Algorithm's version for identification
  int version() const override { return 3; }

private:
  void init() override;

private:
  // method to covnert extents to the properties of ConvertMD
  void convertExtents(const std::vector<double> &Extents,
                      std::vector<double> &minVal,
                      std::vector<double> &maxVal) override;

  // method to calculate the extents of the data from the input workspace
  void calculateExtentsFromData(std::vector<double> &minVal,
                                std::vector<double> &maxVal);
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE3_H_ */
