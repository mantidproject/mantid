// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_APPLYCALIBRATION_H_
#define MANTID_DATAHANDLING_APPLYCALIBRATION_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**

Update detector positions from input table workspace. The positions are updated
as absolute positions and so this update can be repeated.


@author Karl Palmen
*/
class DLLExport ApplyCalibration : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "ApplyCalibration"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Update detector positions from input table workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Instrument";
  } // Needs to change

private:
  /// Overwrites Algorithm method. Does nothing at present
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_APPLYCALIBRATION_H_*/
