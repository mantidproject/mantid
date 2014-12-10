#ifndef MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLE_H_
#define MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLE_H_

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

    Copyright &copy; 2008-2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport ConvertUnitsUsingDetectorTable : public API::Algorithm
{
public:
  /// Default constructor
  ConvertUnitsUsingDetectorTable();
  /// Virtual destructor
  virtual ~ConvertUnitsUsingDetectorTable();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "ConvertUnitsUsingDetectorTable"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Performs a unit change on the X values of a workspace";}

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Utility\\Development";}

private:
  const std::string workspaceMethodName() const { return "convertUnitsUsingDetectorTable"; }
  const std::string workspaceMethodInputProperty() const { return "InputWorkspace"; }

  
  // Overridden Algorithm methods
  void init();
  void exec();

  void setupMemberVariables(const API::MatrixWorkspace_const_sptr inputWS);
  API::MatrixWorkspace_sptr setupOutputWorkspace(const API::MatrixWorkspace_const_sptr inputWS);
  void fillOutputHist(const API::MatrixWorkspace_const_sptr inputWS, const API::MatrixWorkspace_sptr outputWS);

  /// Convert the workspace units according to a simple output = a * (input^b) relationship
  void convertQuickly(API::MatrixWorkspace_sptr outputWS, const double& factor, const double& power);
  /// Convert the workspace units using TOF as an intermediate step in the conversion
  void convertViaTOF(Kernel::Unit_const_sptr fromUnit, API::MatrixWorkspace_sptr outputWS);

  // Calls Rebin as a Child Algorithm to align the bins of the output workspace
  API::MatrixWorkspace_sptr alignBins(const API::MatrixWorkspace_sptr workspace);
  const std::vector<double> calculateRebinParams(const API::MatrixWorkspace_const_sptr workspace) const;

  /// Reverses the workspace if X values are in descending order
  void reverse(API::MatrixWorkspace_sptr workspace);

  /// For conversions to energy transfer, removes bins corresponding to inaccessible values
  API::MatrixWorkspace_sptr removeUnphysicalBins(const API::MatrixWorkspace_const_sptr workspace);

  void putBackBinWidth(const API::MatrixWorkspace_sptr outputWS);

  std::size_t m_numberOfSpectra;     ///< The number of spectra in the input workspace
  bool m_distribution;       ///< Whether input is a distribution. Only applies to histogram workspaces.
  bool m_inputEvents;        ///< Flag indicating whether input workspace is an EventWorkspace
  Kernel::Unit_const_sptr m_inputUnit;  ///< The unit of the input workspace
  Kernel::Unit_sptr m_outputUnit;       ///< The unit we're going to
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLE_H_*/
