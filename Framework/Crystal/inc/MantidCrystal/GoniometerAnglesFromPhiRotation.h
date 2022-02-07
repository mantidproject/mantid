// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * GoniometerAnglesFromPhiRotation.h
 *
 *  Created on: Apr 15, 2013
 *      Author: ruth
 */
/**
 Finds Goniometer angles for a 2nd run with the same chi and omega rotationa and
 only phi rotation changes by a specified amount.

 @author Ruth Mikkelson, SNS, ORNL
 @date 04/15/2013
 */
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Crystal {
class MANTID_CRYSTAL_DLL GoniometerAnglesFromPhiRotation : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "GoniometerAnglesFromPhiRotation"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The 2nd PeaksWorkspace is set up with the correct sample "
           "orientations and UB matrices";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"SetGoniometer"}; }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Goniometer"; }

private:
  /// Initialise the properties
  void init() override;
  Kernel::Matrix<double> getUBRaw(const Kernel::Matrix<double> &UB,
                                  const Kernel::Matrix<double> &GoniometerMatrix) const;

  bool CheckForOneRun(const DataObjects::PeaksWorkspace_sptr &Peaks, Kernel::Matrix<double> &GoniometerMatrix) const;

  void IndexRaw(const DataObjects::PeaksWorkspace_sptr &Peaks, const Kernel::Matrix<double> &UBraw, int &Nindexed,
                double &AvErrIndexed, double &AvErrorAll, double tolerance) const;

  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid
