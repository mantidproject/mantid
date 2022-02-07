// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*


namespace Crystal * PeakHKLOffsets.h
*
*  Created on: May 13, 2013
*      Author: ruth
*/
/**
Shows integer offsets  for each peak of their h,k and l values, along with max
offset and run number and detector number

@author Ruth Mikkelson, SNS, ORNL
@date 05/13/2013
*/
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {
class MANTID_CRYSTAL_DLL ShowPeakHKLOffsets : public API::Algorithm {
public:
  const std::string name() const override { return "ShowPeakHKLOffsets"; };

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return " Histograms, scatter plots, etc. of this data could be useful to "
           "detect calibration problems";
  }

  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"StatisticsOfPeaksWorkspace"}; }

  const std::string category() const override { return "Crystal\\Peaks"; };

private:
  void init() override;

  void exec() override;
};
} // namespace Crystal
} // namespace Mantid
