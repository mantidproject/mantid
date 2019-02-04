// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CONVERTFROMDISTRIBUTION_H_
#define MANTID_ALGORITHMS_CONVERTFROMDISTRIBUTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Converts a histogram workspace from a distribution. i.e. multiplies by the
   bin width

    Required Properties:
    <UL>
    <LI> Workspace - The name of the Workspace to convert.</LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 17/11/2008
*/
class DLLExport ConvertFromDistribution : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertFromDistribution"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts a histogram workspace from a distribution i.e. multiplies "
           "by the bin width.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToDistribution"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Transforms\\Distribution";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTFROMDISTRIBUTION_H_*/
