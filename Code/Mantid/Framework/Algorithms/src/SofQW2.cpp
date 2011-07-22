//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SofQW2.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/Math/LaszloIntersection.h"
#include "MantidGeometry/Math/Vertex2D.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"

#include "MantidKernel/Timer.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    //DECLARE_ALGORITHM(SofQW2)

    using namespace Mantid::Kernel;
    using namespace Mantid::API;
    using Geometry::IDetector_sptr;
    using Geometry::DetectorGroup;
    using Geometry::DetectorGroup_sptr;
    using Geometry::ConvexPolygon;
    using Geometry::Vertex2D;

    /// Default constructor
    SofQW2::SofQW2() 
      : SofQW(), m_emode(0), m_efixed(0.0), m_beamDir(), 
        m_samplePos(), m_thetaIndex(), m_progress(), m_qcache()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void SofQW2::initDocs()
    {
      this->setWikiSummary("Calculate the intensity as a function of momentum transfer and energy");
      this->setOptionalMessage("Calculate the intensity as a function of momentum transfer and energy.");
      this->setWikiDescription("Converts a 2D workspace from units of spectrum number/energy transfer to "
        " the intensity as a function of momentum transfer and energy. The rebinning is done as a weighted "
        " sum of overlapping polygons.");
    }

    /** 
    * Execute the algorithm.
    */
    void SofQW2::exec()
    {
      MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
      // Output workspace
      std::vector<double> newYBins;
      MatrixWorkspace_sptr outputWS = this->setUpOutputWorkspace(inputWS, newYBins);
      const int64_t nOutHist(static_cast<int64_t>(outputWS->getNumberHistograms()));
      const size_t nOutBins(outputWS->blocksize());
      const size_t nreports(static_cast<size_t>(nOutHist*nOutBins+1));
      m_progress = boost::shared_ptr<API::Progress>(new API::Progress(this, 0.0, 1.0, nreports));
      initCachedValues(inputWS);

      PARALLEL_FOR2(inputWS, outputWS)
      for(int64_t i = 0; i < nOutHist; ++i)
      {
        PARALLEL_START_INTERUPT_REGION

        const double y_i = newYBins[i];
        const double y_ip1 = newYBins[i+1];
        const MantidVec& X = outputWS->readX(i);
        // References to the Y and E data
        MantidVec & outY = outputWS->dataY(i);
        MantidVec & outE = outputWS->dataE(i);
        for( size_t j = 0; j < nOutBins; ++j )
        {
          m_progress->report("Computing polygon overlap");
          const double xo_lo(X[j]), xo_hi(X[j+1]);
          ConvexPolygon newPoly(xo_lo, xo_hi, y_i, y_ip1);
          std::pair<double,double> contrib = calculateYE(inputWS, newPoly);
          outY[j] = contrib.first;
          outE[j] = contrib.second;
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
    }

    /**
     * Calculate the Y and E values for the given possible overlap
     * @param inputWS :: A pointer to the inputWS
     * @param newPoly :: A reference to a polygon to test for overlap
     * @returns A pair of Y and E values
     */
    std::pair<double,double> 
    SofQW2::calculateYE(API::MatrixWorkspace_const_sptr inputWS,
                        const ConvexPolygon & newPoly) const
    {
      // Build a list intersection locations in terms of workspace indices
      // along with corresponding weights from that location
      std::vector<BinWithWeight> overlaps = findIntersections(inputWS, newPoly);
      std::pair<double,double> binValues(0,0);
      if( inputWS->isDistribution() )
      {
        const double newWidth = newPoly[3].X() - newPoly[0].X(); // For distribution
        binValues = calculateDistYE(inputWS, overlaps, newWidth);
      }
      else
      {
        binValues = calculateYE(inputWS, overlaps);
      }
      return binValues;
    }

        /**
     * Calculate the Y and E values from the given overlaps
     * @param inputWS :: A pointer to the inputWS
     * @param overlaps :: A list of overlap locations and weights
     * @returns A pair of Y and E values
     */
    std::pair<double,double> SofQW2::calculateYE(API::MatrixWorkspace_const_sptr inputWS,
                                                  const std::vector<BinWithWeight> & overlaps) const
    {
      double totalY(0.0), totalE(0.0);
      std::vector<BinWithWeight>::const_iterator iend = overlaps.end();
      for(std::vector<BinWithWeight>::const_iterator itr = overlaps.begin();
          itr != iend; ++itr)
      {
        const size_t yIndex = itr->yIndex;
        const size_t xIndex = itr->xIndex;
        const MantidVec & inputY = inputWS->readY(yIndex);
        const MantidVec & inputE = inputWS->readE(yIndex);
        totalY += inputY[xIndex] * itr->weight;
        totalE += std::pow(inputE[xIndex] * itr->weight, 2);
      }
      return std::make_pair(totalY,std::sqrt(totalE));
    }

    /**
     * Calculate the Y and E values from the given intersections for an input distribution
     * @param inputWS :: A pointer to the inputWS
     * @param overlaps :: A list of overlap locations and weights
     * @param newBinWidth :: The width of the new bin
     * @returns A pair of Y and E values
     */
    std::pair<double,double> SofQW2::calculateDistYE(API::MatrixWorkspace_const_sptr inputWS,
                                                      const std::vector<BinWithWeight> & overlaps,
                                                      const double newBinWidth) const
    {
      const MantidVec & oldXBins = inputWS->readX(0);
      double totalY(0.0), totalE(0.0);
      std::vector<BinWithWeight>::const_iterator iend = overlaps.end();
      for(std::vector<BinWithWeight>::const_iterator itr = overlaps.begin();
          itr != iend; ++itr)
      {
        const size_t yIndex = itr->yIndex;
        const size_t xIndex = itr->xIndex;
        const MantidVec & inputY = inputWS->readY(yIndex);
        const MantidVec & inputE = inputWS->readE(yIndex);
        double oldWidth = oldXBins[xIndex+1] - oldXBins[xIndex];
        totalY += inputY[xIndex] * oldWidth * itr->weight;
        totalE += std::pow(inputE[xIndex]*oldWidth*itr->weight,2);
      }
      return std::make_pair(totalY/newBinWidth,std::sqrt(totalE)/newBinWidth);
    }

    /**
     * Find the overlap of the inputWS with the given polygon
     * @param oldAxis1 :: Axis 1 bin boundaries from the input grid
     * @param oldAxis2 :: Axis 2 bin boundaries from the input grid
     * @param newPoly :: The new polygon to test
     * @returns A list of intersection locations with weights of the overlap
     */
    std::vector<SofQW2::BinWithWeight> 
    SofQW2::findIntersections(API::MatrixWorkspace_const_sptr inputWS,
                              const Geometry::ConvexPolygon & newPoly) const
    {
      std::vector<BinWithWeight> overlaps;
      overlaps.reserve(5); // Have a guess at a posible limit

      const size_t nypoints(inputWS->getNumberHistograms());
      for(size_t i = 0; i < nypoints; ++i)
      {
        IDetector_sptr det_i;
        try
        {
          det_i = inputWS->getDetector(i);
          if( det_i->isMonitor() ) continue;
        }
        catch(Kernel::Exception::NotFoundError&)
        {
          continue;
        }
        DetectorGroup_sptr detGroup = boost::dynamic_pointer_cast<DetectorGroup>(det_i);
        if( detGroup ) 
        {
          findIntersections(inputWS, i, detGroup->getDetectors(), newPoly, overlaps);
        }
        else
        {
          findIntersections(inputWS, i, std::vector<IDetector_sptr>(1,det_i), newPoly, overlaps);
        }
      }
      return overlaps;
    }

    /// Find the overlap of the inputWS with the given polygon
    void SofQW2::findIntersections(API::MatrixWorkspace_const_sptr inputWS,
                                   const size_t yIndex, 
                                   const std::vector<Geometry::IDetector_sptr> & dets,
                                   const Geometry::ConvexPolygon & newPoly,
                                   std::vector<BinWithWeight> & overlaps) const
    {
      const MantidVec & oldAxis1 = inputWS->readX(0);
      const size_t ndets(dets.size());
      const double ndets_dbl = static_cast<double>(ndets);

      // Find the region to search as we don't need to search everywhere
      const double xn_lo(newPoly[0].X()), xn_hi(newPoly[2].X());
      const double yn_lo(newPoly[0].Y()), yn_hi(newPoly[1].Y());
      MantidVec::const_iterator start_it = std::upper_bound(oldAxis1.begin(), oldAxis1.end(), xn_lo);
      MantidVec::const_iterator end_it = std::upper_bound(oldAxis1.begin(), oldAxis1.end(), xn_hi);
      const size_t start_index = (start_it - oldAxis1.begin() - 1);
      const size_t end_index = start_index + (end_it - start_it + 1);

      for( size_t i = 0; i < ndets; ++i )
      {
        IDetector_sptr det = dets[i];
        for(size_t j = start_index; j < end_index; ++j)
        {
          const double xo_lo(oldAxis1[j]), xo_hi(oldAxis1[j+1]);
          std::pair<double,double> qold = getQRange(det, j);
          if( qold.second < yn_lo || qold.first > yn_hi ) continue;
          //ConvexPolygon oldPoly(xo_lo, xo_hi, qold.first, qold.second);
          Vertex2D *head = new Vertex2D(xo_lo, qold.first);
          head->insert(new Vertex2D(xo_hi, qold.first));
          head->insert(new Vertex2D(xo_hi, qold.second));
          head->insert(new Vertex2D(xo_lo, qold.first));
          ConvexPolygon oldPoly(head);
          try
          {
            ConvexPolygon overlap = intersectionByLaszlo(newPoly, oldPoly);
            overlaps.push_back(BinWithWeight(yIndex,j,overlap.area()/oldPoly.area()/ndets_dbl));
          }
          catch(Geometry::NoIntersectionException &)
          {}            
        }
      }
    }

    /**
     * Determine the Q Range from a given detector on the workspace
     * @param det :: The pointer to the detector instance
     * @param deltaE :: The value of deltaE
     */
    std::pair<double,double> SofQW2::calculateQRange(Geometry::IDetector_sptr det, 
                                                     const double dEMin, const double dEMax) const
    {
      double r(0.0), theta_pt(0.0), phi(0.0);
      det->getPos().getSpherical(r,theta_pt,phi);
      typedef std::vector<double>::const_iterator dbl_iterator;
      // Find the upper bin boundary
      dbl_iterator upp = std::upper_bound(m_thetaIndex.begin(), m_thetaIndex.end(), theta_pt);
      const size_t ip1 = static_cast<size_t>(std::distance(m_thetaIndex.begin(), upp));
      const double theta_max = m_thetaIndex[ip1];
      const double theta_min = m_thetaIndex[ip1-1];      
      V3D scatter_min;
      scatter_min.spherical(r, theta_min, phi);
      scatter_min = (scatter_min - m_samplePos);
      scatter_min.normalize();
      V3D scatter_max;
      scatter_max.spherical(r, theta_max, phi);
      scatter_max = (scatter_max - m_samplePos);
      scatter_max.normalize();
      // Compute ki and kf wave vectors and therefore q = ki - kf
      double ei_min(0.0), ei_max(0.0), ef_min(0.0), ef_max(0.0);
      if( m_emode == 2 )
      {
        std::vector<double> param = det->getNumberParameter("EFixed");
        double efixed = m_efixed;
        if( param.size() != 0 ) efixed = param[0];
        ei_min = efixed + dEMin;
        ei_max = efixed + dEMax;
        ef_min = ef_max = efixed;
      }
      else
      {
        ei_min = ei_max = m_efixed;
        ef_min = m_efixed - dEMin;
        ef_max = m_efixed - dEMax;
      }
      const V3D ki_min = m_beamDir*sqrt(m_EtoK*ei_min);
      const V3D ki_max = m_beamDir*sqrt(m_EtoK*ei_max);
      const V3D kf_min = scatter_min*(sqrt(m_EtoK*ef_min));
      const V3D kf_max = scatter_max*(sqrt(m_EtoK*ef_max));
      // Using the inelastic convention
      return std::pair<double,double>((ki_min - kf_min).norm(), (ki_max - kf_max).norm());
    }

    /**
     * Get the Q Range from a given detector on the workspace
     * @param det :: The pointer to the detector instance
     * @param xIndex :: The index in the X vector (for cache lookup)
     */
    std::pair<double,double> SofQW2::getQRange(Geometry::IDetector_sptr det, 
                                               const size_t xIndex) const
    {
      std::map<detid_t, std::vector<std::pair<double,double> > >::const_iterator it = 
        m_qcache.find(det->getID());
      return it->second[xIndex];
    }   

    /**
     * Init variables caches
     * @param :: Workspace pointer
     */
    void SofQW2::initCachedValues(API::MatrixWorkspace_const_sptr workspace)
    {
      m_progress->report("Initializing caches");

      // Retrieve the emode & efixed properties
      const std::string emode = getProperty("EMode");
      // Convert back to an integer representation
      m_emode = 0;
      if (emode == "Direct") m_emode=1;
      else if (emode == "Indirect") m_emode = 2;
      m_efixed = getProperty("EFixed");

      // Conversion constant for E->k. k(A^-1) = sqrt(energyToK*E(meV))
      m_EtoK = 8.0*M_PI*M_PI*PhysicalConstants::NeutronMass*PhysicalConstants::meV*1e-20 / 
        (PhysicalConstants::h*PhysicalConstants::h);

      // Get a pointer to the instrument contained in the workspace
      Geometry::IInstrument_const_sptr instrument = workspace->getInstrument();
      // Get the distance between the source and the sample (assume in metres)
      Geometry::IObjComponent_const_sptr source = instrument->getSource();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      m_samplePos = sample->getPos();
      m_beamDir = m_samplePos - source->getPos();
      m_beamDir.normalize();
      // Is the instrument set up correctly
      double l1(0.0);
      try
      {
        l1 = source->getDistance(*sample);
        g_log.debug() << "Source-sample distance: " << l1 << std::endl;
      }
      catch (Exception::NotFoundError &)
      {
        throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", 
                                                   workspace->getTitle());
      }
      // Index Q cache
      initQCache(workspace);
    }

    /**
     * A map detector ID and Q ranges
     * This method looks unnecessary as it could be calculated on the fly but
     * the parallelization means that lazy instantation slows it down due to the 
     * necessary CRITICAL sections required to update the cache. The Q range 
     * values are required very frequently so the total time is more than
     * offset by this precaching step
     */
    void SofQW2::initQCache(API::MatrixWorkspace_const_sptr workspace)
    {
      Mantid::Kernel::Timer clock;
      const size_t nhist(workspace->getNumberHistograms());
      std::vector<double> theta_pts;
      theta_pts.reserve(nhist);
      for(size_t i = 0 ; i < nhist; ++i)
      {
        try
        {
          IDetector_sptr det = workspace->getDetector(i);
          theta_pts.push_back(workspace->detectorTwoTheta(det)*180.0/M_PI);
        }
        catch(Kernel::Exception::NotFoundError&)
        {
          continue;
        }
      }
      // Make the bin boundaries
      std::sort(theta_pts.begin(), theta_pts.end());
      const size_t npts = theta_pts.size();
      m_thetaIndex.resize(npts + 1);
      for( size_t i = 0; i < npts - 1; ++i )
      {
        m_thetaIndex[i+1] = 0.5*(theta_pts[i] + theta_pts[i+1]);
      }
      m_thetaIndex[0] = 2.0*theta_pts.front() - m_thetaIndex[1];
      m_thetaIndex[npts] = 2.0*theta_pts.back() - m_thetaIndex[npts - 1];
      const size_t nxpoints = workspace->blocksize();
      const MantidVec & X = workspace->readX(0);

      PARALLEL_FOR1(workspace)
      for(size_t i = 0 ; i < nhist; ++i)
      {
        PARALLEL_START_INTERUPT_REGION

        IDetector_sptr det;
        try
        {
           det = workspace->getDetector(i);
        }
        catch(Kernel::Exception::NotFoundError&)
        {
          continue;
        }
        std::vector<std::pair<double, double> > qvalues(nxpoints);
        DetectorGroup_sptr detGroup = boost::dynamic_pointer_cast<DetectorGroup>(det);
        if( detGroup )
        {
          std::vector<IDetector_sptr> dets = detGroup->getDetectors();
          const size_t ndets(dets.size());
          for( size_t i = 0; i < ndets; ++i )
          {
            IDetector_sptr det_i = dets[i];
            for( size_t j = 0; j < nxpoints; ++j)
            {
              qvalues[j] = calculateQRange(det_i, X[j], X[j+1]);
            }
            PARALLEL_CRITICAL(qcache_a)
            {
              m_qcache.insert(std::make_pair(det_i->getID(), qvalues));
            }
          }
        }
        else
        {
          for( size_t j = 0; j < nxpoints; ++j)
          {
            qvalues[j] = calculateQRange(det, X[j], X[j+1]);
          }
          PARALLEL_CRITICAL(qcache_b)
          {
            m_qcache.insert(std::make_pair(det->getID(), qvalues));
          }
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      g_log.debug() << "Initializing Q Cache took " << clock.elapsed() << " seconds\n";
      // Clear theta bins and STL Trick to release memory
      m_thetaIndex.clear();
      std::vector<double>().swap(m_thetaIndex); 
    }

  } // namespace Mantid
} // namespace Algorithms

