#include "MantidAlgorithms/CreateLogTimeCorrection.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"


using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

using namespace std;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(CreateLogTimeCorrection)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreateLogTimeCorrection::CreateLogTimeCorrection()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreateLogTimeCorrection::~CreateLogTimeCorrection()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Init documentation
    */
  void CreateLogTimeCorrection::initDocs()
  {
    setWikiSummary("Create log time correction table for event filtering by log value"
                   ", if frequency of log is high.");

    setOptionalMessage("Create log time correction table.  Correction for each pixel is based on L1 and L2.");

    return;
  }
  
  //----------------------------------------------------------------------------------------------
  /** Declare properties
    */
  void CreateLogTimeCorrection::init()
  {
    auto inpwsprop = new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "Anonymous", Direction::Input);
    declareProperty(inpwsprop, "Name of the input workspace to generate log correct from.");

    auto outwsprop = new WorkspaceProperty<Workspace2D>("Outputworkspace", "AnonymousOut", Direction::Output);
    declareProperty(outwsprop, "Name of the output workspace containing the corrections.");

    auto fileprop = new FileProperty("OutputFilename", "", FileProperty::Save);
    declareProperty(fileprop, "Name of the output time correction file.");

    return;
  }


  //----------------------------------------------------------------------------------------------
  void CreateLogTimeCorrection::exec()
  {
    // 1. Process input
    m_dataWS = getProperty("InputWorkspace");

    // 2. Explore geometry
    Instrument_const_sptr m_instrument = m_dataWS->getInstrument();

    std::vector<detid_t> detids = m_instrument->getDetectorIDs(true);

    IObjComponent_const_sptr sample = m_instrument->getSample();
    V3D samplepos = sample->getPos();

    IObjComponent_const_sptr source = m_instrument->getSource();
    V3D sourcepos = source->getPos();
    double l1 = sourcepos.distance(samplepos);

    g_log.notice() << "Sample position = " << samplepos << ".\n";
    g_log.notice() << "Source position = " << sourcepos << ", L1 = " << l1 << ".\n";
    g_log.notice() << "Number of detector/pixels = " << detids.size() << ".\n";

    // 3. Output
    Workspace2D_sptr m_outWS = boost::dynamic_pointer_cast<Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D", 1, detids.size(), detids.size()));

    MantidVec& vecX = m_outWS->dataX(0);
    MantidVec& vecY = m_outWS->dataY(0);
    for (size_t i = 0; i < detids.size(); ++i)
    {
      vecX[i] = static_cast<double>(detids[i]);

      IDetector_const_sptr detector = m_instrument->getDetector(detids[i]);
      V3D detpos = detector->getPos();
      vecY[i] = detpos.distance(samplepos);
    }

    setProperty("OutputWorkspace", m_outWS);

  }





} // namespace Algorithms
} // namespace Mantid
