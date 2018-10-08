// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MULTIPLYRANGE_H_
#define MANTID_ALGORITHMS_MULTIPLYRANGE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** An algorithm to multiply a range of bins in a workspace by the factor given.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace</LI>
    <LI> OutputWorkspace - The name of the output workspace</LI>
    <LI> StartBin        - The bin index at the start of the range</LI>
    <LI> EndBin          - The bin index at the end of the range</LI>
    <LI> Factor          - The value by which to multiply the input data
   range</LI>
    </UL>

    @author Robert Dalgliesh, ISIS, RAL
    @date 12/1/2010
 */
class DLLExport MultiplyRange : public API::Algorithm {
public:
  const std::string name() const override { return "MultiplyRange"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "An algorithm to multiply a range of bins in a workspace by the "
           "factor given.";
  }

  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"Multiply"};
  }
  const std::string category() const override {
    return "Arithmetic;CorrectionFunctions";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_MULTIPLYRANGE_H_*/
