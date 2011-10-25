#ifndef MANTID_ALGORITHMS_MASKBINS_H_
#define MANTID_ALGORITHMS_MASKBINS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** Masks bins in a workspace. Bins falling within the range given (even partially) are
    masked, i.e. their data and error values are set to zero and the bin is added to the
    list of masked bins. This range is masked for all spectra in the workspace (though the
    workspace does not have to have common X values in all spectra).

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input. </LI>
    <LI> OutputWorkspace - The name of the Workspace containing the masked bins. </LI>
    <LI> XMin - The value to start masking from.</LI>
    <LI> XMax - The value to end masking at.</LI>
    </UL>
      
    @author Russell Taylor, Tessella plc
    @date 29/04/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport MaskBins : public API::Algorithm
{
public:
  /// Constructor
  MaskBins();
  /// Virtual destructor
  virtual ~MaskBins() {}
  /// Algorithm's name
  virtual const std::string name() const { return "MaskBins"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  void execEvent();
  
  void findIndices(const MantidVec& X, MantidVec::difference_type& startBin, MantidVec::difference_type& endBin);

  double m_startX;                                   ///< The range start point
  double m_endX;                                     ///< The range end point
  std::vector<int> spectra_list; ///<the list of Spectra (workspace index) to load

};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_MASKBINS_H_*/
