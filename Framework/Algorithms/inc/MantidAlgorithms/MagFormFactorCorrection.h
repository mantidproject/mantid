// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MAGFORMFACTORCORRECTION_H_
#define MANTID_ALGORITHMS_MAGFORMFACTORCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** MagneticFormFactors scales the input workspace by the 1/F(Q) where F(Q)
    is the magnetic form factor for the given magnetic ion.

    Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as
   the input one. </LI>
    <LI> MagneticIon     - The name of the magnetic ion (e.g. Fe2 for Fe2+)
   </LI>
    </UL>

    @author Manh Duc Le, STFC
    @date 08/09/2016
*/
class DLLExport MagFormFactorCorrection : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "MagFormFactorCorrection"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "MagFormFactorCorrection corrects a workspace for the magnetic form "
           "factor F(Q) by dividing S(Q,w) by F(Q)^2.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SofQW"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "CorrectionFunctions"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_MAGFORMFACTORCORRECTION_H_*/
