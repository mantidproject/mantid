#ifndef MANTID_CURVEFITTING_CALCULATEMSVESUVIO_H_
#define MANTID_CURVEFITTING_CALCULATEMSVESUVIO_H_
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real.hpp>

namespace Mantid
{
  namespace CurveFitting
  {
    //-----------------------------------------------------------------------------
    // CurveFitting forward declarations
    //-----------------------------------------------------------------------------
    struct DetectorParams;
    struct ResolutionParams;

    /**
      Calculates the multiple scattering & total scattering contributions
      for a flat-plate or cylindrical sample.

      Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory &
      NScD Oak Ridge National Laboratory

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
    class DLLExport CalculateMSVesuvio : public API::Algorithm
    {
    private:
      // Struct to store cache instrument geometry
      struct InstrumentGeometry
      {
        InstrumentGeometry();

        boost::shared_ptr<const Geometry::ReferenceFrame> refframe;
        double srcR1;
        double srcR2;
        double sampleHeight;
        double sampleWidth;
      };
      // Produces random numbers with various probability distributions
      class RandomNumberGenerator
      {
        typedef boost::uniform_real<double> uniform_double;
        typedef boost::normal_distribution<double> gaussian_double;
      public:
        RandomNumberGenerator(const int seed);
        /// Returns a flat random number between 0.0 & 1.0
        double flat();
        /// Returns a random number distributed  by a normal distribution
        double gaussian(const double mean, const double sigma);

      private:
        boost::mt19937 m_generator;
      };
      // Stores counts for each scatter order
      // for a "run" of a given number of events
      struct Simulation
      {
        Simulation(const size_t order, const size_t ntimes);

        std::vector<std::vector<double>> counts;
        size_t maxorder;
        double weight;
        size_t nmscat;
      };
      // Accumulates and averages the results of each simulation
      struct SimulationAggregator
      {
        SimulationAggregator(const size_t nruns,
                             const size_t order,
                             const size_t ntimes);
        // Adds a result as part of the average
        void add(const Simulation & result);

        Simulation averaged;
        double prefactor;
      };

    public:
      CalculateMSVesuvio();
      ~CalculateMSVesuvio();

      virtual const std::string name() const;
      virtual int version() const;
      virtual const std::string category() const;
      virtual const std::string summary() const;

    private:
      void init();
      void exec();

      void cacheInputGeometry();
      void calculateMS(const size_t wsIndex, API::ISpectrum & totalsc,
                       API::ISpectrum & multsc) const;
      Simulation simulate(const size_t nevents, const size_t nscatters,
                            const DetectorParams & detpar,
                            const ResolutionParams &respar) const;
      double calculateTOF(const size_t nscatters,
                          const DetectorParams & detpar,
                          const ResolutionParams &respar,
                          Simulation & counts) const;

      // single-event helpers
      double initialTOF(const double mean, const double sigma) const;
      double moderatorPos(double & widthPos, double & heightPos) const;

      // Member Variables
      RandomNumberGenerator *m_randgen;
      InstrumentGeometry m_instgeom;

      API::MatrixWorkspace_sptr m_inputWS;
      API::Progress *m_progress;
    };

  } // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_CALCULATEMSVESUVIO_H_ */
