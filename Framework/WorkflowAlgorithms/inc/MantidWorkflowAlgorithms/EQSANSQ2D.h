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
  /// (Empty) Constructor
  EQSANSQ2D() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~EQSANSQ2D() {}
  /// Algorithm's name
  virtual const std::string name() const { return "EQSANSQ2D"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Workflow algorithm to process a reduced EQSANS workspace and "
           "produce I(Qx,Qy).";
  }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow\\SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSQ2D_H_*/
