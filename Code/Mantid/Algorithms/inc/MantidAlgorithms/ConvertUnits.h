#ifndef MANTID_ALGORITHMS_CONVERTUNITS_H_
#define MANTID_ALGORITHMS_CONVERTUNITS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** Converts the units in which a workspace is represented.
    Only implemented for histogram data, so far.
    Note that if you are converting to or from units which are not meaningful for monitor detectors,
    then you should not expect the resulting spectrum to hold meaningful values.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as the input one. </LI>
    <LI> Target          - The units to which the workspace should be converted. </LI>
    </UL>

    Optional properties required for certain units (DeltaE & DeltaE_inWavenumber):
    <UL>
    <LI> Emode  - The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry) </LI>
    <LI> Efixed - Value of fixed energy: EI (emode=1) or EF (emode=2) (meV) </LI>
    </UL>

    Optional, deprecated property (see http://www.mantidproject.org/ConvertUnits for details):
    <UL>
    <LI> AlignBins - If true (default is false), rebins if necessary to ensure that all spectra in
                     the output workspace have identical bins (with linear binning) </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 06/03/2008

    Copyright &copy; 2008-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport ConvertUnits : public API::Algorithm
{
public:
  /// Default constructor
  ConvertUnits();
  /// Virtual destructor
  virtual ~ConvertUnits();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "ConvertUnits"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Units";}

private:
  // Overridden Algorithm methods
  void init();
  void exec();
  void execEvent();

  /// Convert the workspace units according to a simple output = a * (input^b) relationship
  void convertQuickly(const int& numberOfSpectra, API::MatrixWorkspace_sptr outputWS, const double& factor, const double& power);
  /// Convert the workspace units using TOF as an intermediate step in the conversion
  void convertViaTOF(const int& numberOfSpectra, Kernel::Unit_const_sptr fromUnit, API::MatrixWorkspace_sptr outputWS);

  /// Convert the EventWorkspace units using TOF as an intermediate step in the conversion
  void convertViaEventsTOF(const int& numberOfSpectra, Kernel::Unit_const_sptr fromUnit, DataObjects::EventWorkspace_sptr outputWS);

  // Calls Rebin as a sub-algorithm to align the bins of the output workspace
  API::MatrixWorkspace_sptr alignBins(const API::MatrixWorkspace_sptr workspace);
  const std::vector<double> calculateRebinParams(const API::MatrixWorkspace_const_sptr workspace) const;

  /// Reverses the workspace if X values are in descending order
  void reverse(API::MatrixWorkspace_sptr workspace);

  /// For conversions to energy transfer, removes bins corresponding to inaccessible values
  API::MatrixWorkspace_sptr removeUnphysicalBins(const API::MatrixWorkspace_const_sptr workspace);

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTUNITS_H_*/
