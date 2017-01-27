#include "MantidAlgorithms/CreateLogTimeCorrection.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"

#include <fstream>

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
/** Declare properties
 */
void CreateLogTimeCorrection::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "Name of the input workspace to generate log correct from.");

  declareProperty(Kernel::make_unique<WorkspaceProperty<TableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace containing the corrections.");

  declareProperty(Kernel::make_unique<FileProperty>("OutputFilename", "",
                                                    FileProperty::OptionalSave),
                  "Name of the output time correction file.");
}

//----------------------------------------------------------------------------------------------
/** Main execution body
  */
void CreateLogTimeCorrection::exec() {
  // 1. Process input
  MatrixWorkspace_sptr dataWS = getProperty("InputWorkspace");

  //   Check whether the output workspace name is same as input
  string outwsname = getPropertyValue("OutputWorkspace");
  if (outwsname.compare(dataWS->getName()) == 0) {
    stringstream errmsg;
    errmsg << "It is not allowed to use the same name by both input matrix "
              "workspace and output table workspace.";
    g_log.error(errmsg.str());
    throw runtime_error(errmsg.str());
  }

  // 2. Explore geometry
  const double l1 = getInstrumentSetup(dataWS->detectorInfo());

  // 3. Calculate log time correction
  calculateCorrection(l1);

  // 4. Output
  TableWorkspace_sptr outWS = generateCorrectionTable();
  setProperty("OutputWorkspace", outWS);

  string filename = getProperty("OutputFilename");
  g_log.information() << "Output file name is " << filename << ".\n";
  if (!filename.empty()) {
    writeCorrectionToFile(filename);
  }
}

//----------------------------------------------------------------------------------------------
/** Get instrument geometry setup including L2 for each detector and L1
  */
double
CreateLogTimeCorrection::getInstrumentSetup(const DetectorInfo &detectorInfo) {

  // 2. Get detector IDs

  std::vector<detid_t> detids = detectorInfo.detectorIDs();
  for (size_t index = 0; index < detectorInfo.size(); ++index) {
    if (!detectorInfo.isMonitor(index)) {
      double l2 = detectorInfo.l2(index);
      m_l2map.emplace(detids[index], l2);
    }
  }

  const double l1 = detectorInfo.l1();
  // 3. Output information
  g_log.information() << "Sample position = " << detectorInfo.samplePosition()
                      << "; "
                      << "Source position = " << detectorInfo.sourcePosition()
                      << ", L1 = " << l1 << "; "
                      << "Number of detector/pixels = " << detectorInfo.size()
                      << ".\n";
  return l1;
}

//----------------------------------------------------------------------------------------------
/** Calculate the log time correction for each pixel, i.e., correcton from event
 * time at detector
  * to time at sample
  */
void CreateLogTimeCorrection::calculateCorrection(const double l1) {
  map<int, double>::iterator miter;
  for (miter = m_l2map.begin(); miter != m_l2map.end(); ++miter) {
    int detid = miter->first;
    double l2 = miter->second;
    double corrfactor = l1 / (l1 + l2);
    m_correctionMap.emplace(detid, corrfactor);
  }
}

//----------------------------------------------------------------------------------------------
/** Write L2 map and correction map to a TableWorkspace
  */
TableWorkspace_sptr CreateLogTimeCorrection::generateCorrectionTable() {
  auto tablews = boost::make_shared<TableWorkspace>();

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
}

} // namespace Algorithms
} // namespace Mantid
