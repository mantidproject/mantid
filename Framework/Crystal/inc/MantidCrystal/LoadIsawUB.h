// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Crystal {

/** Algorithm to load an ISAW-style ASCII UB matrix and lattice
 * parameters file, and place its information into
 * a workspace.
 *
 * @author Janik Zikovsky
 * @date 2011-05-25
 */
class MANTID_CRYSTAL_DLL LoadIsawUB : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadIsawUB"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load an ISAW-style ASCII UB matrix and lattice parameters file, "
           "and place its information into a workspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SaveIsawUB"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\DataHandling;DataHandling\\Isaw"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// load the modulation UB
  void readModulatedUB(std::ifstream &in, Kernel::DblMatrix &ub);
};

} // namespace Crystal
} // namespace Mantid
