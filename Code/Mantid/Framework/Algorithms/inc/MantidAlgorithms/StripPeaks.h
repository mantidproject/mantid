#ifndef MANTID_ALGORITHMS_STRIPPEAKS_H_
#define MANTID_ALGORITHMS_STRIPPEAKS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** This algorithm calls FindPeaks as a subalgorithm and then subtracts
    all the peaks found from the data, leaving just the 'background'.

    *** IT IS ASSUMED THAT THE FITTING FUNCTION WAS A GAUSSIAN ***

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace. </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> fwhm - passed to the FindPeaks subalgorithm</LI>
    <LI> Tolerance - passed to the FindPeaks subalgorithm</LI>
    <LI> WorkspaceIndex - The spectrum from which to remove peaks. Will search all spectra if absent.</LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 30/10/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport StripPeaks : public API::Algorithm
{
public:
  /// (Empty) Constructor
  StripPeaks();
  /// Virtual destructor
  virtual ~StripPeaks() {}
  /// Algorithm's name
  virtual const std::string name() const { return "StripPeaks"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  API::ITableWorkspace_sptr findPeaks(API::MatrixWorkspace_sptr WS);
  API::MatrixWorkspace_sptr removePeaks(API::MatrixWorkspace_const_sptr input, API::ITableWorkspace_sptr peakslist);

};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_STRIPPEAKS_H_*/
