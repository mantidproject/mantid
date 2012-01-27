/*WIKI*



Converts a 2D workspace from units of spectrum number/energy transfer to  the intensity as a function of momentum transfer and energy. The rebinning is done as a weighted  sum of overlapping polygons.



 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SofQW2.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/Math/LaszloIntersection.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidGeometry/Math/Vertex2D.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(SofQW2)

    using namespace Mantid::Kernel;
    using namespace Mantid::API;
    using Geometry::IDetector_const_sptr;
    using Geometry::DetectorGroup;
    using Geometry::DetectorGroup_const_sptr;
    using Geometry::ConvexPolygon;
    using Geometry::Quadrilateral;
    using Geometry::Vertex2D;

    /// Default constructor
    SofQW2::SofQW2()
    : SofQW(), m_emode(0), m_efixedGiven(false), m_efixed(0.0), m_progress(),
      m_Qout(), m_thetaPts(), m_thetaWidth(0.0)
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void SofQW2::initDocs()
    {
      this->setWikiSummary("Calculate the intensity as a function of momentum transfer and energy");
      this->setOptionalMessage("Calculate the intensity as a function of momentum transfer and energy.");
    }

    /**
     * Execute the algorithm.
     */
    void SofQW2::exec()
    {
      MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
      // Do the full check for common binning
      if ( ! WorkspaceHelpers::commonBoundaries(inputWS) )
      {
        throw std::invalid_argument("The input workspace must have common binning across all spectra");
      }

      MatrixWorkspace_sptr outputWS = this->setUpOutputWorkspace(inputWS, m_Qout);
      // We also need to keep track of how many intersections went into a pixel so that we can
      // normalize correctly
      m_numIntersectionsWS = WorkspaceFactory::Instance().create(outputWS);

      const size_t nOutputHist(outputWS->getNumberHistograms());
      const size_t nenergyBins = inputWS->blocksize();

      //Progress reports & cancellation
      const size_t nreports(static_cast<size_t>(inputWS->getNumberHistograms()*inputWS->blocksize()));
      m_progress = boost::shared_ptr<API::Progress>(new API::Progress(this, 0.0, 1.0, nreports));

      // Compute input caches
      initCachedValues(inputWS);
      const size_t nTheta = m_thetaPts.size();
      const MantidVec & X = inputWS->readX(0);

      // Select the calculate Q method based on the mode
      // rather than doing this repeatedly in the loop
      typedef double (SofQW2::*QCalculation)(double, double, double, double) const;
      QCalculation qCalculator;
      if( m_emode == 1)
      {
        qCalculator = &SofQW2::calculateDirectQ;
      }
      else
      {
        qCalculator = &SofQW2::calculateIndirectQ;
      }

      PARALLEL_FOR3(inputWS, m_numIntersectionsWS, outputWS)
      for(int64_t i = 0; i < static_cast<int64_t>(nTheta); ++i) // signed for openmp
      {
        PARALLEL_START_INTERUPT_REGION

        const double theta = m_thetaPts[i];
        if( theta < 0.0 ) // One to skip
        {
          continue;
        }

        const double efixed = getEFixed(inputWS->getDetector(i));
        const double thetaLower = theta - 0.5*m_thetaWidth;
        const double thetaUpper = theta + 0.5*m_thetaWidth;

        for(size_t j = 0; j < nenergyBins; ++j)
        {
          m_progress->report("Computing polygon intersections");
          // For each input polygon test where it intersects with
          // the output grid and assign the appropriate weights of Y/E
          const double dE_j = X[j];
          const double dE_jp1 = X[j+1];

          const V2D ll(dE_j, (this->*qCalculator)(efixed, dE_j,thetaLower,0.0));
          const V2D lr(dE_jp1, (this->*qCalculator)(efixed, dE_jp1,thetaLower,0.0));
          const V2D ur(dE_jp1, (this->*qCalculator)(efixed, dE_jp1,thetaUpper,0.0));
          const V2D ul(dE_j, (this->*qCalculator)(efixed, dE_j,thetaUpper,0.0));
          Quadrilateral inputQ = Quadrilateral(ll, lr, ur, ul);

          rebinToOutput(inputQ, inputWS, i, j, outputWS);
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      /**
       * Normalise the output by the total accumulated fraction
       * and square root the errors
       */

      PARALLEL_FOR1(outputWS)
      for(int64_t i = 0; i < static_cast<int64_t>(nOutputHist); ++i)
      {
        PARALLEL_START_INTERUPT_REGION

        MantidVec & outputY = outputWS->dataY(i);
        MantidVec & outputE = outputWS->dataE(i);
        const MantidVec & noIntersects = m_numIntersectionsWS->readY(i);
        for(size_t j = 0; j < nenergyBins; ++j)
        {
          m_progress->report("Calculating errors and normalising");
          const double numIntersects = noIntersects[j];
          if(numIntersects > 0.0) outputY[j] /= numIntersects;
          outputE[j] = std::sqrt(outputE[j]);
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

    }

    /**
     * Rebin the input quadrilateral to the output grid
     * @param inputQ The input polygon
     * @param inputWS The input workspace containing the input intensity values
     * @param i The index in the Q direction that inputQ references
     * @param j The index in the dE direction that inputQ references
     * @param outputWS A pointer to the output workspace that accumulates the data
     */
    void SofQW2::rebinToOutput(const Geometry::Quadrilateral & inputQ, MatrixWorkspace_const_sptr inputWS, 
                               const size_t i, const size_t j, MatrixWorkspace_sptr outputWS)
    {
      size_t qstart(0.0), qend(0.0), en_start(0.0), en_end(0.0);
      if( !getIntersectionRegion(outputWS, inputQ, qstart, qend, en_start, en_end)) return;
      const MantidVec & X = inputWS->readX(0);
      for( size_t qi = qstart; qi < qend; ++qi )
      {
        MantidVec & outputY = outputWS->dataY(qi);
        MantidVec & outputE = outputWS->dataE(qi);
        for( size_t ei = en_start; ei < en_end; ++ei )
        {
          const V2D ll(X[ei], m_Qout[qi]);
          const V2D lr(X[ei+1], m_Qout[qi]);
          const V2D ur(X[ei+1], m_Qout[qi+1]);
          const V2D ul(X[ei], m_Qout[qi+1]);
          const Quadrilateral outputQ(ll, lr, ur, ul);
          try
          {
            ConvexPolygon overlap = intersectionByLaszlo(outputQ, inputQ);
            const double weight = overlap.area()/inputQ.area();
            outputY[ei] += inputWS->readY(i)[j] * weight;
            m_numIntersectionsWS->dataY(qi)[ei] += weight;
            outputE[ei] += std::pow(inputWS->readE(i)[j] * weight, 2);
          }
          catch(Geometry::NoIntersectionException &)
          {}
        }
      }
    }

    /**
     * Find the possible region of intersection on the output workspace for the given polygon
     * @param outputWS A pointer to the output workspace
     * @param inputQ The input polygon
     * @param qstart An output giving the starting index in the Q direction
     * @param qend An output giving the end index in the Q direction
     * @param en_start An output giving the start index in the dE direction
     * @param en_end An output giving the end index in the dE direction
     * @return True if an intersecition is possible
     */
    bool SofQW2::getIntersectionRegion(API::MatrixWorkspace_const_sptr outputWS, const Geometry::Quadrilateral & inputQ,
        size_t &qstart, size_t &qend, size_t &en_start, size_t &en_end) const
    {
      const MantidVec & energyAxis = outputWS->readX(0);
      const double xn_lo(inputQ.smallestX()), xn_hi(inputQ.largestX());
      const double yn_lo(inputQ.smallestY()), yn_hi(inputQ.largestY());

      if( xn_lo < energyAxis.front() || xn_hi > energyAxis.back() ||
          yn_lo < m_Qout.front() || yn_hi > m_Qout.back() ) return true;

      MantidVec::const_iterator start_it = std::upper_bound(energyAxis.begin(), energyAxis.end(), xn_lo);
      MantidVec::const_iterator end_it = std::upper_bound(energyAxis.begin(), energyAxis.end(), xn_hi);
      en_start = 0;
      en_end = energyAxis.size() - 1;
      if( start_it != energyAxis.begin() )
      {
        en_start = (start_it - energyAxis.begin() - 1);
      }
      if( end_it != energyAxis.end() )
      {
        en_end = end_it - energyAxis.begin();
      }
      // Q region
      start_it = std::upper_bound(m_Qout.begin(), m_Qout.end(), yn_lo);
      end_it = std::upper_bound(m_Qout.begin(), m_Qout.end(), yn_hi);
      qstart = 0;
      qend = m_Qout.size() - 1;
      if( start_it != m_Qout.begin() )
      {
        qstart = (start_it - m_Qout.begin() - 1);
      }
      if( end_it != m_Qout.end() )
      {
        qend = end_it - m_Qout.begin();
      }
      return true;
    }


    /**
     * Return the efixed for this detector
     * @param det A pointer to a detector object
     * @return The value of efixed
     */
    double SofQW2::getEFixed(Geometry::IDetector_const_sptr det) const
    {
      double efixed(0.0);
      if( m_emode == 1 ) //Direct
      {
        efixed = m_efixed;
      }
      else // Indirect
      {
        if( m_efixedGiven ) efixed = m_efixed; // user provided a value
        else
        {
          std::vector<double> param = det->getNumberParameter("EFixed");
          if( param.empty() ) throw std::runtime_error("Cannot find EFixed parameter for component \"" + det->getName()
              + "\". This is required in indirect mode. Please check the IDF contains these values.");
          efixed = param[0];
        }
      }
      return efixed;
    }

    /**
     * Calculate the Q value for a direct instrument
     * @param efixed An efixed value
     * @param deltaE The energy change
     * @param twoTheta The value of the scattering angle
     * @param psi The value of the azimuth
     * @return The value of Q
     */
    double SofQW2::calculateDirectQ(const double efixed, const double deltaE, const double twoTheta,
        const double psi) const
    {
      const double ki = std::sqrt(efixed*m_EtoK);
      const double kf = std::sqrt((efixed - deltaE)*m_EtoK);
      const double Qx = ki - kf*std::cos(twoTheta);
      const double Qy = -kf*std::sin(twoTheta)*std::cos(psi);
      const double Qz = -kf*std::sin(twoTheta)*std::sin(psi);
      return std::sqrt(Qx*Qx + Qy*Qy + Qz*Qz);
    }

    /**
     * Calculate the Q value for a direct instrument
     * @param efixed An efixed value
     * @param deltaE The energy change
     * @param twoTheta The value of the scattering angle
     * @param psi The value of the azimuth
     * @return The value of Q
     */
    double SofQW2::calculateIndirectQ(const double efixed, const double deltaE, const double twoTheta,
        const double psi) const
    {
      UNUSED_ARG(psi);
      const double ki = std::sqrt((efixed + deltaE)*m_EtoK);
      const double kf = std::sqrt(efixed*m_EtoK);
      const double Qx = ki - kf*std::cos(twoTheta);
      const double Qy = -kf*std::sin(twoTheta);
      return std::sqrt(Qx*Qx + Qy*Qy);
    }

    /**
     * Init variables caches
     * @param workspace :: Workspace pointer
     */
    void SofQW2::initCachedValues(API::MatrixWorkspace_const_sptr workspace)
    {
      // Retrieve the emode & efixed properties
      const std::string emode = getProperty("EMode");
      // Convert back to an integer representation
      m_emode = 0;
      if (emode == "Direct") m_emode=1;
      else if (emode == "Indirect") m_emode = 2;
      m_efixed = getProperty("EFixed");

      // Check whether they should have supplied an EFixed value
      if( m_emode == 1 ) // Direct
      {
        // If GetEi was run then it will have been stored in the workspace, if not the user will need to enter one
        if( m_efixed == 0.0 )
        {
          if ( workspace->run().hasProperty("Ei") )
          {
            Kernel::Property *p = workspace->run().getProperty("Ei");
            Kernel::PropertyWithValue<double> *eiProp = dynamic_cast<Kernel::PropertyWithValue<double>*>(p);
            if( !eiProp ) throw std::runtime_error("Input workspace contains Ei but its property type is not a double.");
            m_efixed = (*eiProp)();
          }
          else
          {
            throw std::invalid_argument("Input workspace does not contain an EFixed value. Please provide one or run GetEi.");
          }
        }
        else
        {
          m_efixedGiven = true;
        }
      }
      else
      {
        if( m_efixed != 0.0 )
        {
          m_efixedGiven = true;
        }
      }


      // Conversion constant for E->k. k(A^-1) = sqrt(energyToK*E(meV))
      m_EtoK = 8.0*M_PI*M_PI*PhysicalConstants::NeutronMass*PhysicalConstants::meV*1e-20 /
          (PhysicalConstants::h*PhysicalConstants::h);

      // Index theta cache
      initThetaCache(workspace);
    }

    /**
     * A map detector ID and Q ranges
     * This method looks unnecessary as it could be calculated on the fly but
     * the parallelization means that lazy instantation slows it down due to the
     * necessary CRITICAL sections required to update the cache. The Q range
     * values are required very frequently so the total time is more than
     * offset by this precaching step
     */
    void SofQW2::initThetaCache(API::MatrixWorkspace_const_sptr workspace)
    {
      const size_t nhist = workspace->getNumberHistograms();
      m_thetaPts = std::vector<double>(nhist);
      size_t ndets(0);
      double minTheta(DBL_MAX), maxTheta(-DBL_MAX);

      //PARALLEL_FOR1(workspace)
      for(int64_t i = 0 ; i < (int64_t)nhist; ++i) //signed for OpenMP
      {
        PARALLEL_START_INTERUPT_REGION

        m_progress->report("Calculating detector angles");
        IDetector_const_sptr det;
        try
        {
          det = workspace->getDetector(i);
          // Check to see if there is an EFixed, if not skip it
          try
          {
            getEFixed(det);
          }
          catch(std::runtime_error&)
          {
            det.reset();
          }
        }
        catch(Kernel::Exception::NotFoundError&)
        {
          // Catch if no detector. Next line tests whether this happened - test placed
          // outside here because Mac Intel compiler doesn't like 'continue' in a catch
          // in an openmp block.
        }
        // If no detector found, skip onto the next spectrum
        if( !det || det->isMonitor() )
        {
          m_thetaPts[i] = -1.0; // Indicates a detector to skip
        }
        else
        {
          PARALLEL_ATOMIC
          ++ndets;
          const double theta = workspace->detectorTwoTheta(det);
          m_thetaPts[i] = theta;
          if( theta < minTheta )
          {
            PARALLEL_CRITICAL(minTheta)
            minTheta = theta;
          }
          else if( theta > maxTheta )
          {
            PARALLEL_CRITICAL(maxTheta)
            maxTheta = theta;
          }
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      m_thetaWidth = (maxTheta - minTheta)/static_cast<double>(ndets);
      g_log.information() << "Calculated detector width in theta=" << (m_thetaWidth*180.0/M_PI) << " degrees.\n";
    }


  } // namespace Mantid
} // namespace Algorithms

