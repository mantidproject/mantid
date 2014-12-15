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
/** This algorithm calls FindPeaks as a ChildAlgorithm and then subtracts
    all the peaks found from the data, leaving just the 'background'.

    *** IT IS ASSUMED THAT THE FITTING FUNCTION WAS A GAUSSIAN ***

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace. </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> fwhm - passed to the FindPeaks ChildAlgorithm</LI>
    <LI> Tolerance - passed to the FindPeaks ChildAlgorithm</LI>
    <LI> WorkspaceIndex - The spectrum from which to remove peaks. Will search all spectra if absent.</LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 30/10/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport StripPeaks : public API::Algorithm
{
public:
  /// (Empty) Constructor
  StripPeaks();
  /// Virtual destructor
  virtual ~StripPeaks() {}
  /// Algorithm's name
  virtual const std::string name() const { return "StripPeaks"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "This algorithm attempts to find all the peaks in all spectra of a workspace and subtract them from the data, leaving just the 'background'.";}

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "CorrectionFunctions;Optimization\\PeakFinding"; }

private:
  
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  API::ITableWorkspace_sptr findPeaks(API::MatrixWorkspace_sptr WS);
  API::MatrixWorkspace_sptr removePeaks(API::MatrixWorkspace_const_sptr input, API::ITableWorkspace_sptr peakslist);
  double m_maxChiSq;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_STRIPPEAKS_H_*/
