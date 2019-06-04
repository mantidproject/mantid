// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvertCWSDExpToMomentum.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include "MantidMDAlgorithms/MDWSTransform.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid {
namespace MDAlgorithms {

DECLARE_ALGORITHM(ConvertCWSDExpToMomentum)

/** Constructor
 */
ConvertCWSDExpToMomentum::ConvertCWSDExpToMomentum()
    : m_iColScan(0), m_iColPt(1), m_iColFilename(2), m_iColStartDetID(3),
      m_iMonitorCounts(4), m_iTime(5), m_setQRange(true), m_isBaseName(false),
      m_removeBackground(false) {}

/** Init to declare property
 */
void ConvertCWSDExpToMomentum::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<ITableWorkspace>>("InputWorkspace", "",
                                                      Direction::Input),
      "Name of table workspace for data file names in the experiment.");

  declareProperty(std::make_unique<FileProperty>(
      "InstrumentFilename", "", FileProperty::OptionalLoad, ".xml"));

  declareProperty(
      "DetectorSampleDistanceShift", 0.0,
      "Amount of shift in sample-detector distance from 0.3750 meter.");

  declareProperty(
      "DetectorCenterXShift", 0.0,
      "Amount of shift of detector center in X-direction from (115, 128).");

  declareProperty(
      "DetectorCenterYShift", 0.0,
      "Amount of shift of detector center in Y-direction from (115, 128).");

  declareProperty("UserDefinedWavelength", EMPTY_DBL(),
                  "User defined wave length if it is specified.");

  declareProperty("CreateVirtualInstrument", false,
                  "Flag to create virtual instrument.");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "DetectorTableWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "Name of table workspace containing all the detectors.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of MDEventWorkspace containing all experimental data.");

  declareProperty(std::make_unique<ArrayProperty<double>>("SourcePosition"),
                  "A vector of 3 doubles for position of source.");

  declareProperty(std::make_unique<ArrayProperty<double>>("SamplePosition"),
                  "A vector of 3 doubles for position of sample.");

  declareProperty(std::make_unique<ArrayProperty<double>>("PixelDimension"),
                  "A vector of 8 doubles to determine a cubic pixel's size.");

  declareProperty("IsBaseName", true,
                  "It is specified as true if the data "
                  "file names listed in InputWorkspace are "
                  "base name without directory.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "BackgroundWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Name of optional background workspace.");

  declareProperty(
      std::make_unique<FileProperty>("Directory", "",
                                FileProperty::OptionalDirectory),
      "Directory where data files are if InputWorkspace gives data file name "
      "as the base file name as indicated by 'IsBaseName'.");
}

//----------------------------------------------------------------------------------------------
/** Main exec
 * @brief ConvertCWSDExpToMomentum::exec
 */
void ConvertCWSDExpToMomentum::exec() {
  // Parse inputs
  std::string errmsg;
  bool createvirtual = getProperty("CreateVirtualInstrument");
  bool inputvalid = getInputs(createvirtual, errmsg);
  if (!inputvalid) {
    g_log.error() << "Importing error: " << errmsg << "\n";
    throw std::runtime_error(errmsg);
  }
  m_detSampleDistanceShift = getProperty("DetectorSampleDistanceShift");
  m_detXShift = getProperty("DetectorCenterXShift");
  m_detYShift = getProperty("DetectorCenterYShift");

  // background
  std::string bkgdwsname = getPropertyValue("BackgroundWorkspace");
  if (!bkgdwsname.empty()) {
    m_removeBackground = true;
    m_backgroundWS = getProperty("BackgroundWorkspace");
    // check background
    if (m_backgroundWS->getNumberHistograms() != 256 * 256)
      throw std::invalid_argument("Input background workspace does not have "
                                  "correct number of spectra.");
  } else {
    m_removeBackground = false;
  }

  // Create output MDEventWorkspace
  m_outputWS = createExperimentMDWorkspace();

  if (createvirtual)
    createVirtualInstrument();

  // Add MDEventWorkspace
  addMDEvents(createvirtual);

  setProperty("OutputWorkspace", m_outputWS);

  // Output
  for (size_t i = 0; i < 3; ++i) {
    g_log.notice() << "Q-sample at dimension " << i << ": " << m_minQVec[i]
                   << ", " << m_maxQVec[i] << "\n";
  }
}

//----------------------------------------------------------------------------------------------
/** Create virtual instrument
 * @return
 */
void ConvertCWSDExpToMomentum::createVirtualInstrument() {
  // Get detector list from input table workspace
  std::vector<Kernel::V3D> vec_detpos;
  std::vector<detid_t> vec_detid;
  g_log.information("Start to parse detector parameter tables.");
  parseDetectorTable(vec_detpos, vec_detid);

  // Create a virtual instrument
  g_log.information("Start to create virtual instrument.");
  m_virtualInstrument = Geometry::ComponentHelper::createVirtualInstrument(
      m_sourcePos, m_samplePos, vec_detpos, vec_detid);
  if (!m_virtualInstrument)
    throw std::runtime_error("Failed to create virtual instrument.");
  g_log.information() << "Virtual Instrument has "
                      << m_virtualInstrument->getDetectorIDs().size()
                      << "Detectors\n";
}

//----------------------------------------------------------------------------------------------
/** Create output workspace
 * @brief ConvertCWSDExpToMomentum::createExperimentMDWorkspace
 * @return
 */
API::IMDEventWorkspace_sptr
ConvertCWSDExpToMomentum::createExperimentMDWorkspace() {

  // Create workspace in Q_sample with dimenion as 3
  size_t nDimension = 3;
  IMDEventWorkspace_sptr mdws =
      MDEventFactory::CreateMDWorkspace(nDimension, "MDEvent");

  // Extract Dimensions and add to the output workspace.
  std::vector<std::string> vec_ID(3);
  vec_ID[0] = "Q_sample_x";
  vec_ID[1] = "Q_sample_y";
  vec_ID[2] = "Q_sample_z";

  std::vector<std::string> dimensionNames(3);
  dimensionNames[0] = "Q_sample_x";
  dimensionNames[1] = "Q_sample_y";
  dimensionNames[2] = "Q_sample_z";

  Mantid::Kernel::SpecialCoordinateSystem coordinateSystem =
      Mantid::Kernel::QSample;

  // Add dimensions
  // FIXME - Should I give out a better value???
  if (m_extentMins.size() != 3 || m_extentMaxs.size() != 3 ||
      m_numBins.size() != 3) {
    // Default dimenion
    m_extentMins.resize(3, -10.0);
    m_extentMaxs.resize(3, 10.0);
    m_numBins.resize(3, 100);
  }
  // Sample-Q range
  m_minQVec.resize(3);
  m_maxQVec.resize(3);

  for (size_t d = 0; d < 3; ++d)
    g_log.debug() << "Direction " << d << ", Range = " << m_extentMins[d]
                  << ", " << m_extentMaxs[d] << "\n";

  // Set Q Sample frame
  Mantid::Geometry::QSample frame;

  for (size_t i = 0; i < nDimension; ++i) {
    std::string id = vec_ID[i];
    std::string name = dimensionNames[i];
    mdws->addDimension(
        Geometry::MDHistoDimension_sptr(new Geometry::MDHistoDimension(
            id, name, frame, static_cast<coord_t>(m_extentMins[i]),
            static_cast<coord_t>(m_extentMaxs[i]), m_numBins[i])));
  }

  // Set coordinate system
  mdws->setCoordinateSystem(coordinateSystem);

  return mdws;
}

//----------------------------------------------------------------------------------------------
/** Add MDEvents to MDEventWorkspace from data set in the experiment
 *  Run number is determined by the row of the file in the input table workspace
 * @brief ConvertCWSDExpToMomentum::addMDEvents
 * @param usevirtual :: flag to use virtual instrument
 */
void ConvertCWSDExpToMomentum::addMDEvents(bool usevirtual) {
  MatrixWorkspace_sptr spicews;

  // Check whether to add / or \ to m_dataDir
  std::string sep;
  if (!m_dataDir.empty()) {
#if defined _WIN32 || defined _WIN64
    if (*m_dataDir.rbegin() != '\\') {
      sep = "\\";
    }
#else
    if (*m_dataDir.rbegin() != '/') {
      sep = "/";
    }
#endif
  }

  // Init some variables
  size_t numrows = m_expDataTableWS->rowCount();
  if (numrows > 1 && !usevirtual) {
    g_log.warning("There are more than 1 experiment to import. "
                  "Make sure that all of them have the same instrument.");
  }

  // Loop through all data files in the experiment
  for (size_t ir = 0; ir < numrows; ++ir) {
    std::string rawfilename =
        m_expDataTableWS->cell<std::string>(ir, m_iColFilename);
    detid_t start_detid = 0;
    if (usevirtual)
      start_detid = m_expDataTableWS->cell<detid_t>(ir, m_iColStartDetID);

    // Load data
    bool loaded;
    std::string errmsg;

    std::stringstream filess;
    if (m_isBaseName)
      filess << m_dataDir << sep;
    filess << rawfilename;
    std::string filename(filess.str());

    spicews = loadSpiceData(filename, loaded, errmsg);
    if (!loaded) {
      g_log.error(errmsg);
      continue;
    }
    if (m_removeBackground)
      removeBackground(spicews);

    // Convert from MatrixWorkspace to MDEvents and add events to
    int scanid = m_expDataTableWS->cell<int>(ir, m_iColScan);
    g_log.notice() << "[DB] Scan = " << scanid << "\n";
    int runid = m_expDataTableWS->cell<int>(ir, m_iColPt);
    g_log.notice() << "Pt = " << runid << "\n"
                   << m_iTime << "-th for time/duration"
                   << "\n";
    double time(0.);
    try {
      float time_f = m_expDataTableWS->cell<float>(ir, m_iTime);
      time = static_cast<double>(time_f);
    } catch (const std::runtime_error &) {
      time = m_expDataTableWS->cell<double>(ir, m_iTime);
    }

    int monitor_counts = m_expDataTableWS->cell<int>(ir, m_iMonitorCounts);
    if (!usevirtual)
      start_detid = 0;
    convertSpiceMatrixToMomentumMDEvents(spicews, usevirtual, start_detid,
                                         scanid, runid, time, monitor_counts);
  }

  // Set box extents
  std::vector<API::IMDNode *> boxes;

  // Set extents for all MDBoxes
  progress(0.90, "Set up MDBoxes' dimensions. ");
  m_outputWS->getBoxes(boxes, 1000, true);
  auto it1 = boxes.begin();
  auto it1_end = boxes.end();
  for (; it1 != it1_end; it1++) {
    auto box = *it1;
    for (size_t dim = 0; dim < 3; ++dim) {
      MDBox<MDEvent<3>, 3> *mdbox =
          dynamic_cast<DataObjects::MDBox<MDEvent<3>, 3> *>(box);
      if (!mdbox)
        throw std::runtime_error("Unable to cast to MDBox");
      mdbox->setExtents(dim, -10, 10);
      mdbox->calcVolume();
      mdbox->refreshCache(nullptr);
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Set goniometer to matrix workspace and get its rotation matrix R (from
 * Q-sample to Q-lab
 * and output 1/R
 * @brief ConvertCWSDExpToMomentum::setupTransferMatrix
 * @param dataws :: matrix workspace containing sample rotation angles
 * @param rotationMatrix :: output as matrix 1/R to convert from Q-lab to
 * Q-sample
 */
void ConvertCWSDExpToMomentum::setupTransferMatrix(
    API::MatrixWorkspace_sptr dataws, Kernel::DblMatrix &rotationMatrix) {
  // Check sample logs
  if (!dataws->run().hasProperty("_omega") ||
      !dataws->run().hasProperty("_chi") || !dataws->run().hasProperty("_phi"))
    throw std::runtime_error(
        "Data workspace does not have sample log _phi, _chi or _omega. "
        "Unable to set goniometer and calcualte roation matrix R.");

  // Call algorithm SetGoniometer
  IAlgorithm_sptr setalg = createChildAlgorithm("SetGoniometer");
  setalg->initialize();
  setalg->setProperty("Workspace", dataws);
  setalg->setProperty("Axis0", "_omega,0,1,0,-1");
  setalg->setProperty("Axis1", "_chi,0,0,1,-1");
  setalg->setProperty("Axis2", "_phi,0,1,0,-1");
  setalg->execute();

  if (setalg->isExecuted()) {
    rotationMatrix = dataws->run().getGoniometer().getR();
    g_log.debug() << "Ratation matrix: " << rotationMatrix.str() << "\n";
    rotationMatrix.Invert();
    g_log.debug() << "Ratation matrix: " << rotationMatrix.str() << "\n";
  } else
    throw std::runtime_error("Unable to set Goniometer.");
}

//----------------------------------------------------------------------------------------------
/** Convert a SPICE 2D Det MatrixWorkspace to MDEvents and append to an
 * MDEventWorkspace
 * It is optional to use a virtual instrument or copy from input data workspace
 * @brief ConvertCWSDExpToMomentum::convertSpiceMatrixToMomentumMDEvents
 * @param dataws :: data matrix workspace
 * @param usevirtual :: boolean flag to use virtual instrument
 * @param startdetid :: starting detid for detectors from this workspace mapping
 * to virtual instrument in MDEventWorkspace
 * @param scannumber :: scan number
 * @param runnumber :: run number for all MDEvents created from this matrix
 * @param measuretime :: duration (time) to measure this point
 * @param monitor_counts :: monitor counts; add to ExpInfo
 * workspace
 */
void ConvertCWSDExpToMomentum::convertSpiceMatrixToMomentumMDEvents(
    MatrixWorkspace_sptr dataws, bool usevirtual, const detid_t &startdetid,
    const int scannumber, const int runnumber, double measuretime,
    int monitor_counts) {
  // Create transformation matrix from which the transformation is
  Kernel::DblMatrix rotationMatrix;
  setupTransferMatrix(dataws, rotationMatrix);

  g_log.information() << "Before insert new event, output workspace has "
                      << m_outputWS->getNEvents() << "Events.\n";

  // Creates a new instance of the MDEventInserte to output workspace
  MDEventWorkspace<MDEvent<3>, 3>::sptr mdws_mdevt_3 =
      boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3>>(m_outputWS);
  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> inserter(mdws_mdevt_3);

  // Calcualte k_i: it is assumed that all k_i are same for one Pt.
  // number, i.e., one 2D XML file
  Kernel::V3D sourcePos = dataws->getInstrument()->getSource()->getPos();
  Kernel::V3D samplePos = dataws->getInstrument()->getSample()->getPos();
  if (dataws->x(0).size() != 2)
    throw std::runtime_error(
        "Input matrix workspace has wrong dimension in X-axis.");
  double momentum = 0.5 * (dataws->x(0)[0] + dataws->x(0)[1]);
  Kernel::V3D ki = (samplePos - sourcePos) * (momentum / sourcePos.norm());

  g_log.debug() << "Source at " << sourcePos.toString()
                << ", Norm = " << sourcePos.norm()
                << ", momentum = " << momentum << "\n"
                << "k_i = " << ki.toString() << "\n";

  // Go though each spectrum to conver to MDEvent
  size_t numspec = dataws->getNumberHistograms();
  const auto &specInfo = dataws->spectrumInfo();
  double maxsignal = 0;
  size_t nummdevents = 0;
  for (size_t iws = 0; iws < numspec; ++iws) {
    // Get detector positions and signal
    double signal = dataws->y(iws)[0];
    // Skip event with 0 signal
    if (fabs(signal) < 0.001)
      continue;
    double error = sqrt(fabs(signal));
    Kernel::V3D detpos = specInfo.position(iws);
    std::vector<Mantid::coord_t> q_sample(3);

    // Calculate Q-sample and new detector ID in virtual instrument.
    Kernel::V3D qlab = convertToQSample(samplePos, ki, detpos, momentum,
                                        q_sample, rotationMatrix);
    detid_t native_detid = specInfo.detector(iws).getID();
    detid_t detid = native_detid + startdetid;

    // Insert
    inserter.insertMDEvent(
        static_cast<float>(signal), static_cast<float>(error * error),
        static_cast<uint16_t>(runnumber), detid, q_sample.data());
    updateQRange(q_sample);

    g_log.debug() << "Q-lab = " << qlab.toString() << "\n";
    g_log.debug() << "Insert DetID " << detid << ", signal = " << signal
                  << ", with q_sample = " << q_sample[0] << ", " << q_sample[1]
                  << ", " << q_sample[2] << "\n";

    // Update some statistical inforamtion
    if (signal > maxsignal)
      maxsignal = signal;
    ++nummdevents;
  }

  g_log.information() << "Imported Matrixworkspace of run number " << runnumber
                      << ": Max. Signal = " << maxsignal << ", Add "
                      << nummdevents << " MDEvents "
                      << "\n";

  // Add experiment info including instrument, goniometer and run number
  ExperimentInfo_sptr expinfo = boost::make_shared<ExperimentInfo>();
  if (usevirtual)
    expinfo->setInstrument(m_virtualInstrument);
  else {
    Geometry::Instrument_const_sptr tmp_inst = dataws->getInstrument();
    expinfo->setInstrument(tmp_inst);
  }
  expinfo->mutableRun().setGoniometer(dataws->run().getGoniometer(), false);
  int scan_run_number = scannumber * 1000 + runnumber;
  expinfo->mutableRun().addProperty("run_number", scan_run_number);
  expinfo->mutableRun().addProperty("duration", measuretime);
  expinfo->mutableRun().addProperty("monitor", monitor_counts);
  // Add all the other propertys from original data workspace
  const std::vector<Kernel::Property *> vec_property =
      dataws->run().getProperties();
  for (auto property : vec_property) {
    expinfo->mutableRun().addProperty(property->clone());
  }

  m_outputWS->addExperimentInfo(expinfo);
}

//----------------------------------------------------------------------------------------------
/** Examine input
 * @brief ConvertCWSDExpToMomentum::getInputs
 * @param virtualinstrument :: boolean flag to use virtual instrument
 * @param errmsg
 * @return
 */
bool ConvertCWSDExpToMomentum::getInputs(bool virtualinstrument,
                                         std::string &errmsg) {
  std::stringstream errss;

  // Table workspace for data file names and starting detector IDs (for virtual
  // instrument)
  m_expDataTableWS = getProperty("InputWorkspace");
  const std::vector<std::string> datacolnames =
      m_expDataTableWS->getColumnNames();
  if (datacolnames.size() != 6) {
    errss << "InputWorkspace must have 6 columns.  But now it has "
          << datacolnames.size() << " columns. \n";
  } else {
    if (datacolnames[m_iColFilename] != "File Name" &&
        datacolnames[m_iColFilename] != "Filename")
      errss << "Data file name Table (InputWorkspace)'s Column "
            << m_iColFilename << " must be 'File Name' or 'Filename' but not "
            << datacolnames[m_iColFilename] << ". "
            << "\n";
    if (datacolnames[m_iColStartDetID] != "Starting DetID" &&
        datacolnames[m_iColStartDetID] != "StartDetID")
      errss << "Data file name Table (InputWorkspace)'s Column "
            << m_iColStartDetID
            << " must be 'Staring DetID' or 'StartDetID' but not "
            << datacolnames[m_iColStartDetID] << ". "
            << "\n";
  }
  g_log.warning("Finished parsing Data Table");

  // FIXME/TODO: Add the code to read monitor counts from input table workspace

  // Set up parameters for creating virtual instrument
  g_log.warning() << "About to deal with virtual instrument"
                  << virtualinstrument << "\n";
  if (virtualinstrument) {
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
      errss << "SourcePosition must have 3 items.  Input has "
            << sourcepos.size() << " instead.\n";
    else {
      m_sourcePos.setX(sourcepos[0]);
      m_sourcePos.setY(sourcepos[1]);
      m_sourcePos.setZ(sourcepos[2]);
    }

    std::vector<double> samplepos = getProperty("SamplePosition");
    if (samplepos.size() != 3) {
      errss << "SamplePosition must have 3 items.  Input has "
            << samplepos.size() << " instead.\n";
    } else {
      m_samplePos.setX(samplepos[0]);
      m_samplePos.setY(samplepos[1]);
      m_samplePos.setZ(samplepos[2]);
    }
  }

  m_isBaseName = getProperty("IsBaseName");
  if (m_isBaseName)
    m_dataDir = getPropertyValue("Directory");

  errmsg = errss.str();

  return (errmsg.empty());
}

//----------------------------------------------------------------------------------------------
/** Convert to Q-sample from detector position and momentum
 * @brief ConvertCWSDExpToMomentum::convertToMomentum
 * @param samplePos :: sample position for Kf
 * @param ki :: Ki
 * @param detPos :: detector position
 * @param momentum :: q = 2pi/lambda
 * @param qSample :: output Q-sample
 * @param rotationMatrix :: Invert R matrix
 * @return
 */
Kernel::V3D ConvertCWSDExpToMomentum::convertToQSample(
    const Kernel::V3D &samplePos, const Kernel::V3D &ki,
    const Kernel::V3D &detPos, const double &momentum,
    std::vector<coord_t> &qSample, const Kernel::DblMatrix &rotationMatrix) {

  // Use detector position and wavelength/Q to calcualte Q_lab
  Kernel::V3D kf;
  kf = (detPos - samplePos) * (momentum / (detPos - samplePos).norm());
  Kernel::V3D q_lab = ki - kf;

  // Calculate q_sample from qlab and R matrix
  Kernel::V3D q_sample = rotationMatrix * q_lab;
  qSample.resize(3);
  qSample[0] = static_cast<float>(q_sample.X());
  qSample[1] = static_cast<float>(q_sample.Y());
  qSample[2] = static_cast<float>(q_sample.Z());

  return q_lab;
}

//----------------------------------------------------------------------------------------------
/** Load SPICE data to Matrix workspace
 * @brief ConvertCWSDExpToMomentum::loadSpiceData
 * @param filename
 * @param loaded
 * @param errmsg
 * @return
 */
API::MatrixWorkspace_sptr
ConvertCWSDExpToMomentum::loadSpiceData(const std::string &filename,
                                        bool &loaded, std::string &errmsg) {
  // Init output
  API::MatrixWorkspace_sptr dataws;
  errmsg = "";

  // Load SPICE file
  try {
    IAlgorithm_sptr loader = createChildAlgorithm("LoadSpiceXML2DDet");
    loader->initialize();
    loader->setProperty("Filename", filename);
    // std::vector<size_t> sizelist(2);
    // sizelist[0] = 256;
    // sizelist[1] = 256;
    // loader->setProperty("DetectorGeometry", sizelist);
    loader->setProperty("LoadInstrument", true);
    loader->setProperty("ShiftedDetectorDistance", m_detSampleDistanceShift);
    loader->setProperty("DetectorCenterXShift", m_detXShift);
    loader->setProperty("DetectorCenterYShift", m_detYShift);

    // TODO/FIXME - This is not a nice solution for detector geometry
    // std::string idffile = getPropertyValue("InstrumentFilename");
    // if (idffile.size() > 0) {
    //   loader->setProperty("InstrumentFilename", idffile);
    //   loader->setProperty("DetectorGeometry", "512, 512");
    // }

    double wavelength = getProperty("UserDefinedWavelength");

    if (wavelength != EMPTY_DBL())
      loader->setProperty("UserSpecifiedWaveLength", wavelength);

    loader->execute();

    dataws = loader->getProperty("OutputWorkspace");
    loaded = static_cast<bool>(dataws);
  } catch (std::runtime_error &runerror) {
    loaded = false;
    errmsg = runerror.what();
  }

  return dataws;
}

//----------------------------------------------------------------------------------------------
/** Parase detetor table workspace to vector of detector positions
 * @brief ConvertCWSDExpToMomentum::parseDetectorTable
 * @param vec_detpos
 * @param vec_detid
 */
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
}

//----------------------------------------------------------------------------------------------
/** Update (sample) Q range
 * @brief ConvertCWSDExpToMomentum::updateQRange
 * @param vec_q
 */
void ConvertCWSDExpToMomentum::updateQRange(
    const std::vector<Mantid::coord_t> &vec_q) {
  for (size_t i = 0; i < vec_q.size(); ++i) {
    if (m_setQRange) {
      m_minQVec[i] = vec_q[i];
      m_maxQVec[i] = vec_q[i];
      m_setQRange = false;
    } else if (vec_q[i] < m_minQVec[i])
      m_minQVec[i] = vec_q[i];
    else if (vec_q[i] > m_maxQVec[i])
      m_maxQVec[i] = vec_q[i];
  }
}

/** Remove background per pixel
 * @brief ConvertCWSDExpToMomentum::removeBackground
 * @param dataws
 */
void ConvertCWSDExpToMomentum::removeBackground(
    API::MatrixWorkspace_sptr dataws) {
  if (dataws->getNumberHistograms() != m_backgroundWS->getNumberHistograms())
    throw std::runtime_error("Impossible to have this situation");

  size_t numhist = dataws->getNumberHistograms();
  for (size_t i = 0; i < numhist; ++i) {
    double bkgd_y = m_backgroundWS->y(i)[0];
    if (fabs(bkgd_y) > 1.E-2) {
      dataws->mutableY(i)[0] -= bkgd_y;
      dataws->mutableE(i)[0] = std::sqrt(dataws->y(i)[0]);
    }
  }
}

} // namespace MDAlgorithms
} // namespace Mantid
