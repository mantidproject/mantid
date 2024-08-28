// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {

/** SetUB : Algorithm to set the UB matrix, given lattice parameters and u and v
  vectors as defined in:
  http://horace.isis.rl.ac.uk/Getting_started


  @author Andrei Savici
  @date 2011-11-07
*/
class MANTID_CRYSTAL_DLL SetUB final : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Set the UB matrix, given either lattice parametersand orientation "
           "vectors or the UB matrix elements";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"FindUBUsingFFT", "FindUBUsingIndexedPeaks", "FindUBUsingLatticeParameters"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid
