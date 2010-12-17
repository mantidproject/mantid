//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/MonteCarloAbsorption.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SampleEnvironment.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/NeutronAtom.h"

// @todo: This needs a factory
#include "MantidKernel/MersenneTwister.h"

/// @cond
namespace
{
  /// Number of attempts to choose a random point within the object before it gives up
  const int MaxRandPointAttempts(20);
}
/// @endcond

namespace Mantid
{
  namespace Algorithms
  {
    
    DECLARE_ALGORITHM(MonteCarloAbsorption)

    using API::WorkspaceProperty;
    using API::CompositeValidator;
    using API::WorkspaceUnitValidator;
    using API::InstrumentValidator;
    using API::MatrixWorkspace_sptr;
    using API::WorkspaceFactory;
    using API::Progress;
    using namespace Geometry;
    using Kernel::Direction;

    //------------------------------------------------------------------------------
    // Public methods
    //------------------------------------------------------------------------------

    /**
     * Constructor
     */
    MonteCarloAbsorption::MonteCarloAbsorption() : 
      m_inputWS(), m_sampleShape(NULL), m_container(NULL), m_numberOfPoints(0), 
      m_xStepSize(0), m_numberOfEvents(1), m_samplePos(), m_sourcePos(), 
      m_bbox_length(0.0), m_bbox_halflength(0.0), m_bbox_width(0.0), 
      m_bbox_halfwidth(0.0), m_bbox_height(0.0), m_bbox_halfheight(0.0), 
      m_randGen(NULL)
    {
    }
    
    /**
     * Destructor
     */
    MonteCarloAbsorption::~MonteCarloAbsorption()
    {
      delete m_randGen;
    }

    //------------------------------------------------------------------------------
    // Private methods
    //------------------------------------------------------------------------------
    
    /** 
     * Initialize the algorithm
     */
    void MonteCarloAbsorption::init()
    {
      // The input workspace must have an instrument and units of wavelength
      CompositeValidator<> * wsValidator = new CompositeValidator<>;
      wsValidator->add(new WorkspaceUnitValidator<> ("Wavelength"));
      wsValidator->add(new InstrumentValidator<>());

      declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
          wsValidator),
          "The X values for the input workspace must be in units of wavelength");
      declareProperty(new WorkspaceProperty<> ("OutputWorkspace", "", Direction::Output),
          "Output workspace name");
      Kernel::BoundedValidator<int> *positiveInt = new Kernel::BoundedValidator<int> ();
      positiveInt->setLower(1);
      declareProperty("NumberOfWavelengthPoints", EMPTY_INT(), positiveInt,
          "The number of wavelength points for which a simulation is\n"
          "performed (default: all points)");
      declareProperty("EventsPerPoint", 300, positiveInt->clone(),
          "The number of events to simulate per wavelength point used.");
      declareProperty("SeedValue", 123456789, positiveInt->clone(), 
          "A seed for the random number generator");

    }
    
    /**
     * Execution code
     */
    void MonteCarloAbsorption::exec()
    {
      retrieveInput();
      initCaches();      
      
      MatrixWorkspace_sptr correctionFactors = WorkspaceFactory::Instance().create(m_inputWS);
      correctionFactors->isDistribution(true); // The output of this is a distribution
      correctionFactors->setYUnit(""); // Need to explicitly set YUnit to nothing
      correctionFactors->setYUnitLabel("Attenuation factor");
      
      const bool isHistogram = m_inputWS->isHistogramData();
      const int numHists(m_inputWS->getNumberHistograms());
      const int numBins = m_inputWS->blocksize();
      
      // Compute the step size
      m_xStepSize = numBins/m_numberOfPoints;

      std::ostringstream message;
      message << "Simulation performed every " << m_xStepSize  << " wavelength points" << std::endl;
      g_log.information(message.str());
      message.str("");
      
      Progress prog(this,0.0,1.0,numHists);
      PARALLEL_FOR2(m_inputWS, correctionFactors)
      for( int i = 0; i < numHists; ++i )
      {
        PARALLEL_START_INTERUPT_REGION

        // Copy over the X-values
        const MantidVec & xValues = m_inputWS->readX(i);
        correctionFactors->dataX(i) = xValues;
        // Final detector position
        IDetector_const_sptr detector;
        try
        {
          detector = m_inputWS->getDetector(i);
        }
        catch(Kernel::Exception::NotFoundError&)
        {
          continue;
        }

        MantidVec & yValues = correctionFactors->dataY(i);
        MantidVec & eValues = correctionFactors->dataE(i);
        // Simulation for each requested wavelength point
        for( int bin = 0; bin < numBins; bin += m_xStepSize )
        {
          const double lambda = isHistogram ?
              (0.5 * (xValues[bin] + xValues[bin + 1]) ) : xValues[bin];
          doSimulation(detector.get(), lambda, yValues[bin], eValues[bin]);
          // Ensure we have the last point for the interpolation
          if ( m_xStepSize > 1 && bin + m_xStepSize >= numBins && bin+1 != numBins)
          {
            bin = numBins - m_xStepSize - 1;
          }
        }

        // Interpolate through points not simulated
        if( m_xStepSize > 1 )
        {
          Kernel::VectorHelper::linearlyInterpolateY(xValues, yValues, m_xStepSize);
        }
        prog.report();

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION


      // Save the results
      setProperty("OutputWorkspace", correctionFactors);
    }


    /**
     * Perform the simulation
     * @param detector A pointer to the current detector
     * @param lambda The chosen wavelength
     * @param attenFactor [Output] The calculated attenuation factor for this wavelength
     * @param error [Output] The value of the error on the factor
     */
    void MonteCarloAbsorption::doSimulation(const IDetector *const detector, const double lambda,
                                            double & attenFactor, double & error)
    {
      /**
       Currently, assuming square beam profile to pick start position then randomly selecting 
       a point within the sample using it's bounding box.
       This point defines the single scattering point and hence the attenuation path lengths and final 
       directional vector to the detector
       */
      // Absolute detector position
      const V3D detectorPos(detector->getPos());

      int numDetected(0);
      attenFactor = 0.0;
      error = 0.0;
      while( numDetected < m_numberOfEvents )
      {
        V3D startPos = sampleBeamProfile();
        V3D scatterPoint = selectScatterPoint();
        attenFactor += attenuationFactor(startPos, scatterPoint, detectorPos, lambda);
        ++numDetected;
      }

      // Attenuation factor is simply the average value
      attenFactor /= numDetected;
      // Error is 1/sqrt(nevents)
      error = 1./sqrt((double)numDetected);
    }

    /**
     * Sample the beam profile for a random start location, assigning a weight
     * to the given point
     * @returns The starting location
     */
    V3D MonteCarloAbsorption::sampleBeamProfile() const
    {
      return m_sourcePos;
    }

    /**
     * Selects a random location within the sample + container environment. 
     * @returns Selected position as V3D object
     */
    V3D MonteCarloAbsorption::selectScatterPoint() const
    {
      // Generate 3 random numbers, use them to calculate a random location
      // within the bounding box of the sample environment + sample 
      // and then check if this position is within the whole environment. 
      // If it is return it, if not try again.
      V3D scatterPoint;
      int nattempts(0);
      while( nattempts < MaxRandPointAttempts )
      {
        scatterPoint = V3D(m_bbox_halflength*(2.0*m_randGen->next() - 1.0),
            m_bbox_halfwidth*(2.0*m_randGen->next() - 1.0),
            m_bbox_halfheight*(2.0*m_randGen->next() - 1.0) );
        ++nattempts;
        if( m_sampleShape->isValid(scatterPoint) ||
            (m_container && m_container->isValid(scatterPoint)) )
        {
          scatterPoint += m_samplePos;
          return scatterPoint;
        }
      }
      // If we got here then the shape is too strange for the bounding box to be of any use.
      g_log.error() << "Attempts to generat a random point with the sample/can "
          << "have exceeded the allowed number of tries.\n";
      throw std::runtime_error("Attempts to produce random scatter point failed. Check sample shape.");

    }

    /**
     * Return the attenuation factor for the given track
     * @param startPos The origin of the track
     * @param scatterPoint The point of scatter
     * @param finalPos The end point of the track
     * @param lambda The wavelength of the neutron
     * @returns The attenuation factor for this neutron's track
     */
    double 
    MonteCarloAbsorption::attenuationFactor(const V3D & startPos, const V3D & scatterPoint,
                                            const V3D & finalPos, const double lambda)
    {
      double factor(1.0);

      // Define two tracks, before and after scatter, and trace check their 
      // intersections with the the environment and sample
      Track beforeScatter(scatterPoint, (startPos - scatterPoint));
      Track afterScatter(scatterPoint, (finalPos - scatterPoint));
      // Theoretically this should never happen as there should always be an intersection
      // but do to precision limitations points very close to the surface give
      // zero intersection, so just reject
      if( m_sampleShape->interceptSurface(beforeScatter) == 0 || 
          m_sampleShape->interceptSurface(afterScatter) == 0 )
      {
        return 0.0;
      }

      double length = beforeScatter.begin()->distInsideObject;
      factor *= attenuation(length, *m_sampleMaterial, lambda);

      beforeScatter.clearIntersectionResults();
      if( m_container )
      {
        m_container->interceptSurfaces(beforeScatter);
      }
      // Attenuation factor is product of factor for each material
      Track::LType::const_iterator cend = beforeScatter.end();
      for(Track::LType::const_iterator citr = beforeScatter.begin();
          citr != cend; ++citr)
      {
        length = citr->distInsideObject;
        IObjComponent *objComp = dynamic_cast<IObjComponent*>(citr->componentID);
        Material_const_sptr mat = objComp->material();
        factor *= attenuation(length, *mat, lambda);
      }

      length = afterScatter.begin()->distInsideObject;
      factor *= attenuation(length, *m_sampleMaterial, lambda);
      
      afterScatter.clearIntersectionResults();
      if( m_container )
      {
        m_container->interceptSurfaces(afterScatter);
      }
      // Attenuation factor is product of factor for each material
      cend = afterScatter.end();
      for(Track::LType::const_iterator citr = afterScatter.begin();
          citr != cend; ++citr)
      {
        length = citr->distInsideObject;
        IObjComponent *objComp = dynamic_cast<IObjComponent*>(citr->componentID);
        Material_const_sptr mat = objComp->material();
        factor *= attenuation(length, *mat, lambda);
      }
        
      return factor;
    }

    /**
     * Calculate the attenuation for a given length, material and wavelength
     * @param length Distance through the material
     * @param material A reference to the Material 
     * @param lambda The wavelength
     * @returns The attenuation factor
     */
    double 
    MonteCarloAbsorption::attenuation(const double length, const Geometry::Material& material,
                                      const double lambda) const
    {
      const double rho = material.numberDensity() * 100.0;
      const double sigma_s = material.totalScatterXSection(lambda);
      const double sigma_t = sigma_s + material.absorbXSection(lambda);
      return exp(-rho*sigma_t*length);
    }

    /**
     * Gather the input values and check validity
     * @throws std::invalid_argument If the input is invalid. Currently if there is
     * no defined sample shape
     */
    void MonteCarloAbsorption::retrieveInput()
    {
      m_inputWS = getProperty("InputWorkspace");

      // // Define a test sample material
      // Material *vanadium = new Material("Vanadium", PhysicalConstants::getNeutronAtom(23,0), 0.072);
      // m_inputWS->mutableSample().setMaterial(*vanadium);

      // // Define test environment
      // std::ostringstream xml;
      // xml << "<sphere id=\"" << "sp" <<  "\">"
      // 	  << "<centre x=\"" << 0.0 << "\"  y=\"" << 0.0 << "\" z=\"" << 0.0 << "\" />"
      // 	  << "<radius val=\"" << 0.05 << "\" />"
      // 	  << "</sphere>";

      // Geometry::ShapeFactory shapeMaker;
      // Object_sptr p = shapeMaker.createShape(xml.str());

      // API::SampleEnvironment * kit = new API::SampleEnvironment("TestEnv");
      // kit->add(new ObjComponent("one", p, NULL, boost::shared_ptr<Material>(vanadium)));
      // m_inputWS->mutableSample().setEnvironment(kit);

      m_sampleShape = &(m_inputWS->sample().getShape());
      m_sampleMaterial = &(m_inputWS->sample().getMaterial());
      if( !m_sampleShape->hasValidShape() )
      {
        g_log.debug() << "Invalid shape defined on workspace. TopRule = " << m_sampleShape->topRule()
		                  << ", No. of surfaces: " << m_sampleShape->getSurfacePtr().size() << "\n";
        throw std::invalid_argument("Input workspace has an invalid sample shape.");
      }
      
      if( m_sampleMaterial->totalScatterXSection(1.0) == 0.0 )
      {
        g_log.warning() << "The sample material appears to have zero scattering cross section.\n"
                        << "Result will most likely be nonsensical.\n";
      }
      
      try
      {
        m_container = &(m_inputWS->sample().getEnvironment());
      }
      catch(std::runtime_error&)
      {
        m_container = NULL;
        g_log.information() << "No environment has been defined, continuing with only sample.\n";
      }

      m_numberOfPoints = getProperty("NumberOfWavelengthPoints");
      if( isEmpty(m_numberOfPoints) ||  m_numberOfPoints > m_inputWS->blocksize() )
      {
        m_numberOfPoints = m_inputWS->blocksize();
        if( !isEmpty(m_numberOfPoints) )
        {
          g_log.warning() << "The requested number of wavelength points is larger than the spectra size. "
                          << "Defaulting to spectra size.\n";
        }
      }

      m_numberOfEvents = getProperty("EventsPerPoint");
    }

    /**
     * Initialise the caches used here including setting up the random 
     * number generator
     */
    void MonteCarloAbsorption::initCaches()
    {
      if( !m_randGen )
      {
        m_randGen = new Kernel::MersenneTwister;
        int seedValue = getProperty("SeedValue");
        m_randGen->setSeed(seedValue);
      }
      
      m_samplePos = m_inputWS->getInstrument()->getSample()->getPos();
      m_sourcePos = m_inputWS->getInstrument()->getSource()->getPos();
      BoundingBox box(m_sampleShape->getBoundingBox());
      if( m_container )
      {
        BoundingBox envBox;
        m_container->getBoundingBox(envBox);
        box.grow(envBox);
      }
      //Save the dimensions for quicker calculations later
      m_bbox_length = box.xMax() - box.xMin();
      m_bbox_halflength = 0.5 * m_bbox_length;
      m_bbox_width = box.yMax() - box.yMin();
      m_bbox_halfwidth = 0.5 * m_bbox_width;
      m_bbox_height = box.zMax() - box.zMin();
      m_bbox_halfheight = 0.5 * m_bbox_height;
    }

  }
}
