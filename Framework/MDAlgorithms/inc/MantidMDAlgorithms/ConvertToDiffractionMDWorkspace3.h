// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
class MANTID_MDALGORITHMS_DLL ConvertToDiffractionMDWorkspace3
    : public MDAlgorithms::BaseConvertToDiffractionMDWorkspace {
public:
  /// Algorithm's version for identification
  int version() const override { return 3; }
  const std::vector<std::string> seeAlso() const override { return {"ConvertToMD", "SetSpecialCoordinates"}; }

private:
  void init() override;

private:
  // method to covnert extents to the properties of ConvertMD
  void convertExtents(const std::vector<double> &Extents, std::vector<double> &minVal,
                      std::vector<double> &maxVal) override;

  // method to calculate the extents of the data from the input workspace
  void calculateExtentsFromData(std::vector<double> &minVal, std::vector<double> &maxVal);
};

} // namespace MDAlgorithms
} // namespace Mantid
