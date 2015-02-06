#ifndef MANTID_ALGORITHMS_CALCULATETRANSMISSION_H_
#define MANTID_ALGORITHMS_CALCULATETRANSMISSION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {
/** Calculates the transmission correction, as a function of wavelength, for a
   SANS
    instrument. Currently makes the assumption that the incident beam monitor's
    UDET is 2, while that of the transmission monitor is 3 (as for LOQ).

    Required Properties:
    <UL>
    <LI> SampleRunWorkspace  - The workspace containing the sample transmission
   run. </LI>
    <LI> DirectRunWorkspace  - The workspace containing the direct beam
   transmission run. </LI>
    <LI> OutputWorkspace     - The fitted transmission correction. </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> IncidentBeamMonitor - The UDET of the incident beam monitor (Default:
   2, as for LOQ). </LI>
    <LI> TransmissionMonitor - The UDET of the transmission monitor (Default: 3,
   as for LOQ). </LI>
    <LI> MinWavelength       - The minimum wavelength for the fit (Default: 2.2
   Angstroms). </LI>
    <LI> MaxWavelength       - The maximum wavelength for the fit (Default: 10
   Angstroms). </LI>
    <LI> FitMethod           - Whether to fit to the log of the transmission
   curve (the default) or directly (i.e. linearly). </LI>
    <LI> OutputUnfittedData  - If true (false is the default), will output an
   additional workspace
                               called [OutputWorkspace]_unfitted containing the
   unfitted transmission
                               correction. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 22/01/2009

    Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
class DLLExport CalculateTransmission : public API::Algorithm {
public:
  /// Constructor
  CalculateTransmission();
  /// Virtual destructor
  virtual ~CalculateTransmission();
  /// Algorithm's name
  virtual const std::string name() const { return "CalculateTransmission"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculates the transmission correction, as a function of "
           "wavelength, for a SANS instrument.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "SANS;CorrectionFunctions\\TransmissionCorrections";
  }

private:
  /// stores an estimate of the progress so far as a proportion (starts at zero
  /// goes to 1.0)
  mutable double m_done;

  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  /// Pull out a single spectrum from a 2D workspace
  API::MatrixWorkspace_sptr extractSpectrum(API::MatrixWorkspace_sptr WS,
                                            const int64_t index);
  /// Returns a workspace with the evaulation of the fit to the calculated
  /// transmission fraction
  API::MatrixWorkspace_sptr fit(API::MatrixWorkspace_sptr raw,
                                std::vector<double> rebinParams,
                                const std::string fitMethod);
  /// Call the Linear fitting algorithm as a child algorithm
  API::MatrixWorkspace_sptr fitData(API::MatrixWorkspace_sptr WS, double &grad,
                                    double &offset);
  /// Call the Polynomial fitting algorithm as a child algorithm
  API::MatrixWorkspace_sptr fitPolynomial(API::MatrixWorkspace_sptr WS,
                                          int order,
                                          std::vector<double> &coeficients);
  /// Calls the rebin algorithm
  API::MatrixWorkspace_sptr rebin(std::vector<double> &binParams,
                                  API::MatrixWorkspace_sptr output);
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CALCULATETRANSMISSION_H_*/
