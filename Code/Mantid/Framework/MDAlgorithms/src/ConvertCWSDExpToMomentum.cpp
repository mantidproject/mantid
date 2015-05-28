#include "MantidMDAlgorithms/ConvertCWSDExpToMomentum.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid {
namespace MDAlgorithms {

  DECLARE_ALGORITHM(ConvertCWSDExpToMomentum)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertCWSDExpToMomentum::ConvertCWSDExpToMomentum()
    : m_iColFilename(2), m_iColStartDetID(3) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertCWSDExpToMomentum::~ConvertCWSDExpToMomentum() {}

/** Init to declare property
 */
void ConvertCWSDExpToMomentum::init() {
  declareProperty(
      new WorkspaceProperty<ITableWorkspace>("InputWorkspace", "",
                                             Direction::Input),
      "Name of table workspace for data file names in the experiment.");

  declareProperty(new WorkspaceProperty<ITableWorkspace>(
                      "DetectorTableWorkspace", "", Direction::Input),
                  "Name of table workspace containing all the detectors.");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                    "OutputWorkspace", "", Direction::Output),
                  "Name of MDEventWorkspace containing all experimental data.");

  declareProperty(new ArrayProperty<double>("SourcePosition"),
                  "A vector of 3 doubles for position of source.");

  declareProperty(new ArrayProperty<double>("SamplePosition"),
                  "A vector of 3 doubles for position of sample.");

  declareProperty(new ArrayProperty<double>("PixelDimension"),
                  "A vector of 8 doubles to determine a cubic pixel's size.");

}

/**
 * @brief ConvertCWSDExpToMomentum::exec
 */
void ConvertCWSDExpToMomentum::exec() {
  // Parse inputs
  std::string errmsg("");
  bool inputvalid = getInputs(errmsg);
  if (!inputvalid)
    throw std::runtime_error(errmsg);

  g_log.notice("[DB] Inputs are examined.");

  // Create output MDEventWorkspace
  m_outputWS = createExperimentMDWorkspace();

  // Add MDEventWorkspace
  addMDEvents();

  setProperty("OutputWorkspace", m_outputWS);

  return;
}

API::IMDEventWorkspace_sptr
ConvertCWSDExpToMomentum::createExperimentMDWorkspace() {
  // Get detector list from input table workspace
  std::vector<Kernel::V3D> vec_detpos;
  std::vector<detid_t> vec_detid;
  parseDetectorTable(vec_detpos, vec_detid);

  // FIXME - Should set create a solid instrument
   m_virtualInstrument = Geometry::ComponentHelper::createVirtualInstrument(m_sourcePos, m_samplePos,
                                                                               vec_detpos,
                                                                               vec_detid);
   if (!m_virtualInstrument)
     throw std::runtime_error("Failed to create virtual instrument.");
   g_log.notice() << "[DB] Virtual Instrument has " <<  m_virtualInstrument->getDetectorIDs().size()
                 << "Detectors\n";


  // Create workspace
  // FIXME - Should I give out a better value???
  m_extentMins.resize(3, -10.0);
  m_extentMaxs.resize(3, 10.0);
  m_numBins.resize(3, 100);

  size_t m_nDimensions = 3;
  IMDEventWorkspace_sptr mdws =
      MDEventFactory::CreateMDWorkspace(m_nDimensions, "MDEvent");

  // Extract Dimensions and add to the output workspace.

  // FIXME - Is this univeral including momentrum?
  std::vector<std::string> vec_ID(3);
  vec_ID[0] = "x";
  vec_ID[1] = "y";
  vec_ID[2] = "z";

  std::vector<std::string> dimensionNames(3);
  dimensionNames[0] = "Q_sample_x";
  dimensionNames[1] = "Q_sample_y";
  dimensionNames[2] = "Q_sample_z";
  Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = Mantid::Kernel::QSample;

  // Add dimensions
  if (m_extentMins.size() != 3 || m_extentMaxs.size() != 3 || m_numBins.size() != 3)
    throw std::runtime_error("The range and number of bins are not set up correctly to create MDEventWorkspace.");
  else
    for (size_t d = 0; d < 3; ++d)
      g_log.debug() << "Direction " << d << ", Range = " << m_extentMins[d]
                       << ", " << m_extentMaxs[d] << "\n";

  for (size_t i = 0; i < m_nDimensions; ++i) {
    std::string id = vec_ID[i];
    std::string name = dimensionNames[i];
    // FIXME - Unit of momentum?
    std::string units = "???";
    // int nbins = 100;

    mdws->addDimension(
        Geometry::MDHistoDimension_sptr(new Geometry::MDHistoDimension(
            id, name, units, static_cast<coord_t>(m_extentMins[i]),
            static_cast<coord_t>(m_extentMaxs[i]), m_numBins[i])));
  }

  // Add events
  // Creates a new instance of the MDEventInserter.
  MDEventWorkspace<MDEvent<3>, 3>::sptr MDEW_MDEVENT_3 =
      boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3> >(mdws);

  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> inserter(
      MDEW_MDEVENT_3);

  // FIXME - Add instrument?

  mdws->setCoordinateSystem(coordinateSystem);

  return mdws;
}

void ConvertCWSDExpToMomentum::addMDEvents() {
  MatrixWorkspace_sptr spicews;

  size_t numrows = m_expDataTableWS->rowCount();
  size_t numFileNotLoaded(0);
  for (size_t ir = 0; ir < numrows; ++ir) {
    std::string filename =
        m_expDataTableWS->cell<std::string>(ir, m_iColFilename);
    detid_t start_detid = m_expDataTableWS->cell<detid_t>(ir, m_iColStartDetID);

    // Load data
    bool loaded;
    std::string errmsg;
    spicews = loadSpiceData(filename, loaded, errmsg);
    if (!loaded) {
      g_log.error(errmsg);
      ++numFileNotLoaded;
      continue;
    }

    // Convert from MatrixWorkspace to MDEvents and add events to
    int runid = static_cast<int>(ir) + 1;
    convertSpiceMatrixToMomentumMDEvents(spicews, start_detid, runid);
  }

  return;
}

void ConvertCWSDExpToMomentum::convertSpiceMatrixToMomentumMDEvents(
    MatrixWorkspace_sptr dataws, const detid_t &startdetid,
    const int runnumber) {

  // Creates a new instance of the MDEventInserter.
  MDEventWorkspace<MDEvent<3>, 3>::sptr mdws_mdevt_3 =
      boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3> >(m_outputWS);
  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> inserter(
        mdws_mdevt_3);

  size_t numspec = dataws->getNumberHistograms();
  for (size_t iws = 0; iws < numspec; ++iws) {
    // Get detector positions and signal
    double signal = dataws->readY(iws)[0];
    double error = dataws->readE(iws)[0];
    double wavelength = dataws->readX(iws)[0];
    // Create the MDEvent
    Kernel::V3D detpos = dataws->getDetector(iws)->getPos();
    std::vector<Mantid::coord_t> momentum(3);
    convertToMomentum(detpos, wavelength, momentum);
    detid_t detid = dataws->getDetector(iws)->getID() + startdetid;
    // Insert
    inserter.insertMDEvent(static_cast<float>(signal), static_cast<float>(error*error),
                           static_cast<uint16_t>(runnumber), detid,
                           momentum.data());
    // m_outputWS->addExperimentInfo();
  }

  ExperimentInfo_sptr expinfo = boost::make_shared<ExperimentInfo>();
  expinfo->setInstrument(m_virtualInstrument);
  m_outputWS->addExperimentInfo(expinfo);

}

/// Examine input
bool ConvertCWSDExpToMomentum::getInputs(std::string &errmsg) {
  std::stringstream errss;

  // Table workspace for data file names and starting detector IDs (for virtual
  // instrument)
  m_expDataTableWS = getProperty("InputWorkspace");
  const std::vector<std::string> datacolnames =
      m_expDataTableWS->getColumnNames();
  if (datacolnames.size() != 4) {
    errss << "InputWorkspace must have 4 columns.  But now it has "
          << datacolnames.size() << " columns. \n";
  } else {
    if (datacolnames[m_iColFilename].compare("File Name") != 0)
      errss << "Data file name Table (InputWorkspace)'s Column "
            << m_iColFilename << " must be 'File Name'"
            << "\n";
    if (datacolnames[m_iColStartDetID].compare("Starting DetID") != 0)
      errss << "Data file name Table (InputWorkspace)'s Column "
            << m_iColStartDetID << " must be 'Staring DetID'"
            << "\n";
  }

  // Table workspace for detector positions
  m_detectorListTableWS = getProperty("DetectorTableWorkspace");
  const std::vector<std::string> detcolnames =
      m_detectorListTableWS->getColumnNames();
  if (detcolnames.size() != 5) {
    errss << "Detector table (DetectorTableWorkspace) must have 5 columns"
          << "\n";
  }

  // Sample and source position
  std::vector<double> sourcepos = getProperty("SourcePosition");
  if (sourcepos.size() != 3)
    errss << "SourcePosition must have 3 items.  Input has " << sourcepos.size()
          << " instead.\n";
  else {
    m_sourcePos.setX(sourcepos[0]);
    m_sourcePos.setY(sourcepos[1]);
    m_sourcePos.setZ(sourcepos[2]);
  }

  std::vector<double> samplepos = getProperty("SamplePosition");
  if (samplepos.size() != 3) {
    errss << "SamplePosition must have 3 items.  Input has " << samplepos.size()
          << " instead.\n";
  } else {
    m_samplePos.setX(samplepos[0]);
    m_samplePos.setY(samplepos[1]);
    m_samplePos.setZ(samplepos[2]);
  }

  errmsg = errss.str();
  return (errmsg.size() > 0);
}

void ConvertCWSDExpToMomentum::convertToMomentum(const std::vector<double> &detPos, const double &wavelength, std::vector<coord_t> &qSample)
{
  // TODO/FIXME : Implement ASAP!

  return;
}

API::MatrixWorkspace_sptr ConvertCWSDExpToMomentum::loadSpiceData(const std::string &filename, bool &loaded, std::string &errmsg)
{
  IAlgorithm_sptr loader = createChildAlgorithm("LoadSpiceXML2DDet");
  loader->initialize();
  loader->setProperty("Filename", filename);
  std::vector<size_t> sizelist(2);
  sizelist[0] = 256;
  sizelist[1] = 256;
  loader->setProperty("DetectorGeometry", sizelist);
  loader->setProperty("LoadInstrument", true);

  loader->execute();

  API::MatrixWorkspace_sptr dataws = loader->getProperty("OutputWorkspace");
  if (dataws)
    loaded = true;
  else
    loaded = false;

  return dataws;
}


void ConvertCWSDExpToMomentum::parseDetectorTable(
    std::vector<Kernel::V3D> &vec_detpos, std::vector<detid_t> &vec_detid) {
  // Set vectors' sizes
  size_t numrows = m_detectorListTableWS->rowCount();
  vec_detpos.resize(numrows);
  vec_detid.resize(numrows);

  // Parse table
  for (size_t i = 0; i < numrows; ++i) {
    detid_t detid = m_detectorListTableWS->cell<int>(i, 0);
    vec_detid[i] = detid;

    double x = m_detectorListTableWS->cell<double>(i, 1);
    double y = m_detectorListTableWS->cell<double>(i, 2);
    double z = m_detectorListTableWS->cell<double>(i, 3);
    Kernel::V3D detpos(x, y, z);
    vec_detpos[i] = detpos;
  }

  return;
}

} // namespace MDAlgorithms
} // namespace Mantid
