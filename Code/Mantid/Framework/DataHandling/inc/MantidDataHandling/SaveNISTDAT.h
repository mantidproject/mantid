#ifndef MANTID_ALGORITHMS_SAVENISTDAT_H_
#define MANTID_ALGORITHMS_SAVENISTDAT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/**
    Writer for compatibility with SansView and NIST reduced data file format.
    Writes a SANS 2D data file in Q space. If multiple TOF or wavelength bins are part of the
    workspace, they will be summed up in Q.

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The (partly) corrected data in units of wavelength. </LI>
    <LI> Filename          - Output file. </LI>
    <LI> OutputBinning     - The bin parameters to use for the final result. </LI>
    </UL>

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SaveNISTDAT : public API::Algorithm
{
public:
  /// (Empty) Constructor
  SaveNISTDAT() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SaveNISTDAT() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SaveNISTDAT"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SAVENISTDAT_H_*/
