//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidCurveFitting/CalculateMSVesuvio.h"
// Use helpers for storing detector/resolution parameters
#include "MantidCurveFitting/ConvertToYSpace.h"
#include "MantidCurveFitting/MSVesuvioHelpers.h"
#include "MantidCurveFitting/VesuvioResolution.h"

#include "MantidAPI/SampleShapeValidator.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/Track.h"

#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/VectorHelper.h"

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
      const size_t MAX_SCATTER_PT_TRIES = 25;
      /// Conversion constant
      const double MASS_TO_MEV = 0.5*PhysicalConstants::NeutronMass/PhysicalConstants::meV;
    } // end anonymous namespace

    //-------------------------------------------------------------------------
    // Member functions
    //-------------------------------------------------------------------------

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(CalculateMSVesuvio)

    /// Constructor
    CalculateMSVesuvio::CalculateMSVesuvio() : Algorithm(),
      m_randgen(NULL),
      m_acrossIdx(0), m_upIdx(1), m_beamIdx(3), m_beamDir(), m_srcR2(0.0),
      m_halfSampleHeight(0.0), m_halfSampleWidth(0.0), m_halfSampleThick(0.0),
      m_sampleShape(NULL), m_sampleProps(NULL),
      m_detHeight(-1.0), m_detWidth(-1.0), m_detThick(-1.0),
      m_tmin(-1.0), m_tmax(-1.0), m_delt(-1.0), m_foilRes(-1.0),
      m_nscatters(0), m_nruns(0), m_nevents(0),
      m_progress(NULL), m_inputWS()
    {
    }

    /// Destructor
    CalculateMSVesuvio::~CalculateMSVesuvio()
    {
      delete m_randgen;
      delete m_progress;
      delete m_sampleProps;
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

      // -- Sample --
      auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
      positiveInt->setLower(1);
      declareProperty("NoOfMasses", -1, positiveInt,
                      "The number of masses contained within the sample");

      auto positiveNonZero = boost::make_shared<BoundedValidator<double>>();
      positiveNonZero->setLower(0.0);
      positiveNonZero->setLowerExclusive(true);
      declareProperty("SampleDensity", -1.0, positiveNonZero,
                      "The density of the sample in gm/cm^3");

      auto nonEmptyArray = boost::make_shared<ArrayLengthValidator<double>>();
      nonEmptyArray->setLengthMin(3);
      declareProperty(new ArrayProperty<double>("AtomicProperties", nonEmptyArray),
                      "Atomic properties of masses within the sample. "
                      "The expected format is 3 consecutive values per mass: "
                      "mass(amu), cross-section (barns) & s.d of Compton profile.");
      setPropertyGroup("NoOfMasses", "Sample");
      setPropertyGroup("SampleDensity", "Sample");
      setPropertyGroup("AtomicProperties", "Sample");

      // -- Beam --
      declareProperty("BeamRadius", -1.0, positiveNonZero,
                      "Radius, in cm, of beam");

      // -- Algorithm --
      declareProperty("Seed", 123456789, positiveInt,
                      "Seed the random number generator with this value");
      declareProperty("NumScatters", 3, positiveInt,
                      "Number of scattering orders to calculate");
      declareProperty("NumRuns", 10, positiveInt,
                      "Number of simulated runs per spectrum");
      declareProperty("NumEventsPerRun", 50000, positiveInt,
                      "Number of events per run");
      setPropertyGroup("Seed", "Algorithm");
      setPropertyGroup("NumScatters", "Algorithm");
      setPropertyGroup("NumRuns", "Algorithm");
      setPropertyGroup("NumEventsPerRun", "Algorithm");

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
      cacheInputs();

      // Create new workspaces
      MatrixWorkspace_sptr totalsc = WorkspaceFactory::Instance().create(m_inputWS);
      MatrixWorkspace_sptr multsc = WorkspaceFactory::Instance().create(m_inputWS);

      // Initialize random number generator
      m_randgen = new MSVesuvioHelper::RandomNumberGenerator(getProperty("Seed"));

      // Setup progress
      const size_t nhist = m_inputWS->getNumberHistograms();
      m_progress = new API::Progress(this, 0.0, 1.0, nhist*m_nruns*2);
      for(size_t i = 0; i < nhist; ++i)
      {

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
     * Caches inputs insuitable form for speed in later calculations
     */
    void CalculateMSVesuvio::cacheInputs()
    {
      // Algorithm
      int nscatters = getProperty("NumScatters");
      m_nscatters = static_cast<size_t>(nscatters);
      int nruns = getProperty("NumRuns");
      m_nruns = static_cast<size_t>(nruns);
      int nevents = getProperty("NumEventsPerRun");
      m_nevents = static_cast<size_t>(nevents);
      
      // -- Geometry --
      const auto instrument = m_inputWS->getInstrument();
      m_beamDir = instrument->getSample()->getPos() - instrument->getSource()->getPos();
      m_beamDir.normalize();

      const auto rframe = instrument->getReferenceFrame();
      m_acrossIdx = rframe->pointingHorizontal();
      m_upIdx = rframe->pointingUp();
      m_beamIdx = rframe->pointingAlongBeam();

      m_srcR2 = getProperty("BeamRadius");
      // Convert to metres
      m_srcR2 /= 100.0;

      // Sample shape
      m_sampleShape = &(m_inputWS->sample().getShape());
      // We know the shape is valid from the property validator
      // Use the bounding box as an approximation to determine the extents
      // as this will match in both height and width for a cuboid & cylinder
      // sample shape
      Geometry::BoundingBox bounds = m_sampleShape->getBoundingBox();
      V3D boxWidth = bounds.width();
      // Use half-width/height for easier calculation later
      m_halfSampleWidth = 0.5*boxWidth[m_acrossIdx];
      m_halfSampleHeight = 0.5*boxWidth[m_upIdx];
      m_halfSampleThick = 0.5*boxWidth[m_beamIdx];

      // -- Workspace --
      const auto & inX = m_inputWS->readX(0);
      m_tmin = inX.front()*1e-06;
      m_tmax = inX.back()*1e-06;
      m_delt = (inX[1] - inX.front());

      // -- Sample --
      int nmasses = getProperty("NoOfMasses");
      std::vector<double> sampleInfo = getProperty("AtomicProperties");
      const int nInputAtomProps = static_cast<int>(sampleInfo.size());
      const int nExptdAtomProp(3);
      if(nInputAtomProps != nExptdAtomProp*nmasses)
      {
        std::ostringstream os;
        os << "Inconsistent AtomicProperties list defined. Expected " << nExptdAtomProp*nmasses
           << " values, however, only " << sampleInfo.size() << " have been given.";
        throw std::invalid_argument(os.str());
      }
      const int natoms = nInputAtomProps/3;
      m_sampleProps = new SampleComptonProperties(natoms);
      m_sampleProps->density = getProperty("SampleDensity");

      double totalMass(0.0); // total mass in grams
      m_sampleProps->totalxsec = 0.0;
      for(int i = 0; i < natoms; ++i)
      {
        auto & comptonAtom = m_sampleProps->atoms[i];
        comptonAtom.mass = sampleInfo[nExptdAtomProp*i];
        totalMass += comptonAtom.mass*PhysicalConstants::AtomicMassUnit*1000;

        const double xsec = sampleInfo[nExptdAtomProp*i + 1];
        comptonAtom.sclength = sqrt(xsec/(4.0*M_PI));
        const double factor = 1.0 + (PhysicalConstants::NeutronMassAMU/comptonAtom.mass);
        m_sampleProps->totalxsec += (xsec/(factor*factor));

        comptonAtom.profile = sampleInfo[nExptdAtomProp*i + 2];
      }
      const double numberDensity = m_sampleProps->density*1e6/totalMass; // formula units/m^3
      m_sampleProps->mu = numberDensity*m_sampleProps->totalxsec*1e-28;

      // -- Detector geometry -- choose first detector that is not a monitor
      Geometry::IDetector_const_sptr detPixel;
      for(size_t i = 0; i < m_inputWS->getNumberHistograms(); ++i)
      {
        try
        {
          detPixel = m_inputWS->getDetector(i);
        }
        catch(Exception::NotFoundError &)
        {
          continue;
        }
        if(!detPixel->isMonitor()) break;
      }
      // Bounding box in detector frame
      Geometry::Object_const_sptr pixelShape = detPixel->shape();
      if(!pixelShape || !pixelShape->hasValidShape())
      {
        throw std::invalid_argument("Detector pixel has no defined shape!");
      }
      Geometry::BoundingBox detBounds = pixelShape->getBoundingBox();
      V3D detBoxWidth = detBounds.width();
      m_detWidth = detBoxWidth[m_acrossIdx];
      m_detHeight = detBoxWidth[m_upIdx];
      m_detThick = detBoxWidth[m_beamIdx];

      // Foil resolution
      auto foil = instrument->getComponentByName("foil-pos0");
      if(!foil)
      {
        throw std::runtime_error("Workspace has no gold foil component defined.");
      }
      auto param = m_inputWS->instrumentParameters().get(foil.get(), "hwhm_lorentz");
      if(!param)
      {
        throw std::runtime_error("Foil component has no hwhm_lorentz parameter defined.");
      }
      m_foilRes = param->value<double>();
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
      detpar.t0 *= 1e6; // t0 in microseconds here
      ResolutionParams respar = VesuvioResolution::getResolutionParameters(m_inputWS, wsIndex);

      // Final counts averaged over all simulations
      MSVesuvioHelper::SimulationAggregator accumulator(m_nruns);
      for(size_t i = 0; i < m_nruns; ++i)
      {
        m_progress->report("MS calculation: idx=" + boost::lexical_cast<std::string>(wsIndex)
                           + ", run=" + boost::lexical_cast<std::string>(i));
        
        simulate(detpar, respar,
                 accumulator.newSimulation(m_nscatters, m_inputWS->blocksize()));

        m_progress->report("MS calculation: idx=" + boost::lexical_cast<std::string>(wsIndex)
                           + ", run=" + boost::lexical_cast<std::string>(i));
      }

      // Average over all runs and assign to output workspaces
      MSVesuvioHelper::SimulationWithErrors avgCounts = accumulator.average();
      avgCounts.normalise();
      assignToOutput(avgCounts, totalsc, multsc);
    }

    /**
     * Perform a single simulation of a given number of events for up to a maximum number of
     * scatterings on a chosen detector
     * @param detpar Detector information describing the final detector position
     * @param respar Resolution information on the intrument as a whole
     * @param simulCounts Simulation object used to storing the calculated number of counts
     */
    void CalculateMSVesuvio::simulate(const DetectorParams & detpar,
                                      const ResolutionParams &respar,
                                      MSVesuvioHelper::Simulation & simulCounts) const
    {
      for(size_t i = 0; i < m_nevents; ++i)
      {
        calculateCounts(detpar, respar, simulCounts);
      }
    }

    /**
     * Assign the averaged counts to the correct workspaces
     * @param avgCounts Counts & errors separated for each scattering order
     * @param totalsc A non-const reference to the spectrum for the total scattering calculation
     * @param multsc A non-const reference to the spectrum for the multiple scattering contribution
     */
    void
    CalculateMSVesuvio::assignToOutput(const  MSVesuvioHelper::SimulationWithErrors &avgCounts,
                                       API::ISpectrum & totalsc, API::ISpectrum & multsc) const
    {
      // Sum up all multiple scatter events
      auto & msscatY = multsc.dataY();
      auto & msscatE = multsc.dataE();
      for(size_t i = 1; i < m_nscatters; ++i) //(i >= 1 for multiple scatters)
      {
        const auto & counts = avgCounts.sim.counts[i];
        // equivalent to msscatY[j] += counts[j]
        std::transform(counts.begin(), counts.end(), msscatY.begin(), msscatY.begin(),
                       std::plus<double>());
        const auto & scerrors = avgCounts.errors[i];
        // sum errors in quadrature
        std::transform(scerrors.begin(), scerrors.end(), msscatE.begin(), msscatE.begin(),
                       VectorHelper::SumGaussError<double>());
      }
      // for total scattering add on single-scatter events
      auto & totalscY = totalsc.dataY();
      auto & totalscE = totalsc.dataE();
      const auto & counts0 = avgCounts.sim.counts.front();
      std::transform(counts0.begin(), counts0.end(), msscatY.begin(), totalscY.begin(),
                     std::plus<double>());
      const auto & errors0 = avgCounts.errors.front();
      std::transform(errors0.begin(), errors0.end(), msscatE.begin(), totalscE.begin(),
                     VectorHelper::SumGaussError<double>());
    }

    /**
     * @param detpar Detector information describing the final detector position
     * @param respar Resolution information on the intrument as a whole
     * @param simulation [Output] Store the calculated counts here
     * @return The sum of the weights for all scatters
     */
    double CalculateMSVesuvio::calculateCounts(const DetectorParams &detpar,
                                               const ResolutionParams &respar,
                                               MSVesuvioHelper::Simulation &simulation) const
    {
      double weightSum(0.0);

      // moderator coord in lab frame
      V3D srcPos = generateSrcPos(detpar.l1);
      if(fabs(srcPos[m_acrossIdx]) > m_halfSampleWidth ||
         fabs(srcPos[m_upIdx]) > m_halfSampleHeight)
      {
        return 0.0; // misses sample
      }

      // track various variables during calculation
      std::vector<double> weights(m_nscatters, 1.0), // start at 1.0
        tofs(m_nscatters, 0.0), // tof accumulates for each piece of the calculation
        en1(m_nscatters, 0.0);

      const double vel2 = sqrt(detpar.efixed/MASS_TO_MEV);
      const double t2 = detpar.l2/vel2;
      en1[0] = generateE0(detpar.l1, t2, weights[0]);
      tofs[0] = generateTOF(en1[0], respar.dtof, respar.dl1); // correction for resolution in l1

      // Neutron path
      std::vector<V3D> scatterPts(m_nscatters), // track origin of each scatter
          neutronDirs(m_nscatters); // neutron directions
      V3D startPos(srcPos);
      neutronDirs[0] = m_beamDir;

      generateScatter(startPos, neutronDirs[0], weights[0], scatterPts[0]);
      double distFromStart = startPos.distance(scatterPts[0]);
      // Compute TOF for first scatter event
      const double vel0 = sqrt(en1[0]/MASS_TO_MEV);
      tofs[0] += (distFromStart*1e6/vel0);

      // multiple scatter events within sample, i.e not including zeroth
      for(size_t i = 1; i < m_nscatters; ++i)
      {
        weights[i] = weights[i-1];
        tofs[i] = tofs[i-1];

        // Generate a new direction of travel
        const V3D & prevSc = scatterPts[i-1];
        V3D & curSc = scatterPts[i];
        const V3D & oldDir = neutronDirs[i-1];
        V3D & newDir = neutronDirs[i];
        size_t ntries(0);
        do
        {
          const double randth = acos(2.0*m_randgen->flat() - 1.0);
          const double randphi = 2.0*M_PI*m_randgen->flat();
          newDir.azimuth_polar_SNS(1.0, randphi, randth);

          // Update weight
          const double wgt = weights[i];
          if(generateScatter(prevSc, newDir, weights[i], curSc))
            break;
          else
          {
            weights[i] = wgt; // put it back to what it was
            ++ntries;
          }
        }
        while(ntries < MAX_SCATTER_PT_TRIES);
        if(ntries == MAX_SCATTER_PT_TRIES)
        {
          throw std::runtime_error("Unable to generate scatter point in sample. Check sample shape.");
        }

        const double scang = newDir.angle(oldDir);
        auto e1range = calculateE1Range(scang, en1[i-1]);
        en1[i] = e1range.first + m_randgen->flat()*(e1range.second - e1range.first);
        const double d2sig = partialDiffXSec(en1[i-1], en1[i], scang);
        double weight = d2sig*4.0*M_PI*(e1range.second - e1range.first)/m_sampleProps->totalxsec;
        // accumulate total weight
        weightSum += weight;
        weights[i] *= weight; // account for this scatter on top of previous

        // Increment time of flight...
        const double veli = sqrt(en1[i]/MASS_TO_MEV);
        tofs[i] += (curSc.distance(prevSc)*1e6/veli);
      }

      // force all orders in to current detector
      const auto & inX = m_inputWS->readX(0);
      for(size_t i = 0; i < m_nscatters; ++i)
      {
        double scang(0.0), distToExit(0.0);
        V3D detPos = generateDetectorPos(detpar.pos, en1[i], scatterPts[i], neutronDirs[i],
                                         scang, distToExit);
        // Weight by probability neutron leaves sample
        double & curWgt = weights[i];
        curWgt *= exp(-m_sampleProps->mu*distToExit);
        // Weight by cross-section for the final energy
        const double efinal = generateE1(detpar.theta, detpar.efixed, m_foilRes);
        curWgt *= partialDiffXSec(en1[i], efinal, scang)/m_sampleProps->totalxsec;
        // final TOF
        const double veli = sqrt(efinal/MASS_TO_MEV);
        tofs[i] += detpar.t0 + (scatterPts[i].distance(detPos)*1e6)/veli;
        // "Bin" weight into appropriate place
        std::vector<double> &counts = simulation.counts[i];
        const double finalTOF = tofs[i];

        for (size_t it = 0; it < inX.size(); ++it)
        {
          if (inX[it] - 0.5*m_delt < finalTOF && finalTOF < inX[it] + 0.5*m_delt)
          {
            counts[it] += curWgt;
            break;
          }
        }
      }

      return weightSum;
    }

    /**
      * Sample from the moderator assuming it can be seen
      * as a cylindrical ring with inner and outer radius
      * @param l1 Src-sample distance (m)
      * @returns Position on the moderator of the generated point
      */
    V3D CalculateMSVesuvio::generateSrcPos(const double l1) const
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
      V3D srcPos;
      srcPos[m_acrossIdx] = widthPos;
      srcPos[m_upIdx] = heightPos;
      srcPos[m_beamIdx] = -l1;

      return srcPos;
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

      weight = 2.0*en0/t1/pow(en0, 0.9);
      weight *= 1e-4; // Reduce weight to ~1

      return en0;
    }

    /**
     * Generate an initial tof from this distribution:
     * 1-(0.5*X**2/T0**2+X/T0+1)*EXP(-X/T0), where x is the time and t0
     * is the src-sample time.
     * @param dtof Error in time resolution (us)
     * @param en0 Value of the incident energy
     * @param dl1 S.d of moderator to sample distance
     * @return tof Guass TOF modified for asymmetric pulse
     */
    double CalculateMSVesuvio::generateTOF(const double en0, const double dtof,
                                           const double dl1) const
    {
      const double vel1 = sqrt(en0/MASS_TO_MEV);
      const double dt1 = (dl1/vel1)*1e6;
      const double xmin(0.0), xmax(15.0*dt1);
      double dx = 0.5*(xmax - xmin);
      // Generate a random y position in th distribution
      const double yv = m_randgen->flat();

      double xt(xmin);
      double tof = m_randgen->gaussian(0.0, dtof);
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
     * @param scatterPt [Out] Generated scattering point
     * @return True if the scatter event was generated, false otherwise
     */
    bool CalculateMSVesuvio::generateScatter(const Kernel::V3D &startPos, const Kernel::V3D &direc,
                                             double &weight, V3D &scatterPt) const
    {
      Track particleTrack(startPos, direc);
      if(m_sampleShape->interceptSurface(particleTrack) != 1)
      {
        return false;
      }
      // Find distance inside object and compute probability of scattering
      const auto & link = particleTrack.begin();
      double totalObjectDist = link->distInsideObject;
      const double scatterProb = 1.0 - exp(-m_sampleProps->mu*totalObjectDist);
      // Select a random point on the track that is the actual scatter point
      // from the scattering probability distribution
      const double dist = -log(1.0 - m_randgen->flat()*scatterProb)/m_sampleProps->mu;
      const double fraction = dist/totalObjectDist;
      // Scatter point is then entry point + fraction of width in each direction
      scatterPt = link->entryPoint;
      V3D edgeDistances = (link->exitPoint - link->entryPoint);
      scatterPt += edgeDistances*fraction;
      // Update weight
      weight *= scatterProb;
      return true;
    }

    /**
     * @param theta Neutron scattering angle (radians)
     * @param en0 Computed incident energy
     * @return The range of allowed final energies for the neutron
     */
    std::pair<double, double> CalculateMSVesuvio::calculateE1Range(const double theta, const double en0) const
    {
      const double k0 = sqrt(en0/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      const double sth(sin(theta)), cth(cos(theta));

      double e1min(1e10), e1max(-1e10); // large so that anything else is smaller
      const auto & atoms = m_sampleProps->atoms;
      for(size_t i = 0; i < atoms.size(); ++i)
      {
        const double mass = atoms[i].mass;

        const double fraction = (cth + sqrt(mass*mass - sth*sth))/(1.0 + mass);
        const double k1 = fraction*k0;
        const double en1 = PhysicalConstants::E_mev_toNeutronWavenumberSq*k1*k1;
        const double qr = sqrt(k0*k0 + k1*k1 - 2.0*k0*k1*cth);
        const double wr = en0 - en1;
        const double width = PhysicalConstants::E_mev_toNeutronWavenumberSq*atoms[i].profile*qr/mass;
        const double e1a = en0 - wr - 10.0*width;
        const double e1b = en0 - wr + 10.0*width;
        if(e1a < e1min) e1min = e1a;
        if(e1b > e1max) e1max = e1b;
      }
      if(e1min < 0.0) e1min = 0.0;
      return std::make_pair(e1min, e1max);
    }

    /**
     * Compute the partial differential cross section for this energy and theta.
     * @param en0 Initial energy (meV)
     * @param en1 Final energy (meV)
     * @param theta Scattering angle
     * @return Value of differential cross section
     */
    double CalculateMSVesuvio::partialDiffXSec(const double en0, const double en1, const double theta) const
    {
      const double rt2pi = sqrt(2.0*M_PI);

      const double k0 = sqrt(en0/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      const double k1 = sqrt(en1/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      const double q = sqrt(k0*k0 + k1*k1 - 2.0*k0*k1*cos(theta));
      const double w = en0 - en1;

      double pdcs(0.0);
      const auto & atoms = m_sampleProps->atoms;
      if(q > 0.0) // avoid continuous checking in loop
      {
        for(size_t i = 0; i < atoms.size(); ++i)
        {
          const double jstddev = atoms[i].profile;
          const double mass = atoms[i].mass;
          const double y = mass*w/(4.18036*q) - 0.5*q;
          const double jy = exp(-0.5*y*y/(jstddev*jstddev))/(jstddev*rt2pi);
          const double sqw = mass*jy/(4.18036*q);

          const double sclength = atoms[i].sclength;
          pdcs += sclength*sclength*(k1/k0)*sqw;
        }
      }
      else
      {
        for(size_t i = 0; i < atoms.size(); ++i)
        {
          const double sclength = atoms[i].sclength;
          pdcs += sclength*sclength;
        }
      }

      return pdcs;
    }

    /**
     * Generate a random position within the final detector in the lab frame
     * @param nominalPos The poisiton of the centre point of the detector
     * @param energy The final energy of the neutron
     * @param scatterPt The position of the scatter event that lead to this detector
     * @param direcBeforeSc Directional vector that lead to scatter point that hit this detector
     * @param scang [Output] The value of the scattering angle for the generated point
     * @param distToExit [Output] The distance covered within the object from scatter to exit
     * @return A new position in the detector
     */
    V3D CalculateMSVesuvio::generateDetectorPos(const V3D &nominalPos, const double energy,
                                                const V3D &scatterPt, const V3D &direcBeforeSc,
                                                double &scang, double &distToExit) const
    {
      const double mu = 7430.0/sqrt(energy); // Inverse attenuation length (m-1) for vesuvio det.
      const double ps = 1.0 - exp(-mu*m_detThick); // Probability of detection in path thickness.
      V3D detPos;
      scang = 0.0; distToExit = 0.0;
      size_t ntries(0);
      do
      {
        // Beam direction by moving to front of "box"define by detector dimensions and then
        // computing expected distance travelled based on probability
        detPos[m_beamIdx] = (nominalPos[m_beamIdx] - 0.5*m_detThick) - \
            (log(1.0 - m_randgen->flat()*ps)/mu);
        // perturb away from nominal position
        detPos[m_acrossIdx] = nominalPos[m_acrossIdx] + (m_randgen->flat() - 0.5)*m_detWidth;
        detPos[m_upIdx] = nominalPos[m_upIdx] + (m_randgen->flat() - 0.5)*m_detHeight;

        // Distance to exit the sample for this order
        V3D scToDet = detPos - scatterPt;
        scToDet.normalize();
        Geometry::Track scatterToDet(scatterPt, scToDet);
        if(m_sampleShape->interceptSurface(scatterToDet) > 0)
        {
          scang = direcBeforeSc.angle(scToDet);
          const auto & link = scatterToDet.begin();
          distToExit = link->distInsideObject;
          break;
        }
        // if point is very close surface then there may be no valid intercept so try again
        ++ntries;
      }
      while(ntries < MAX_SCATTER_PT_TRIES);
      if(ntries == MAX_SCATTER_PT_TRIES)
      {
        throw std::runtime_error("Unable to create track from sample to detector. "
                                 "Detector shape may be too small.");
      }
      return detPos;
    }

    /**
     * Generate the final energy of the analyser
     * @param angle Detector angle from sample
     * @param e1nom The nominal final energy of the analyzer
     * @param e1res The resoltion in energy of the analyser
     * @return A value for the final energy of the neutron
     */
    double CalculateMSVesuvio::generateE1(const double angle, const double e1nom,
                                          const double e1res) const
    {
      if(e1res == 0.0) return e1nom;

      const double randv = m_randgen->flat();
      if(e1nom < 5000.0)
      {
        if(angle > 90.0) return MSVesuvioHelper::finalEnergyAuDD(randv);
        else return MSVesuvioHelper::finalEnergyAuYap(randv);
      }
      else
      {
        return MSVesuvioHelper::finalEnergyUranium(randv);
      }
    }

  } // namespace Algorithms
} // namespace Mantid
