#ifndef MANTID_ALGORITHMS_CALCULATETRANSMISSION_H_
#define MANTID_ALGORITHMS_CALCULATETRANSMISSION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{
/** Calculates the transmission correction, as a function of wavelength, for a SANS
    instrument. Currently makes the assumption that the incident beam monitor's
    UDET is 2, while that of the transmission monitor is 3 (as for LOQ). 
   
    Required Properties:
    <UL>
    <LI> SampleRunWorkspace  - The workspace containing the sample transmission run. </LI>
    <LI> DirectRunWorkspace  - The workspace containing the direct beam transmission run. </LI>
    <LI> OutputWorkspace     - The fitted transmission correction. </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> MinWavelength       - The minimum wavelength for the fit (Default: 2.2 Angstroms). </LI>
    <LI> MaxWavelength       - The maximum wavelength for the fit (Default: 10 Angstroms). </LI>
    <LI> OutputUnfittedData  - If true (false is the default), will output an additional workspace
                               called [OutputWorkspace]_unfitted containing the unfitted transmission
                               correction. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 22/01/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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
class DLLExport CalculateTransmission : public API::Algorithm
{
public:
  /// (Empty) Constructor
  CalculateTransmission() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~CalculateTransmission() {}
  /// Algorithm's name
  virtual const std::string name() const { return "CalculateTransmission"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  /// Pull out a single spectrum from a 2D workspace
  API::MatrixWorkspace_sptr extractSpectrum(DataObjects::Workspace2D_sptr WS, const int index);
  /// Call the Linear fitting algorithm as a child algorithm
  API::MatrixWorkspace_sptr fitToData(API::MatrixWorkspace_sptr WS);
  
  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CALCULATETRANSMISSION_H_*/
