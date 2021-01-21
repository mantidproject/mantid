// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_poly.h>

namespace Mantid {
namespace Crystal {

/** LoadHKL : Load an ISAW-style .hkl file
 * into a PeaksWorkspace
 *
 * @author Vickie Lynch, SNS
 * @date 2012-01-25
 */
class MANTID_CRYSTAL_DLL LoadHKL : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadHKL"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads an ASCII .hkl file to a PeaksWorkspace."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SaveHKL"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\DataHandling;DataHandling\\Text"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid
