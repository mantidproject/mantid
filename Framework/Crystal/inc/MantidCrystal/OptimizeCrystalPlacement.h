// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * OptimizeCrystalPlacement.h
 *
 *  Created on: Jan 26, 2013
 *      Author: ruth
 */

#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {

/** OptimizeCrystalPlacement

Description:
This algorithm basically indexes peaks with the crystal orientation matrix
stored in the peaks workspace.
The optimization is on the goniometer settings for the runs in the peaks
workspace and also the sample
orientation .
@author Ruth Mikkelson, SNS,ORNL
@date 01/26/2013
*/
class MANTID_CRYSTAL_DLL OptimizeCrystalPlacement : public API::Algorithm {
public:
  const std::string name() const override { return "OptimizeCrystalPlacement"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm  optimizes goniometer settings  and sample "
           "orientation to better index the peaks.";
  }

  int version() const override { return 1; };

  const std::string category() const override { return "Crystal\\Corrections"; };

private:
  void init() override;

  void exec() override;
};
} // namespace Crystal
} // namespace Mantid
