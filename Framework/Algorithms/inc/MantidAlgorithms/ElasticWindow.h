// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** @author Michael Whitty, STFC ISIS
    @date 25/10/2010

    This algorithm uses the Integration, ConvertSpectrumAxis and Transpose
   algorithms
    to provide an integrated value over q and q^2..
*/
class MANTID_ALGORITHMS_DLL ElasticWindow : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ElasticWindow"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm performs an integration over an energy range, with "
           "the option to subtract a background over a second range, then "
           "transposes the result into a single-spectrum workspace with units "
           "in Q and Q^2.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"Integration"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Inelastic\\Indirect"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
