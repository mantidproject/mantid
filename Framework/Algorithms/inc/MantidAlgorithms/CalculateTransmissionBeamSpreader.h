#ifndef MANTID_ALGORITHMS_CALCTRANSBEAMSPREADER_H_
#define MANTID_ALGORITHMS_CALCTRANSBEAMSPREADER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {
/** Calculates the transmission correction, as a function of wavelength, for a
   SANS
    instrument. The beam-spreader method is used.

    Required Properties:
    <UL>
    <LI> SampleSpreaderRunWorkspace  - The workspace containing the sample
   beam-spreader run. </LI>
    <LI> DirectSpreaderRunWorkspace  - The workspace containing the direct
   beam-spreader run. </LI>
    <LI> SampleScatterRunWorkspace   - The workspace containing the sample
   scattering run. </LI>
    <LI> DirectScatterRunWorkspace   - The workspace containing the direct beam
   scattering run. </LI>
    <LI> SpreaderTransmissionValue   - Transmission coefficient of the beam
   spreader. </LI>
    <LI> SpreaderTransmissionError   - Uncertainty on the transmission
   coefficient of the beam spreader. </LI>
    <LI> IncidentBeamMonitor         - The UDET of the incident beam monitor.
   </LI>
    <LI> OutputWorkspace             - The fitted transmission correction. </LI>
    </UL>

    Optional Properties:
    <UL>
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

    @author Mathieu Doucet, ORNL
    @date 28/07/2010

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
class DLLExport CalculateTransmissionBeamSpreader : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override {
    return "CalculateTransmissionBeamSpreader";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates the sample transmission using the beam spreader (aka "
           "glass carbon) method.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateTransmission", "ApplyTransmissionCorrection"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "SANS;CorrectionFunctions\\TransmissionCorrections";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Pull out a single spectrum from a 2D workspace
  API::MatrixWorkspace_sptr extractSpectrum(API::MatrixWorkspace_sptr WS,
                                            const size_t index);
  /// Call the Linear fitting algorithm as a child algorithm
  API::MatrixWorkspace_sptr fitToData(API::MatrixWorkspace_sptr WS);
  /// Sum the total detector, excluding masked pixels and monitors
  API::MatrixWorkspace_sptr sumSpectra(API::MatrixWorkspace_sptr WS);

  bool logFit =
      false; ///< If true, will take log of transmission curve before fitting
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CALCULATETRANSMISSION_H_*/
