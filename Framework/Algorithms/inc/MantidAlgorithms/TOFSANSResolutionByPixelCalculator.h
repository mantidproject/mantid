// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_TOFSANSRESOLUTIONBYPIXELCALCULATOR_H_
#define MANTID_ALGORITHMS_TOFSANSRESOLUTIONBYPIXELCALCULATOR_H_
#include "MantidKernel/System.h"
namespace Mantid {
namespace Algorithms {

/**Helper class which provides the uncertainty calculations for the
TOFSANSResolutionByPixel class
*/
class DLLExport TOFSANSResolutionByPixelCalculator {
public:
  double getWavelengthIndependentFactor(double r1, double r2, double deltaR,
                                        double lCollim, double l2) const;
  double getSigmaQValue(double moderatorValue,
                        double wavlengthIndependentFactor, double q,
                        double wavelength, double deltaWavelength, double l1,
                        double l2) const;
};
} // namespace Algorithms
} // namespace Mantid

#endif