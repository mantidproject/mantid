#ifndef MANTID_ALGORITHMS_EQSANSQ2D_H_
#define MANTID_ALGORITHMS_EQSANSQ2D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Workflow algorithm to process a reduced EQSANS workspace and produce
   I(Qx,Qy).
    The algorithm deals with the frame skipping option.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport EQSANSQ2D : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "EQSANSQ2D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Workflow algorithm to process a reduced EQSANS workspace and "
           "produce I(Qx,Qy).";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Workflow\\SANS"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSQ2D_H_*/
