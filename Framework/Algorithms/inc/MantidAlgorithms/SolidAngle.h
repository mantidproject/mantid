// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SOLIDANDGLE_H_
#define MANTID_ALGORITHMS_SOLIDANDGLE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Algorithms {
/** Calculates and outputs the solid angles for each detector in the instrument.
    The sample position is taken as a point source for the solid angle
   calculations.
    @author Nick Draper, Tessella Support Services plc
    @date 26/01/2009
*/
class DLLExport SolidAngle : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SolidAngle"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The SolidAngle algorithm calculates the solid angle in steradians "
           "for each of the detectors in an instrument and outputs the data in "
           "a workspace.  This can then be used to normalize a data workspace "
           "using the divide algorithm should you wish.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Divide"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "CorrectionFunctions\\InstrumentCorrections";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  void initSpectrum(const API::MatrixWorkspace &input,
                    API::MatrixWorkspace &output, const size_t j);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SOLIDANDGLE_H_*/
