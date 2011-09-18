#ifndef MANTID_ALGORITHMS_SOLIDANDGLE_H_
#define MANTID_ALGORITHMS_SOLIDANDGLE_H_
/*WIKI* 


The algorithm calculates solid angles from the sample position of the input workspace for all of the spectra selected.  If several detectors have been mapped to the same spectrum then the solid angles of this detectors will be summed to provide the solid angle for the spectrum.
The solid angle of a detector that has been masked or marked as dead is considered to be 0 steradians.

This algorithms can happily accept [[Ragged Workspace|ragged workspaces]] as an input workspace.  The result would be a ragged output workspace whose X axis values match the lowest and highest of each the input spectra.

Note:  The Solid angle calculation assumes that the path between the sample and detector is unobstructed by another other instrument components.



*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Calculates and outputs the solid angles for each detector in the instrument.  
    The sample position is taken as a point source for the solid angle calculations.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as the input one. </LI>
    </UL>

    @author Nick Draper, Tessella Support Services plc
    @date 26/01/2009

    Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SolidAngle : public API::Algorithm
{
public:
  /// Default constructor
  SolidAngle();
  /// Virtual destructor
  virtual ~SolidAngle();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SolidAngle"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SOLIDANDGLE_H_*/
