#ifndef MANTID_ALGORITHM_SIMPLEINTEGRATION_H_
#define MANTID_ALGORITHM_SIMPLEINTEGRATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "../../Kernel/inc/Algorithm.h"
#include "../../Kernel/inc/Logger.h"

namespace Mantid
{
namespace Algorithms
{
/** @class SimpleIntegration SimpleIntegration.h Algorithms/SimpleIntegration.h

    Takes a 2D workspace as input and sums each Histogram1D contained within
    it, storing the result as a Workspace1D.
    
    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>
       
    Optional Properties (assume that you count from zero):
    <UL>
    <LI> StartX - X bin number to integrate from (default 0)</LI>
    <LI> EndX - X bin number to integrate to (default max)</LI>
    <LI> StartY - Y bin number to integrate from (default 0)</LI>
    <LI> EndY - Y bin number to integrate to (default max)</LI>
    </UL>
    
    @author Russell Taylor, Tessella Support Services plc
    @date 05/10/2007
    
    Copyright &copy; 2007 ???RAL???

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
*/
class DLLExport SimpleIntegration : public Kernel::Algorithm
{
public:
  /// Default constructor
	SimpleIntegration() {};
	/// Destructor
	virtual ~SimpleIntegration() {};
	
private:
  /** Performs the initialisation task of retrieving and setting parameters
   * 
   *  @return A StatusCode object indicating whether the operation was successful
   */
  Kernel::StatusCode init();
  
  /** Executes the algorithm
   * 
   *  @return A StatusCode object indicating whether the operation was successful
   */
  Kernel::StatusCode exec();
  
  /// Does nothing at present
  Kernel::StatusCode final();
  
  /// The X bin to start the integration from
  int m_MinX;
  /// The X bin to finish the integration at
  int m_MaxX;
  /// The Y bin to start the integration from
  int m_MinY;
  /// The Y bin to finish the integration at
  int m_MaxY;

  ///static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_SIMPLEINTEGRATION_H_*/
