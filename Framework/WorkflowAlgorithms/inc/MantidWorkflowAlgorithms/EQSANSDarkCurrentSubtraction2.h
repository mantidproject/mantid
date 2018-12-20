#ifndef MANTID_ALGORITHMS_EQSANSDarkCurrentSubtraction2_H_
#define MANTID_ALGORITHMS_EQSANSDarkCurrentSubtraction2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Subtract dark current for EQSANS.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> Filename      - Data file to use as dark current</LI>
    <LI> ReductionTableWorkspace - Table workspace to use to keep track of the
   reduction</LI>
    <LI> OutputDarkCurrentWorkspace - Name of dark current workspace</LI>
    <LI> OutputMessage - Human readable output message</LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport EQSANSDarkCurrentSubtraction2 : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override {
    return "EQSANSDarkCurrentSubtraction";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Perform EQSANS dark current subtraction.";
  }
  /// Algorithm's version
  int version() const override { return (2); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Workflow\\SANS\\UsesPropertyManager";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSDarkCurrentSubtraction2_H_*/
