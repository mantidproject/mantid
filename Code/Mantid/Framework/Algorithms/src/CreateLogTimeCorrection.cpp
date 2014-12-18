#include "MantidAlgorithms/CreateLogTimeCorrection.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceValidators.h"

#include <fstream>
#include <iomanip>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

using namespace std;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CreateLogTimeCorrection)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CreateLogTimeCorrection::CreateLogTimeCorrection() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CreateLogTimeCorrection::~CreateLogTimeCorrection() {}

//----------------------------------------------------------------------------------------------
/** Declare properties
 */
void CreateLogTimeCorrection::init() {
  auto inpwsprop = new WorkspaceProperty<MatrixWorkspace>(
      "InputWorkspace", "", Direction::Input,
      boost::make_shared<InstrumentValidator>());
  declareProperty(inpwsprop,
                  "Name of the input workspace to generate log correct from.");

  auto outwsprop = new WorkspaceProperty<TableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output);
  declareProperty(outwsprop,
                  "Name of the output workspace containing the corrections.");

  auto fileprop =
      new FileProperty("OutputFilename", "", FileProperty::OptionalSave);
  declareProperty(fileprop, "Name of the output time correction file.");

  return;
}

//----------------------------------------------------------------------------------------------
/** Main execution body
  */
void CreateLogTimeCorrection::exec() {
  // 1. Process input
  m_dataWS = getProperty("InputWorkspace");
  m_instrument = m_dataWS->getInstrument();

  //   Check whether the output workspace name is same as input
  string outwsname = getPropertyValue("OutputWorkspace");
  if (outwsname.compare(m_dataWS->name()) == 0) {
    stringstream errmsg;
    errmsg << "It is not allowed to use the same name by both input matrix "
              "workspace and output table workspace.";
    g_log.error(errmsg.str());
    throw runtime_error(errmsg.str());
  }

  // 2. Explore geometry
  getInstrumentSetup();

  // 3. Calculate log time correction
  calculateCorrection();

  // 4. Output
  TableWorkspace_sptr outWS = generateCorrectionTable();
  setProperty("OutputWorkspace", outWS);

  string filename = getProperty("OutputFilename");
  g_log.information() << "Output file name is " << filename << ".\n";
  if (filename.size() > 0) {
    writeCorrectionToFile(filename);
  }
}

//----------------------------------------------------------------------------------------------
/** Get instrument geometry setup including L2 for each detector and L1
  */
void CreateLogTimeCorrection::getInstrumentSetup() {
  // 1. Get sample position and source position
  IComponent_const_sptr sample = m_instrument->getSample();
  if (!sample) {
    throw runtime_error("No sample has been set.");
  }
  V3D samplepos = sample->getPos();

  IComponent_const_sptr source = m_instrument->getSource();
  if (!source) {
    throw runtime_error("No source has been set.");
  }
  V3D sourcepos = source->getPos();
  m_L1 = sourcepos.distance(samplepos);

  // 2. Get detector IDs
  std::vector<detid_t> detids = m_instrument->getDetectorIDs(true);
  for (size_t i = 0; i < detids.size(); ++i) {
    IDetector_const_sptr detector = m_instrument->getDetector(detids[i]);
    V3D detpos = detector->getPos();
    double l2 = detpos.distance(samplepos);
    m_l2map.insert(make_pair(detids[i], l2));
  }

  // 3. Output information
  g_log.information() << "Sample position = " << samplepos << "; "
                      << "Source position = " << sourcepos << ", L1 = " << m_L1
                      << "; "
                      << "Number of detector/pixels = " << detids.size()
                      << ".\n";
}

//----------------------------------------------------------------------------------------------
/** Calculate the log time correction for each pixel, i.e., correcton from event
 * time at detector
  * to time at sample
  */
void CreateLogTimeCorrection::calculateCorrection() {
  map<int, double>::iterator miter;
  for (miter = m_l2map.begin(); miter != m_l2map.end(); ++miter) {
    int detid = miter->first;
    double l2 = miter->second;
    double corrfactor = m_L1 / (m_L1 + l2);
    m_correctionMap.insert(make_pair(detid, corrfactor));
  }
}

//----------------------------------------------------------------------------------------------
/** Write L2 map and correction map to a TableWorkspace
  */
TableWorkspace_sptr CreateLogTimeCorrection::generateCorrectionTable() {
  TableWorkspace_sptr tablews(new TableWorkspace());

  tablews->addColumn("int", "DetectorID");
  tablews->addColumn("double", "Correction");
  tablews->addColumn("double", "L2");

  if (m_l2map.size() != m_correctionMap.size())
    throw runtime_error("Program logic error!");

  map<int, double>::iterator l2iter, citer;
  for (l2iter = m_l2map.begin(); l2iter != m_l2map.end(); ++l2iter) {
    int detid = l2iter->first;
    double l2 = l2iter->second;

    citer = m_correctionMap.find(detid);
    if (citer == m_correctionMap.end()) {
      throw runtime_error("Program logic error (B)!");
    }
    double correction = citer->second;

    TableRow newrow = tablews->appendRow();
    newrow << detid << correction << l2;
  }

  return tablews;
}

//----------------------------------------------------------------------------------------------
/** Write correction map to a text file
  */
void CreateLogTimeCorrection::writeCorrectionToFile(string filename) {
  ofstream ofile;
  ofile.open(filename.c_str());

  if (ofile.is_open()) {
    map<int, double>::iterator miter;
    for (miter = m_correctionMap.begin(); miter != m_correctionMap.end();
         ++miter) {
      int detid = miter->first;
      double corr = miter->second;
      ofile << detid << "\t" << setw(20) << setprecision(5) << corr << "\n";
    }
    ofile.close();
  } else {
    g_log.error() << "Unable to open file " << filename << " to write!\n";
  }

  return;
}

} // namespace Algorithms
} // namespace Mantid
