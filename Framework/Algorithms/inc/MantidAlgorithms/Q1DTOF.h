#ifndef MANTID_ALGORITHMS_Q1DTOF_H_
#define MANTID_ALGORITHMS_Q1DTOF_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**
    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Q1DTOF : public API::Algorithm {
public:
  /// (Empty) Constructor
  Q1DTOF() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~Q1DTOF() {}
  /// Algorithm's name
  virtual const std::string name() const { return "Q1DTOF"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Performs azimuthal averaging on a 2D SANS data to produce I(Q).";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_Q1DTOF_H_*/
