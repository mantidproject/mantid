#ifndef MANTID_ALGORITHMS_EQSANSDARKCURRENTSUBTRACTION_H_
#define MANTID_ALGORITHMS_EQSANSDARKCURRENTSUBTRACTION_H_

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
class DLLExport EQSANSDarkCurrentSubtraction : public API::Algorithm {
public:
  /// (Empty) Constructor
  EQSANSDarkCurrentSubtraction() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~EQSANSDarkCurrentSubtraction() {}
  /// Algorithm's name
  virtual const std::string name() const {
    return "EQSANSDarkCurrentSubtraction";
  }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Perform EQSANS dark current subtraction.";
  }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Workflow\\SANS\\UsesPropertyManager";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSDARKCURRENTSUBTRACTION_H_*/
