#ifndef MANTID_ALGORITHMS_CROSSCORRELATE_H_
#define MANTID_ALGORITHMS_CROSSCORRELATE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectraAxis.h"

namespace Mantid
{
namespace Algorithms
{
/** Compute the cross correlation function for a range of spectra with respect to a reference spectrum.
 * This is use in powder diffraction experiments when trying to estimate the offset of one spectra
 * with respect to another one. The spectra are converted in d-spacing and then interpolate
 * on the X-axis of the reference. The cross correlation function is computed in the range [-N/2,N/2]
 * where N is the number of points.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace. </LI>
    <LI> ReferenceSpectra - The spectra number against which cross-correlation function is computed.</LI>
    <LI> Spectra_min  - Lower bound of the spectra range for which cross-correlation is computed.</LI>
    <LI> Spectra_max - Upper bound of the spectra range for which cross-correlation is computed.</LI>
    </UL>

    @author Laurent C Chapon, ISIS Facility Rutherford Appleton Laboratory
    @date 15/12/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CrossCorrelate : public API::Algorithm
{
public:
  /// (Empty) Constructor
  CrossCorrelate() : API::Algorithm(),m_progress(NULL) {}
  /// Virtual destructor
  virtual ~CrossCorrelate() {if(m_progress) delete m_progress;m_progress=NULL;}
  /// Algorithm's name
  virtual const std::string name() const { return "CrossCorrelate"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Cross-correlates a range of spectra against one reference spectra in the same workspace.";}

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Utility;Arithmetic"; }

private:
  
  /// Initialisation code
  void init();
  ///Execution code
  void exec();
  /// Spectra to index map
  spec2index_map index_map;
  /// Iterator for the spectra to index map
  spec2index_map::iterator index_map_it;

  /// Progress reporting
  API::Progress* m_progress;
};


// Functor for vector sum

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CROSSCORRELATE_H_*/
