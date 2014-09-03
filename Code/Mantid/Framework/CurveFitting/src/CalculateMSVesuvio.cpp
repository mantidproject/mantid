//-----------------------------------------------------------------------------
// Includes
//
#include "MantidCurveFitting/CalculateMSVesuvio.h"
// Use helpers for storing detector/resolution parameters
#include "MantidCurveFitting/ConvertToYSpace.h"
#include "MantidCurveFitting/VesuvioResolution.h"

#include "MantidAPI/SampleShapeValidator.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/Track.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MersenneTwister.h"

#include <boost/make_shared.hpp>

namespace Mantid
{
  namespace CurveFitting
  {
    using namespace API;
    using namespace Kernel;
    using Geometry::Link;
    using Geometry::ParameterMap;
    using Geometry::Track;

    namespace
    {
      const size_t NSIMULATIONS = 10;
      const size_t NEVENTS = 100000;
      const size_t NSCATTERS = 3;
      const size_t MAX_SCATTER_PT_TRIES = 500;
      /// Conversion constant
      const double MASS_TO_MEV = 0.5*PhysicalConstants::NeutronMass/PhysicalConstants::meV;

    } // end anonymous namespace

    //-------------------------------------------------------------------------
    // RandomNumberGenerator helper
    //-------------------------------------------------------------------------
    /**
     * Produces random numbers with various probability distributions
     */
    CalculateMSVesuvio::
      RandomNumberGenerator::RandomNumberGenerator(const int seed) : m_generator()
    {
      m_generator.seed(static_cast<boost::mt19937::result_type>(seed));
    }
    /// Returns a flat random number between 0.0 & 1.0
    double CalculateMSVesuvio::
      RandomNumberGenerator::flat()
    {
      return uniform_double()(m_generator,
                              uniform_double::param_type(0.0, 1.0));
    }
    /// Returns a random number distributed  by a normal distribution
    double CalculateMSVesuvio::
      RandomNumberGenerator::gaussian(const double mean, const double sigma)
    {
      return gaussian_double()(m_generator,
                               gaussian_double::param_type(mean, sigma));
    }

    //-------------------------------------------------------------------------
    // Simulation helpers
    //-------------------------------------------------------------------------
    /**
     * Stores counts for each scatter order
     * for a "run" of a given number of events
     */
    CalculateMSVesuvio::Simulation::
      Simulation(const size_t order, const size_t ntimes) :
        counts(order, std::vector<double>(ntimes)),
        maxorder(order), weight(0.0), nmscat(0)
    {}

    /**
     * Accumulates and averages the results
     * of each simulation
     */
    CalculateMSVesuvio::SimulationAggregator::
      SimulationAggregator(const size_t nruns,
                           const size_t order,
                           const size_t ntimes) :
        averaged(order, ntimes),
        prefactor(1.0/static_cast<double>(nruns))
      {}

    /** Adds a result as part of the average
     * @param result A new simulation result
     */
    void CalculateMSVesuvio::SimulationAggregator::
      add(const Simulation & result)
    {
      // No check is performed whether the number of
      // stated runs has been reached or the order is the same
      for(size_t i = 0; i < averaged.maxorder; ++i)
      {
        auto & avgcounts = averaged.counts[i];
        const auto & rescounts = result.counts[i];
        std::vector<double>::iterator avgIter(avgcounts.begin());
        std::vector<double>::const_iterator resIter(rescounts.begin());
        for(; avgIter != avgcounts.end(); ++avgIter, ++resIter)
        {
          *avgIter += prefactor*(*resIter);
        }
      }
    }

    //-------------------------------------------------------------------------
    // Algorithm definitions
    //-------------------------------------------------------------------------

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(CalculateMSVesuvio)

    /// Constructor
    CalculateMSVesuvio::CalculateMSVesuvio() : Algorithm(),
      m_randgen(NULL),
      m_acrossDir(0), m_upDir(1), m_beamDir(3), m_srcR1(0.0), m_srcR2(0.0),
      m_halfSampleHeight(0.0), m_halfSampleWidth(0.0), m_halfSampleThick(0.0),
      m_maxWidthSampleFrame(0.0), m_goniometer(NULL), m_sampleShape(NULL), m_samplePos(),
      m_mu(-1.0),
      m_tmin(-1.0), m_tmax(-1.0), m_dt(-1.0),
      m_progress(NULL), m_inputWS()
    {
    }

    /// Destructor
    CalculateMSVesuvio::~CalculateMSVesuvio()
    {
      delete m_randgen;
    }

    /// @copydoc Algorithm::name
    const std::string CalculateMSVesuvio::name() const
    {
      return "CalculateMSVesuvio";
    }

    /// @copydoc Algorithm::version
    int CalculateMSVesuvio::version() const
    {
      return 1;
    }

    /// @copydoc Algorithm::category
    const std::string CalculateMSVesuvio::category() const
    {
      return "Corrections";
    }

    /// @copydoc Algorithm::summary
    const std::string CalculateMSVesuvio::summary() const
    {
      return "Corrects for the effects of multiple scattering "
             "on a flat plate or cylindrical sample.";
    }

    /**
     * Initialize the algorithm's properties.
     */
    void CalculateMSVesuvio::init()
    {
      // Inputs
      auto inputWSValidator = boost::make_shared<CompositeValidator>();
      inputWSValidator->add<WorkspaceUnitValidator>("TOF");
      inputWSValidator->add<SampleShapeValidator>();
      declareProperty(new WorkspaceProperty<>("InputWorkspace","",
                                              Direction::Input, inputWSValidator),
                      "Input workspace to be corrected, in units of TOF.");

      auto positiveNonZero = boost::make_shared<BoundedValidator<double>>();
      positiveNonZero->setLower(0.0);
      positiveNonZero->setLowerExclusive(true);
      declareProperty("BeamUmbraRadius", -1.0, positiveNonZero,
                      "Radius, in cm, of part in total shadow.");
      declareProperty("BeamPenumbraRadius", -1.0, positiveNonZero,
                      "Radius, in cm, of part in partial shadow.");
      setPropertyGroup("BeamUmbraRadius", "Beam");
      setPropertyGroup("BeamPenumbraRadius", "Beam");

      auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
      positiveInt->setLower(1);
      declareProperty("Seed", 123456789, positiveInt,
                      "Seed the random number generator with this value");

      // Outputs
      declareProperty(new WorkspaceProperty<>("TotalScatteringWS","", Direction::Output),
                      "Workspace to store the calculated total scattering counts");
      declareProperty(new WorkspaceProperty<>("MultipleScatteringWS","", Direction::Output),
                      "Workspace to store the calculated total scattering counts");
    }

    /**
     * Execute the algorithm.
     */
    void CalculateMSVesuvio::exec()
    {
      m_inputWS = getProperty("InputWorkspace");
      cacheInputGeometry();
      // Values used frequently
      const auto & inX = m_inputWS->readX(0);
      m_tmin = inX.front();
      m_tmax = inX.back();
      m_dt = inX[1] - m_tmin;

      // Create new workspaces
      MatrixWorkspace_sptr totalsc = WorkspaceFactory::Instance().create(m_inputWS);
      MatrixWorkspace_sptr multsc = WorkspaceFactory::Instance().create(m_inputWS);

      // Initialize random number generator
      m_randgen = new RandomNumberGenerator(getProperty("Seed"));

      // Setup progress
      const int64_t nhist = static_cast<int64_t>(m_inputWS->getNumberHistograms());
      m_progress = new API::Progress(this, 0.0, 1.0, nhist);
      for(int64_t i = 0; i < nhist; ++i)
      {
        m_progress->report("Calculating corrections");

        // Copy over the X-values
        const MantidVec & xValues = m_inputWS->readX(i);
        totalsc->dataX(i) = xValues;
        multsc->dataX(i) = xValues;

        // Final detector position
        Geometry::IDetector_const_sptr detector;
        try
        {
          detector = m_inputWS->getDetector(i);
        }
        catch(Kernel::Exception::NotFoundError&)
        {
          // intel compiler doesn't like continue in a catch inside an OMP loop
        }
        if(!detector)
        {
          std::ostringstream os;
          os << "No valid detector object found for spectrum at workspace index '" << i << "'. No correction calculated.";
          g_log.information(os.str());
          continue;
        }

        // the output spectrum objects have references to where the data will be stored
        calculateMS(i, *totalsc->getSpectrum(i), *multsc->getSpectrum(i));
      }

      setProperty("TotalScatteringWS", totalsc);
      setProperty("MultipleScatteringWS", multsc);
    }

    /**
     * Caches sample, detector geometry for speed in later calculations
     */
    void CalculateMSVesuvio::cacheInputGeometry()
    {
      const auto instrument = m_inputWS->getInstrument();
      m_samplePos = instrument->getSample()->getPos();

      const auto rframe = instrument->getReferenceFrame();
      m_acrossDir = rframe->pointingHorizontal();
      m_upDir = rframe->pointingUp();
      m_beamDir = rframe->pointingAlongBeam();

      m_srcR1 = getProperty("BeamUmbraRadius");
      m_srcR2 = getProperty("BeamPenumbraRadius");
      if(m_srcR2 < m_srcR1)
      {
        std::ostringstream os;
        os << "Invalid beam radius parameters. Penumbra value="
           << m_srcR2 << " < Umbra value="
           << m_srcR1;
        throw std::invalid_argument(os.str());
      }

      // Sample rotation specified by a goniometer
      m_goniometer = &(m_inputWS->run().getGoniometerMatrix());
      // Sample shape
      m_sampleShape = &(m_inputWS->sample().getShape());
      // We know the shape is valid from the property validator
      // Use the bounding box as an approximation to determine the extents
      // as this will match in both height and width for a cuboid & cylinder
      // sample shape
      Geometry::BoundingBox bounds = m_sampleShape->getBoundingBox();
      V3D boxWidth = bounds.width();
      // Use half-width/height for easier calculation later
      m_halfSampleWidth = 0.5*boxWidth[m_acrossDir];
      m_halfSampleHeight = 0.5*boxWidth[m_upDir];
      m_halfSampleThick = 0.5*boxWidth[m_beamDir];
    }

    /**
     * Calculate the total scattering and contributions from higher-order scattering for given
     * spectrum
     * @param wsIndex The index on the input workspace for the chosen spectrum
     * @param totalsc A non-const reference to the spectrum that will contain the total scattering calculation
     * @param multsc A non-const reference to the spectrum that will contain the multiple scattering contribution
     */
    void CalculateMSVesuvio::calculateMS(const size_t wsIndex, API::ISpectrum & totalsc,
                                         API::ISpectrum & multsc) const
    {
      // Detector information
      DetectorParams detpar = ConvertToYSpace::getDetectorParameters(m_inputWS, wsIndex);
      // t0 is stored in seconds here, whereas here we want microseconds
      detpar.t0 *= 1e6;

      const Geometry::IDetector_const_sptr detector = m_inputWS->getDetector(wsIndex);
      const auto & pmap = m_inputWS->instrumentParameters();
      // Resolution information
      ResolutionParams respar;
      respar.dl1 = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_l1");
      respar.dl2 = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_l2");
      respar.dthe = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_theta"); //radians
      respar.dEnLorentz = ConvertToYSpace::getComponentParameter(detector, pmap, "hwhm_lorentz");
      respar.dEnGauss = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_gauss");

      // Final counts averaged over all simulations
      SimulationAggregator avgCounts(NSIMULATIONS, NSCATTERS, m_inputWS->blocksize());
      for(size_t i = 0; i < NSIMULATIONS; ++i)
      {
        avgCounts.add(simulate(NEVENTS, NSCATTERS, detpar, respar));
      }

    }

    /**
     * Perform a single simulation of a given number of events for up to a maximum number of
     * scatterings on a chosen detector
     * @param nevents The number of neutron events to simulate
     * @param nscatters Maximum order of scattering that should be simulated
     * @param detpar Detector information describing the final detector position
     * @param respar Resolution information on the intrument as a whole
     * @return A new simulation object storing the calculated number of counts
     */
    CalculateMSVesuvio::Simulation
      CalculateMSVesuvio::simulate(const size_t nevents, const size_t nscatters,
                                   const DetectorParams & detpar,
                                   const ResolutionParams &respar) const
    {
      Simulation simulCounts(NSCATTERS, m_inputWS->blocksize());
      for(size_t i = 0; i < nevents; ++i)
      {
        simulCounts.weight += calculateCounts(nscatters, detpar, respar, simulCounts);
      }

      return simulCounts;
    }

    /**
     *
     * @param nscatters Maximum order of scattering that should be simulated
     * @param detpar Detector information describing the final detector position
     * @param respar Resolution information on the intrument as a whole
     * @param counts [Output] Store the calculated counts here
     * @return
     */
    double CalculateMSVesuvio::calculateCounts(const size_t nscatters, const DetectorParams &detpar,
                                               const ResolutionParams &respar,
                                               Simulation &counts) const
    {
      double weightSum(0.0);
      std::vector<double> weights(nscatters, 1.0); // start at 1.0
      std::vector<double> tofs(nscatters, 0.0);

      // Initial TOF based on uncertainty in time measurement on detector

      // moderator coord in lab frame
      V3D srcPos;
      generateSrcPos(detpar.l1, srcPos); // fills in x,y,z
      // transform to sample frame
      srcPos.rotate(*m_goniometer);
      if(fabs(srcPos[0]) > m_halfSampleWidth ||
         fabs(srcPos[1]) > m_halfSampleHeight) return 0.0; // misses sample

      const double vel2 = sqrt(detpar.efixed/MASS_TO_MEV);
      const double t2 = detpar.l2/vel2;
      double en0 = generateE0(detpar.l1, t2, weights[0]);
      tofs[0] = generateTOF(en0, detpar.t0, respar.dl1); // correction for resolution in l1

      // Neutron paths. No beam divergence so initial direction is parallel to beam direction
      V3D startPos(srcPos);
      V3D particleDir;
      particleDir[m_beamDir] = 1.0;

      V3D scatterPt = generateScatter(startPos, particleDir, weights[0]);
      double distFromStart = scatterPt.distance(startPos);
      // Compute TOF for first scatter event
      double vel = sqrt(en0/MASS_TO_MEV);
      tofs[0] += (distFromStart*1e6/vel);

      return weightSum;
    }

    /**
      * Sample from the moderator assuming it can be seen
      * as a cylindrical ring with inner and outer radius
      * @param l1 Src-sample distance (m)
      * @param srcPos [Out] Position in the height direction of the generated point
      */
    double CalculateMSVesuvio::generateSrcPos(const double l1,
                                              Kernel::V3D &srcPos) const
    {
      double radius(-1.0), widthPos(0.0), heightPos(0.0);
      do
      {
        widthPos = -m_srcR2 + 2.0*m_srcR2*m_randgen->flat();
        heightPos = -m_srcR2 + 2.0*m_srcR2*m_randgen->flat();
        using std::sqrt;
        radius = sqrt(widthPos*widthPos + heightPos*heightPos);
      }
      while(radius > m_srcR2);
      // assign to output
      srcPos[m_acrossDir] = widthPos;
      srcPos[m_upDir] = heightPos;
      srcPos[m_beamDir] = -l1;

      if(radius > m_srcR1)
        return (m_srcR2 - radius)/(m_srcR2 - m_srcR1);
      else
        return 1.0; // inside umbra unit weight
    }

    /**
     * Generate an incident energy based on a randomly-selected TOF value
     * It is assigned a weight = (2.0*E0/(T-t2))/E0^0.9.
     * @param l1 Distance from src to sample (metres)
     * @param t2 Nominal time from sample to detector (seconds)
     * @param weight [Out] Weight factor to modify for the generated energy value
     * @return
     */
    double CalculateMSVesuvio::generateE0(const double l1, const double t2, double &weight) const
    {
      const double tof = m_tmin + (m_tmax - m_tmin)*m_randgen->flat();
      const double t1 = (tof - t2);
      const double vel0 = l1/t1;
      const double en0 = MASS_TO_MEV*vel0*vel0;

      weight *= 2.0*weight/t1/pow(weight, 0.9);
      weight *= 1e-4; // Reduce weight to ~1

      return en0;
    }

    /**
     * Generate an initial tof from this distribution:
     * 1-(0.5*X**2/T0**2+X/T0+1)*EXP(-X/T0), where x is the time and t0
     * is the src-sample time.
     * @param dt0 Error in time resolution (us)
     * @param en0 Value of the incident energy
     * @param dl1 S.d of moderator to sample distance
     * @return tof Guass TOF modified for asymmetric pulse
     */
    double CalculateMSVesuvio::generateTOF(const double en0, const double dt0,
                                           const double dl1) const
    {
      const double vel1 = sqrt(en0/MASS_TO_MEV);
      const double dt1 = (dl1/vel1)*1e6;
      const double xmin(0.0), xmax(15.0*dt1);
      double dx = 0.5*(xmax - xmin);
      // Generate a random y position in th distribution
      const double yv = m_randgen->flat();

      double xt(xmin);
      double tof = m_randgen->gaussian(0.0, dt0);
      while(true)
      {
        xt += dx;
        //Y=1-(0.5*X**2/T0**2+X/T0+1)*EXP(-X/T0)
        double y = 1.0 - (0.5*xt*xt/(dt1*dt1) + xt/dt1 + 1)*exp(-xt/dt1);
        if(fabs(y - yv) < 1e-4)
        {
          tof += xt - 3*dt1;
          break;
        }
        if(y > yv)
        {
          dx = -fabs(0.5*dx);
        }
        else
        {
          dx = fabs(0.5*dx);
        }
      }
      return tof;
    }

    /**
     * Generate a scatter event and update the weight according to the
     * amount the beam would be attenuted by the sample
     * @param startPos Starting position
     * @param direc Direction of travel for the neutron
     * @param weight [InOut] Multiply the incoming weight by the attenuation factor
     * @return The generated scattering point
     */
    Kernel::V3D CalculateMSVesuvio::generateScatter(const Kernel::V3D &startPos, const Kernel::V3D &direc,
                                                    double &weight) const
    {
      Track particleTrack(startPos, direc);
      if(m_sampleShape->interceptSurface(particleTrack) != 1)
      {
        throw std::runtime_error("CalculateMSVesuvio::calculateCounts - "
                                 "Sample shape appears to have a hole in it?. Unable to continue");
      }
      // Find distance inside object and compute probability of scattering
      const auto & link = particleTrack.begin();
      double totalObjectDist = link->distInsideObject;
      const double scatterProb = 1.0 - exp(-m_mu*totalObjectDist);
      // Select a random point on the track that is the actual scatter point
      // from the scattering probability distribution
      const double dist = -log(1.0 - m_randgen->flat()*scatterProb)/m_mu;
      // From start point advance in direction of travel by computed distance to find scatter point
      // Track is defined as set of links and exit point of first link is entry to sample!
      V3D scatterPt = link->entryPoint;
      scatterPt += direc*dist;
      // Update weight
      weight *= scatterProb;

      return scatterPt;
    }

  } // namespace Algorithms
} // namespace Mantid
