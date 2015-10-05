#ifndef MANTID_ALGORITHMS_SAVENISTDAT_H_
#define MANTID_ALGORITHMS_SAVENISTDAT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/**
    Writer for compatibility with SansView and NIST reduced data file format.
    Writes I(Qx,Qy), the output of Qxy, to an ASCII file.

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The reduced data in units of Q. </LI>
    <LI> Filename          - Output file. </LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SaveNISTDAT : public API::Algorithm {
public:
  /// (Empty) Constructor
  SaveNISTDAT() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SaveNISTDAT() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SaveNISTDAT"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Save I(Qx,Qy) data to a text file compatible with NIST and DANSE "
           "readers.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "SANS;DataHandling\\Text";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SAVENISTDAT_H_*/
