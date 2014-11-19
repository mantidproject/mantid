#ifndef MANTID_ALGORITHM_PHASEQUADMUON_H_
#define MANTID_ALGORITHM_PHASEQUADMUON_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /**Algorithm for calculating Muon spectra.

    @author Raquel Alvarez, ISIS, RAL
    @date 1/12/2011

    Copyright &copy; 2014-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport PhaseQuadMuon : public API::Algorithm
    {
    public:
      /// Default constructor
      PhaseQuadMuon() : API::Algorithm(), m_muLife(2.19703), m_bigNumber(1e10) {};
      /// Destructor
      virtual ~PhaseQuadMuon() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "PhaseQuad";}
      ///Summary of algorithms purpose
      virtual const std::string summary() const {return "Calculate Muon squashograms from inputWorkspace and PhaseTable.";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Muon";}

    private:

      class HistData { // TODO: do I need all the members?
      public:
        bool detOK;   // Detector is OK
        double alpha; // Detector efficiency
        double phi;   // Detector phase
        double lag;   // Detector time-shift
        double dead;  // Dead TODO remove if not used
        double deadm; // Dead TODO remove if not used
        double chisq; // Of silver fit TODO remove if not used
      };

      /// Initialise the properties
      void init();
      /// Run the algorithm
      void exec();
      /// Load the PhaseTable
      void loadPhaseTable(const std::string& filename);
      /// Rescale detector efficiency to maximum value
      void normaliseAlphas (std::vector<HistData>& m_histData);
      /// Remove exponential decay from input histograms
      void removeExponentialDecay (API::MatrixWorkspace_sptr inputWs, API::MatrixWorkspace_sptr outputWs);
      /// Remove exponential decay from input histograms
      void loseExponentialDecay (API::MatrixWorkspace_sptr inputWs, API::MatrixWorkspace_sptr outputWs);
      /// Create squashograms
      void squash(API::MatrixWorkspace_sptr tempWs, API::MatrixWorkspace_sptr outputWs);
      /// Put back in exponential decay
      void regainExponential(API::MatrixWorkspace_sptr outputWs);
      /// Number of input histograms
      size_t m_nHist;
      /// Number of datapoints per histogram
      size_t m_nData;
      /// TODO: remove if not necessary
      double m_res;
      /// Mean of time-shifts TODO: remove if not necessary
      double m_meanLag;
      /// Good muons from here on (bin no) TODO: remove if not necessary
      size_t m_tValid;
      /// Pulse definitely finished by here (bin no) TODO: remove if not necessary
      size_t m_tNoPulse;
      /// Double-pulse flag TODO: remove if not necessary
      bool m_isDouble;
      /// Muon decay curve TODO: remove if not necessary
      double m_tau;
      /// Freq (of silver run) TODO: remove if not necessary
      double m_w;
      /// Muon lifetime
      double m_muLife;
      /// Poisson limit TODO: remove if not necessary
      double m_poissonLim;
      /// Maximum counts expected
      double m_bigNumber;
      /// Vector of detector data
      std::vector<HistData> m_histData;
    };

  } // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_PHASEQUAD_H_*/