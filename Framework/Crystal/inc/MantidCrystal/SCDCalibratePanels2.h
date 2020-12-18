// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"


namespace Mantid {
namespace Crystal {

/** SCDCalibratePanels2 :
 * Using input peakworkspace with indexation results to calibrate each
 * individual panels.
 * The target calibration properties include:
 * - T0: sec?
 *       time for proton to travel from reactor to target to generate neutron 
 * - L1: meters
 *       distance between target and sample
 * - L2: meters (also known as z_shift)
 *       distance between sample and the center of each panel
 * - Rot: degrees
 *       Euler angles (xyz )
*/
class MANTID_CRYSTAL_DLL SCDCalibratePanels2 : public Mantid::API::Algorithm{
public:

    const std::string name() const override {
        return "SCDCalibratePanels2";
    }

    const std::string summary() const override {
    return "Panel parameters and L0 are optimized to "
           "minimize errors between theoretical and actual q values for the "
           "peaks";
    }

    int version() const override {return 1;}

    const std::string category() const override {
        return "Crystal\\Corrections";
    }

    const std::vector<std::string> seeAlso() const override {
        return {"CalculateUMatrix"};
    }

private:
    /// Overwrites Algorithm method. Does nothing at present
    void init() override;

    /// Overwrites Algorithm method
    void exec() override;

    /// Private validator for inputs
    std::map<std::string, std::string> validateInputs() override;

};

} // namespace Crystal
} // namespace Mantid
