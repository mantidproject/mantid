#ifndef MANTID_ALGORITHM_PHASEQUADMUON_H_
#define MANTID_ALGORITHM_PHASEQUADMUON_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**Algorithm for calculating Muon spectra.

@author Raquel Alvarez, ISIS, RAL
@date 1/12/2014

Copyright &copy; 2014-12 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
National Laboratory

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
class DLLExport PhaseQuadMuon : public API::Algorithm {
public:
  /// Default constructor
  PhaseQuadMuon()
      : API::Algorithm(), m_muLife(2.19703), m_bigNumber(1e10), m_tPulseOver(0),
        m_pulseTail(182), m_poissonLim(30), m_pulseTwo(0.325){};
  /// Destructor
  virtual ~PhaseQuadMuon(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "PhaseQuad"; }
  /// Summary of algorithm's purpose
  virtual const std::string summary() const {
    return "Calculate Muon squashograms from InputWorkspace and PhaseTable/PhaseList.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Muon"; }

private:
  class HistData {
  public:
    bool detOK;   // Detector is OK
    double n0;    // Detector n0
    double alpha; // Detector efficiency
    double phi;   // Detector phase
  };

  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
  /// Convert X units from micro-secs to nano-secs and shift to start at t=0
  void convertToNanoSecs(API::MatrixWorkspace_sptr inputWs);
  /// Convert X units from nano-secs to micro-secs and shift back
  void convertToMicroSecs(API::MatrixWorkspace_sptr inputWs);
  /// Load the Phase Table
  void loadPhaseTable(API::ITableWorkspace_sptr phaseTable,
                      API::ITableWorkspace_sptr deadTimeTable);
  /// Load the Phase List
  void loadPhaseList(const std::string &filename,
                     API::ITableWorkspace_sptr deadTimeTable);
  /// Apply dead time correction
  void deadTimeCorrection(API::MatrixWorkspace_sptr inputWs,
                          API::ITableWorkspace_sptr deadTimeTable,
                          API::MatrixWorkspace_sptr &tempWs);
  /// Rescale detector efficiency to maximum value
  void normaliseAlphas(std::vector<HistData> &m_histData);
  /// Remove exponential decay from input histograms
  void loseExponentialDecay(API::MatrixWorkspace_sptr tempWs);
  /// Create squashograms
  void squash(const API::MatrixWorkspace_sptr tempWs,
              API::MatrixWorkspace_sptr outputWs);
  /// Put back in exponential decay
  void regainExponential(API::MatrixWorkspace_sptr outputWs);
  /// Muon lifetime
  double m_muLife;
  /// Maximum counts expected
  double m_bigNumber;
  /// Pulse definitely finished by here (bin no)
  int m_tPulseOver;
  /// Number of bins to exclude after lag-time (ie pulse arrival time)
  double m_pulseTail;
  /// Poisson limit
  double m_poissonLim;
  /// Time (microsec) by which a well-def'd point in the first proton/pion pulse
  /// leads its counterpart in the second
  double m_pulseTwo;
  /// Number of input histograms
  int m_nHist;
  /// Number of datapoints per histogram
  int m_nData;
  /// Time resolution
  double m_res;
  /// Mean of time-shifts
  double m_meanLag;
  /// Good muons from here on (bin no). Unused now but can be needed in the
  /// future
  int m_tValid;
  /// Double-pulse flag
  bool m_isDouble;
  /// Minimum value of t
  double m_tMin;
  /// Vector of detector data
  std::vector<HistData> m_histData;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_PHASEQUAD_H_*/