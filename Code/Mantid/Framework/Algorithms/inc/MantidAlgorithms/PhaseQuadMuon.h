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
      virtual const std::string summary() const {return "Calculate Muon spectra from PhaseTable.";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Muon";}

    private:

      class HistData { // TODO: do I need all the members?
      public:
        double n0;    // 
        double alpha; // Efective asymmetry
        double phi;   // Phase
        double lag;   // Time-shift
        double dead;  // 
        double deadm; //
        double chisq; // Of silver fit
      };

      /// Initialise the properties
      void init();
      /// Run the algorithm
      void exec();
      /// Load the PhaseTable
      void loadPhaseTable(const std::string& filename);
      /// TODO
      void normaliseAlphas (std::vector<HistData>& m_histData);
      /// Remove exponential decay from input histograms
      void removeExponentialDecay (API::MatrixWorkspace_sptr inputWs, API::MatrixWorkspace_sptr outputWs);
      /// Remove exponential decay from input histograms
      void loseExponentialDecay (API::MatrixWorkspace_sptr inputWs, API::MatrixWorkspace_sptr outputWs);
      /// Create squashograms
      void squash(API::MatrixWorkspace_sptr tempWs, API::MatrixWorkspace_sptr outputWs);
      /// Number of input histograms
      size_t m_nHist;
      /// Number of datapoints per histogram
      size_t m_nData;
      /// TODO
      double m_res;
      /// Mean of time-shifts
      double m_meanLag;
      /// Good muons from here on (bin no)
      size_t m_tValid;
      /// Pulse definitely finished by here (bin no)
      size_t m_tNoPulse;
      /// Double-pulse flag
      bool m_isDouble;
      /// Muon decay curve
      double m_tau;
      /// Freq (of silver run)
      double m_w;
      /// Muon lifetime
      double m_muLife;
      /// Poisson limit
      double m_poissonLim;
      /// TODO
      double m_bigNumber;
      /// Vector of histograms data
      std::vector<HistData> m_histData;
    };

  } // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_PHASEQUAD_H_*/