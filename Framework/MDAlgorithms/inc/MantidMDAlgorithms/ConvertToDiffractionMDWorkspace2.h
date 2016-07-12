#ifndef MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE2_H_
#define MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE2_H_

#include "MantidMDAlgorithms/BoxControllerSettingsAlgorithm.h"

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
    : public MDAlgorithms::BoxControllerSettingsAlgorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override {
    return "ConvertToDiffractionMDWorkspace";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, "
           "Qz) for an elastic diffraction experiment.";
  }

  /// Algorithm's version for identification
  int version() const override { return 2; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\Creation";
  }

private:
  void init() override;
  void exec() override;

  // the target frame names exposed as the algorithm properties and recognized
  // by old convertToDiffractionWorkspace algorithm.
  std::vector<std::string> frameOptions;

protected: // for testing
  // method to convert the value of the target frame specified for the
  // ConvertToDiffractionMDWorksapce  into the properties names of the
  // ConvertToMD
  void convertFramePropertyNames(const std::string &TargFrame,
                                 std::string &TargFrameName,
                                 std::string &ScalingName);
  // method to convert the extents specified for the
  // ConvertToDiffractionMDWorksapce  into the min-max properties names of the
  // ConvertToMD
  void convertExtents(const std::vector<double> &Extents,
                      std::vector<double> &minVal,
                      std::vector<double> &maxVal) const;
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE_H_ */
