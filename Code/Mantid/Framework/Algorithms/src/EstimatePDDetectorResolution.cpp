/*WIKI*

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/EstimatePDDetectorResolution.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

using namespace std;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(EstimatePDDetectorResolution)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  EstimatePDDetectorResolution::EstimatePDDetectorResolution()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  EstimatePDDetectorResolution::~EstimatePDDetectorResolution()
  {
  }

  //----------------------------------------------------------------------------------------------
  /**
    */
  void EstimatePDDetectorResolution::initDocs()
  {
    setWikiSummary("Estimate the resolution of each detector for a powder diffractometer. ");
    setOptionalMessage("Estimate the resolution of each detector for a powder diffractometer. ");
  }

  //----------------------------------------------------------------------------------------------
  void EstimatePDDetectorResolution::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input),
                    "Name of the workspace to have detector resolution calculated. ");

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
                    "Name of the output workspace containing delta(d)/d of each detector/spectrum. ");
  }

  //----------------------------------------------------------------------------------------------
  /**
    */
  void EstimatePDDetectorResolution::exec()
  {
    processAlgProperties();

    retrieveInstrumentParameters();

    createOutputWorkspace();

    estimateDetectorResolution();

    setProperty("OutputWorkspace", m_outputWS);
  }

  //----------------------------------------------------------------------------------------------
  /**
    */
  void EstimatePDDetectorResolution::processAlgProperties()
  {
    m_inputWS = getProperty("InputWorkspace");

  }

  //----------------------------------------------------------------------------------------------
  /**
    */
  void EstimatePDDetectorResolution::retrieveInstrumentParameters()
  {
    // Call SolidAngle to get solid angles for all detectors
    Algorithm_sptr calsolidangle = createChildAlgorithm("SolidAngle", -1, -1, true);
    calsolidangle->initialize();

    calsolidangle->setProperty("InputWorkspace", m_inputWS);

    calsolidangle->execute();
    if (!calsolidangle->isExecuted())
      throw runtime_error("Unable to run solid angle. ");

    m_solidangleWS = calsolidangle->getProperty("OutputWorkspace");
    if (!m_solidangleWS)
      throw runtime_error("Unable to get solid angle workspace from SolidAngle(). ");


    size_t numspec = m_solidangleWS->getNumberHistograms();
    for (size_t i = 0; i < numspec; ++i)
      g_log.debug() << "[DB]: " << m_solidangleWS->readY(i)[0] << "\n";

    // Calculate centre neutron velocity
    Property* cwlproperty = m_inputWS->run().getProperty("LambdaRequest");
    if (!cwlproperty)
      throw runtime_error("Unable to locate property LambdaRequest as central wavelength. ");
    TimeSeriesProperty<double>* cwltimeseries = dynamic_cast<TimeSeriesProperty<double>* >(cwlproperty);
    if (!cwltimeseries)
      throw runtime_error("LambdaReqeust is not a TimeSeriesProperty in double. ");
    if (cwltimeseries->size() != 1)
      throw runtime_error("LambdaRequest should contain 1 and only 1 entry. ");

    double centrewavelength = cwltimeseries->nthValue(0);
    string unit = cwltimeseries->units();
    if (unit.compare("Angstrom") == 0)
      centrewavelength *= 1.0E-10;
    else
      throw runtime_error("Unit is not recognized");

    m_centreVelocity = PhysicalConstants::h/PhysicalConstants::NeutronMass/centrewavelength;
    g_log.notice() << "Centre wavelength = " << centrewavelength << ", Centre neutron velocity = " << m_centreVelocity << "\n";

    // Calcualte L1 sample to source
    Instrument_const_sptr instrument = m_inputWS->getInstrument();
    V3D samplepos = instrument->getSample()->getPos();
    V3D sourcepos = instrument->getSource()->getPos();
    double m_L1 = samplepos.distance(sourcepos);
    g_log.notice() << "L1 = " << m_L1 << "\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**
    */
  void EstimatePDDetectorResolution::createOutputWorkspace()
  {
    size_t numspec = m_inputWS->getNumberHistograms();

    m_outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", numspec, 1, 1));

    return;
  }
  //----------------------------------------------------------------------------------------------
  /**
    */
  void EstimatePDDetectorResolution::estimateDetectorResolution()
  {
    size_t numspec = m_inputWS->getNumberHistograms();
    for (size_t i = 0; i < numspec; ++i)
    {
      // Get the distance from















    }

    return;
  }

} // namespace Algorithms
} // namespace Mantid
