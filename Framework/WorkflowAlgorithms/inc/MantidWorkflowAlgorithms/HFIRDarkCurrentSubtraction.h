#ifndef MANTID_ALGORITHMS_HFIRDARKCURRENTSUBTRACTION_H_
#define MANTID_ALGORITHMS_HFIRDARKCURRENTSUBTRACTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Subtract dark current for HFIR SANS.

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
class DLLExport HFIRDarkCurrentSubtraction : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override {
    return "HFIRDarkCurrentSubtraction";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Perform HFIR SANS dark current subtraction.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Workflow\\SANS\\UsesPropertyManager";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  double getCountingTime(API::MatrixWorkspace_sptr inputWS);

  static const int DEFAULT_MONITOR_ID = 0;
  static const int DEFAULT_TIMER_ID = 1;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_HFIRDarkCurrentSubtraction_H_*/
