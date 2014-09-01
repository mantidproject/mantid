//-----------------------------------------------------------------------------
// Includes
//
#include "MantidCurveFitting/CalculateMSVesuvio.h"
// Use helpers for storing detector/resolution parameters
#include "MantidCurveFitting/ConvertToYSpace.h"
#include "MantidCurveFitting/VesuvioResolution.h"

#include "MantidAPI/SampleShapeValidator.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MersenneTwister.h"

#include <boost/make_shared.hpp>

namespace Mantid
{
  namespace CurveFitting
  {
    using namespace Kernel;
    using namespace API;

    namespace
    {
      const size_t NSIMULATIONS = 10;
      const size_t NEVENTS = 500000;
      const size_t NSCATTERS = 3;
    } // end anonymous namespace

    //-------------------------------------------------------------------------
    // InstrumentGeometry cache
    //-------------------------------------------------------------------------

    CalculateMSVesuvio::InstrumentGeometry::InstrumentGeometry() :
      refframe(), srcR1(0.0), srcR2(0.0),
      sampleHeight(0.0), sampleWidth(0.0)
    {
    }

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
      m_randgen(NULL), m_instgeom(),

      m_inputWS(),
      m_progress(NULL)
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
      const int64_t nhist = static_cast<int64_t>(m_inputWS->getNumberHistograms());

      // Create new workspaces
      MatrixWorkspace_sptr totalsc = WorkspaceFactory::Instance().create(m_inputWS);
      MatrixWorkspace_sptr multsc = WorkspaceFactory::Instance().create(m_inputWS);

      // Initialize random number generator
      m_randgen = new RandomNumberGenerator(getProperty("Seed"));

      // Setup progress
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
      const auto rframe = instrument->getReferenceFrame();
      m_instgeom.refframe = rframe;

      m_instgeom.srcR1 = getProperty("BeamUmbraRadius");
      m_instgeom.srcR2 = getProperty("BeamPenumbraRadius");
      if(m_instgeom.srcR2 < m_instgeom.srcR1)
      {
        std::ostringstream os;
        os << "Invalid beam radius parameters. Penumbra value="
           << m_instgeom.srcR2 << " < Umbra value="
           << m_instgeom.srcR1;
        throw std::invalid_argument(os.str());
      }

      // Sample shape
      const auto & sampleShape = m_inputWS->sample().getShape();
      // We know the shape is valid from the property validator but we
      // need to check if it is cuboid or cylinder
      int objType(-1);
      double radius(-1.0), height(-1.0);
      std::vector<V3D> pts;
      sampleShape.GetObjectGeom(objType, pts, radius, height);
      if(objType != 1 && objType != 3)
      {
        throw std::invalid_argument("Invalid sample shape. Currently only "
                                    "cuboid or cylinder are supported");
      }
      assert(pts.size() == 4);
      if(objType == 1) // cuboid
      {
        auto horiz = rframe->pointingHorizontal();
        m_instgeom.sampleWidth = fabs(pts[0][horiz] - pts[3][horiz]);
        auto up = rframe->pointingUp();
        m_instgeom.sampleHeight = fabs(pts[0][up] - pts[1][up]);
      }
      else
      {
        m_instgeom.sampleWidth = 2.0*radius;
        m_instgeom.sampleHeight = height;
      }
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
        simulCounts.weight += calculateTOF(nscatters, detpar, respar, simulCounts);
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
    double CalculateMSVesuvio::calculateTOF(const size_t nscatters, const DetectorParams &detpar,
                                            const ResolutionParams &respar,
                                            Simulation &counts) const
    {
      double weightSum(0.0);
      std::vector<double> weightsPerScatter(nscatters, 1.0); // start at 1.0
      std::vector<double> tofs(nscatters, 0.0);

      tofs[0] = initialTOF(0.0, detpar.t0);
      // moderator coord in lab frame
      V3D srcLab(0.0, -detpar.l1, 0.0);
      weightsPerScatter[0] *= moderatorPos(srcLab[0], srcLab[2]);
//      if(heightLab > m_instgeom.sampleHeight ||
//         widthLab > m_instgeom.sampleWidth) return 0.0; // misses sample


      return weightSum;
    }

    /**
     * Sample a tof value from a gaussian distribution
     * @param centre The value of the centra point of the distribution
     * @param sigma Width of the distribution
     * @return New variable distributed accordingly
     */
    double CalculateMSVesuvio::initialTOF(const double centre, const double sigma) const
    {
      return m_randgen->gaussian(centre, sigma);
    }

    /**
      * Sample from the moderator assuming it can be seen
      * as a cylindrical ring with inner and outer radius
      * @param widthPos [Out] Position in the width direction of the generated point
      * @param heightPos [Out] Position in the height direction of the generated point
      */
    double CalculateMSVesuvio::moderatorPos(double &widthPos, double &heightPos) const
    {
      double radius(-1.0);
      do
      {
        widthPos = -m_instgeom.srcR2 + 2.0*m_instgeom.srcR2*m_randgen->flat();
        heightPos = -m_instgeom.srcR2 + 2.0*m_instgeom.srcR2*m_randgen->flat();
        using std::sqrt;
        radius = sqrt(widthPos*widthPos + heightPos*heightPos);
      }
      while(radius > m_instgeom.srcR2);

      if(radius > m_instgeom.srcR1)
        return (m_instgeom.srcR2 - radius)/(m_instgeom.srcR2 - m_instgeom.srcR1);
      else
        return 1.0; // inside umbra unit weight
    }

  } // namespace Algorithms
} // namespace Mantid
