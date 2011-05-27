#ifndef MANTID_ALGORITHMS_Q1DTOF_H_
#define MANTID_ALGORITHMS_Q1DTOF_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/**
    Part of data reduction for SANS. Transforms a 2D detector array into I(Q) by assigning each
    2D pixel to a Q bin. The contribution of each pixel is weighted by either 1 or a function of the error
    on the signal in that pixel.

    The weighting of the pixel by the error follows the HFIR/SANS reduction code implemented in IGOR
    by Ken Littrell, ORNL.

    Choosing to weight each pixel by 1 gives I(q) where each bin is the average of all pixels contributing to
    that bin.

    Each pixel is sub-divided in NPixelDivision*NPixelDivision sub-pixels when averaging.

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The (partly) corrected data in units of wavelength. </LI>
    <LI> OutputWorkspace   - The workspace in which to store the result histogram. </LI>
    <LI> OutputBinning     - The bin parameters to use for the final result. </LI>
    <LI> NPixelDivision    - The number of pixel sub-divisions in each direction used for calculation. </LI>
    <LI> PixelSizeX        - The pixel size, in meter, in the X direction.</LI>
    <LI> PixelSizeY        - The pixel size, in meter, in the Y direction. </LI>
    <LI> ErrorWeighting    - Whether to weight pixel contribution by their error (default: false). </LI>
    </UL>

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Q1DTOF : public API::Algorithm
{
public:
  /// (Empty) Constructor
  Q1DTOF() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~Q1DTOF() {}
  /// Algorithm's name
  virtual const std::string name() const { return "Q1DTOF"; }
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

#endif /*MANTID_ALGORITHMS_Q1DTOF_H_*/
