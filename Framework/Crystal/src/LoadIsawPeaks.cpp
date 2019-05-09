// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCrystal/CalibrationHelpers.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Unit.h"
#include <boost/algorithm/string/trim.hpp>

using Mantid::Kernel::Strings::getWord;
using Mantid::Kernel::Strings::readToEndOfLine;
using Mantid::Kernel::Units::Wavelength;

namespace Mantid {
namespace Crystal {

DECLARE_FILELOADER_ALGORITHM(LoadIsawPeaks)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

/**
 * Determine the confidence with which this algorithm can load a given file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadIsawPeaks::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &extn = descriptor.extension();
  // If the extension is peaks or integrate then give it a go
  if (extn != ".peaks" && extn != ".integrate")
    return 0;

  int confidence(0);
  try {
    auto &in = descriptor.data();
    // Read the header, load the instrument
    std::string tag;
    std::string r = getWord(in, false);

    if (r.length() < 1)
      throw std::logic_error(std::string("No first line of Peaks file"));

    if (r != "Version:")
      throw std::logic_error(
          std::string("No Version: on first line of Peaks file"));

    std::string C_version = getWord(in, false);
    if (C_version.length() < 1)
      throw std::logic_error(std::string("No Version for Peaks file"));

    getWord(in, false); // tag
    // cppcheck-suppress unreadVariable
    std::string C_Facility = getWord(in, false);

    getWord(in, false); // tag
    std::string C_Instrument = getWord(in, false);

    if (C_Instrument.length() < 1)
      throw std::logic_error(std::string("No Instrument for Peaks file"));

    // Date: use the current date/time if not found
    Types::Core::DateAndTime C_experimentDate;
    tag = getWord(in, false);
    if (tag == "Date:")
      getWord(in, false);
    readToEndOfLine(in, true);
    confidence = 95;
  } catch (std::exception &) {
  }

  return confidence;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadIsawPeaks::init() {
  const std::vector<std::string> exts{".peaks", ".integrate"};
  declareProperty(Kernel::make_unique<FileProperty>("Filename", "",
                                                    FileProperty::Load, exts),
                  "Path to an ISAW-style .peaks filename.");
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadIsawPeaks::exec() {
  // Create the workspace
  PeaksWorkspace_sptr ws(new PeaksWorkspace());
  std::string outputwsName = getPropertyValue("OutputWorkspace");
  AnalysisDataService::Instance().addOrReplace(outputwsName, ws);

  // This loads (appends) the peaks
  this->appendFile(ws, getPropertyValue("Filename"));

  // Save it in the output
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(ws));

  this->checkNumberPeaks(ws, getPropertyValue("Filename"));
}

//-----------------------------------------------------------------------------------------------
/** Reads the header of a .peaks file
 * @param outWS :: the workspace in which to place the information
 * @param in :: stream of the input file
 * @param T0 :: Time offset
 * @return the first word on the next line
 */
std::string LoadIsawPeaks::readHeader(PeaksWorkspace_sptr outWS,
                                      std::ifstream &in, double &T0) {
  std::string tag;
  std::string r = getWord(in, false);

  if (r.length() < 1)
    throw std::logic_error(std::string("No first line of Peaks file"));

  if (r != "Version:")
    throw std::logic_error(
        std::string("No Version: on first line of Peaks file"));

  std::string C_version = getWord(in, false);
  if (C_version.length() < 1)
    throw std::logic_error(std::string("No Version for Peaks file"));

  getWord(in, false); // tag
  // cppcheck-suppress unreadVariable
  std::string C_Facility = getWord(in, false);

  getWord(in, false); // tag
  std::string C_Instrument = getWord(in, false);

  if (C_Instrument.length() < 1)
    throw std::logic_error(std::string("No Instrument for Peaks file"));

  // Date: use the current date/time if not found
  Types::Core::DateAndTime C_experimentDate;
  std::string date;
  tag = getWord(in, false);
  if (tag.empty())
    date = Types::Core::DateAndTime::getCurrentTime().toISO8601String();
  else if (tag == "Date:")
    date = getWord(in, false);
  tag = getWord(in, false);
  m_isModulatedStructure = tag == "MOD";
  readToEndOfLine(in, true);

  // Now we load the instrument using the name and date
  MatrixWorkspace_sptr tempWS =
      WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
  tempWS->mutableRun().addProperty<std::string>("run_start", date);

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("InstrumentName", C_Instrument);
  loadInst->setProperty("RewriteSpectraMap",
                        Mantid::Kernel::OptionalBool(true));
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
  loadInst->executeAsChildAlg();

  // Populate the instrument parameters in this workspace - this works around a
  // bug
  tempWS->populateInstrumentParameters();
  Geometry::Instrument_const_sptr instr = tempWS->getInstrument();
  outWS->setInstrument(instr);

  IAlgorithm_sptr applyCal = createChildAlgorithm("LoadIsawDetCal");
  applyCal->initialize();
  applyCal->setProperty("InputWorkspace", outWS);
  applyCal->setProperty("Filename", getPropertyValue("Filename"));
  applyCal->executeAsChildAlg();
  T0 = applyCal->getProperty("TimeOffset");

  // Now skip all lines on L1, detector banks, etc. until we get to a block of
  // peaks. They start with 0.
  std::string s;
  std::vector<int> det;
  while (s != "0" && in.good()) {
    readToEndOfLine(in, true);
    s = getWord(in, false);
    int bank = 0;
    // Save all bank numbers in header lines
    Strings::convert(getWord(in, false), bank);
    if (s == "5")
      det.push_back(bank);
  }
  // Find bank numbers in instument that are not in header lines
  std::string maskBanks;
  if (!instr)
    throw std::runtime_error(
        "No instrument in the Workspace. Cannot save DetCal file.");
  // We cannot assume the peaks have bank type detector modules, so we have a
  // string to check this
  std::string bankPart = "bank";
  if (instr->getName() == "WISH")
    bankPart = "WISHpanel";
  // Get all children
  std::vector<IComponent_const_sptr> comps;
  instr->getChildren(comps, true);
  for (auto &comp : comps) {
    std::string bankName = comp->getName();
    boost::trim(bankName);
    boost::erase_all(bankName, bankPart);
    int bank = 0;
    Strings::convert(bankName, bank);
    for (int j : det) {
      if (bank == j) {
        bank = 0;
        continue;
      }
    }
    if (bank == 0)
      continue;
    // Track unique bank numbers
    maskBanks += bankName + ",";
  }

  if (!maskBanks.empty()) {
    // remove last comma
    maskBanks.resize(maskBanks.size() - 1);
    // Mask banks that are not in header lines
    try {
      Algorithm_sptr alg = createChildAlgorithm("MaskBTP");
      alg->setProperty<Workspace_sptr>("Workspace", outWS);
      alg->setProperty("Bank", maskBanks);
      if (!alg->execute())
        throw std::runtime_error(
            "MaskDetectors Child Algorithm has not executed successfully");
    } catch (...) {
      g_log.error("Can't execute MaskBTP algorithm");
    }
  }
  return s;
}

//-----------------------------------------------------------------------------------------------
/** Read one peak in a line of an ISAW peaks file.
 *
 * @param outWS :: workspace to add peaks to
 * @param lastStr [in,out] :: last word (the one at the start of the line)
 * @param in :: input stream
 * @param seqNum [out] :: the sequence number of the peak
 * @param bankName :: the bank number from the ISAW file.
 * @param qSign :: For inelastic this is 1; for crystallography this is -1
 * @return the Peak the Peak object created
 */
DataObjects::Peak LoadIsawPeaks::readPeak(PeaksWorkspace_sptr outWS,
                                          std::string &lastStr,
                                          std::ifstream &in, int &seqNum,
                                          std::string bankName, double qSign) {
  double h;
  double k;
  double l;
  double col;
  double row;
  double wl;
  double IPK;
  double Inti;
  double SigI;

  std::string s = lastStr;

  if (s.length() < 1 && in.good()) // blank line
  {
    readToEndOfLine(in, true);
    s = getWord(in, false);
  }

  if (s.length() < 1)
    throw std::runtime_error("Empty peak line encountered.");

  if (s == "2") {
    readToEndOfLine(in, true);
    for (s = getWord(in, false); s.length() < 1 && in.good();
         s = getWord(in, true)) {
      s = getWord(in, false);
    }
  }

  if (s.length() < 1)
    throw std::runtime_error("Empty peak line encountered.");

  /// If line starts with 3, it contains Bragg peaks
  /// If line starts with 9, it contains Bragg peaks and satellites
  /// with extra columns for mnp
  if (s != "3" && s != "9")
    throw std::runtime_error("Empty peak line encountered.");

  seqNum = std::stoi(getWord(in, false));

  h = qSign * std::stod(getWord(in, false), nullptr);
  k = qSign * std::stod(getWord(in, false), nullptr);
  l = qSign * std::stod(getWord(in, false), nullptr);
  V3D mod = V3D(0, 0, 0);
  V3D intHKL = V3D(h, k, l);
  if (m_isModulatedStructure) {
    mod[0] = qSign * std::stoi(getWord(in, false), nullptr);
    mod[1] = qSign * std::stoi(getWord(in, false), nullptr);
    mod[2] = qSign * std::stoi(getWord(in, false), nullptr);
  }

  col = std::stod(getWord(in, false), nullptr);
  row = std::stod(getWord(in, false), nullptr);
  UNUSED_ARG(std::stod(getWord(in, false), nullptr)); // chan
  UNUSED_ARG(std::stod(getWord(in, false), nullptr)); // L2
  UNUSED_ARG(std::stod(getWord(in, false), nullptr)); // ScatAng

  UNUSED_ARG(std::stod(getWord(in, false), nullptr)); // Az
  wl = std::stod(getWord(in, false), nullptr);
  UNUSED_ARG(std::stod(getWord(in, false), nullptr)); // D
  IPK = std::stod(getWord(in, false), nullptr);

  Inti = std::stod(getWord(in, false), nullptr);
  SigI = std::stod(getWord(in, false), nullptr);
  UNUSED_ARG(std::stoi(getWord(in, false))); // iReflag

  // Finish the line and get the first word of next line
  readToEndOfLine(in, true);
  lastStr = getWord(in, false);

  // Find the detector ID from row/col
  Instrument_const_sptr inst = outWS->getInstrument();
  if (!inst)
    throw std::runtime_error("No instrument in PeaksWorkspace!");

  int pixelID =
      findPixelID(inst, bankName, static_cast<int>(col), static_cast<int>(row));

  // Create the peak object
  Peak peak(outWS->getInstrument(), pixelID, wl);
  peak.setHKL(h, k, l);
  peak.setIntHKL(intHKL);
  peak.setIntMNP(mod);
  peak.setIntensity(Inti);
  peak.setSigmaIntensity(SigI);
  peak.setBinCount(IPK);
  peak.setPeakNumber(seqNum);
  // Return the peak
  return peak;
}

//----------------------------------------------------------------------------------------------
int LoadIsawPeaks::findPixelID(Instrument_const_sptr inst, std::string bankName,
                               int col, int row) {
  boost::shared_ptr<const IComponent> parent =
      getCachedBankByName(bankName, inst);

  if (!parent)
    return -1; // peak not in any detector.

  if (parent->type() == "RectangularDetector") {
    boost::shared_ptr<const RectangularDetector> RDet =
        boost::dynamic_pointer_cast<const RectangularDetector>(parent);

    boost::shared_ptr<Detector> pixel = RDet->getAtXY(col, row);
    return pixel->getID();
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    boost::shared_ptr<const Geometry::ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    if (children[0]->getName() == "sixteenpack") {
      asmb = boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(
          children[0]);
      children.clear();
      asmb->getChildren(children, false);
    }
    int col0 = col - 1;
    // WISH detectors are in bank in this order in instrument
    if (inst->getName() == "WISH")
      col0 = (col % 2 == 0 ? col / 2 + 75 : (col - 1) / 2);
    boost::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(
            children[col0]);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);
    Geometry::IComponent_const_sptr first = grandchildren[row - 1];
    Geometry::IDetector_const_sptr det =
        boost::dynamic_pointer_cast<const Geometry::IDetector>(first);
    return det->getID();
  }
}

//-----------------------------------------------------------------------------------------------
/** Read the header of each peak block section */
std::string LoadIsawPeaks::readPeakBlockHeader(std::string lastStr,
                                               std::ifstream &in, int &run,
                                               int &detName, double &chi,
                                               double &phi, double &omega,
                                               double &monCount) {
  std::string s = lastStr;

  if (s.length() < 1 && in.good()) // blank line
  {
    readToEndOfLine(in, true);
    s = getWord(in, false);
  }

  if (s.length() < 1)
    return std::string();

  if (s == "0") {
    readToEndOfLine(in, true);
    s = getWord(in, false);
    while (s.length() < 1) {
      readToEndOfLine(in, true);
      s = getWord(in, false);
    }
  }

  if (s != "1")
    return s;

  run = std::stoi(getWord(in, false));
  detName = std::stoi(getWord(in, false));
  chi = std::stod(getWord(in, false), nullptr);
  phi = std::stod(getWord(in, false), nullptr);

  omega = std::stod(getWord(in, false), nullptr);
  monCount = std::stod(getWord(in, false), nullptr);
  readToEndOfLine(in, true);

  return getWord(in, false);
}
// special formatted file Use clear peaks to no append

//-----------------------------------------------------------------------------------------------
/** Append the peaks from a .peaks file into the workspace
 * @param outWS :: the workspace in which to place the information
 * @param filename :: path to the .peaks file
 */
void LoadIsawPeaks::appendFile(PeaksWorkspace_sptr outWS,
                               std::string filename) {
  // HKL's are flipped by -1 because of the internal Q convention
  // unless Crystallography convention
  double qSign = -1.0;
  std::string convention = ConfigService::Instance().getString("Q.convention");
  if (convention == "Crystallography")
    qSign = 1.0;
  // Open the file
  std::ifstream in(filename.c_str());

  // Calculate filesize
  in.seekg(0, in.end);
  auto filelen = in.tellg();
  in.seekg(0, in.beg);

  // Read the header, load the instrument
  double T0;
  auto s = readHeader(outWS, in, T0);
  // set T0 in the run parameters
  API::Run &m_run = outWS->mutableRun();
  m_run.addProperty<double>("T0", T0, true);

  if (!in.good() || s.length() < 1)
    throw std::runtime_error("End of Peaks file before peaks");

  if (s != "0")
    throw std::logic_error("No header for Peak segments");

  readToEndOfLine(in, true);
  s = getWord(in, false);

  int run, bankNum;
  double chi, phi, omega, monCount;

  // Build the universal goniometer that will build the rotation matrix.
  Mantid::Geometry::Goniometer uniGonio;
  uniGonio.makeUniversalGoniometer();

  // Progress is reported based on how much of the file we've read
  Progress prog(this, 0.0, 1.0, filelen);

  while (in.good()) {
    // Read the header if necessary
    s = readPeakBlockHeader(s, in, run, bankNum, chi, phi, omega, monCount);
    // Build the Rotation matrix using phi,chi,omega
    uniGonio.setRotationAngle("phi", phi);
    uniGonio.setRotationAngle("chi", chi);
    uniGonio.setRotationAngle("omega", omega);
    // Put goniometer into peaks workspace
    outWS->mutableRun().setGoniometer(uniGonio, false);

    std::ostringstream oss;
    std::string bankString = "bank";
    if (outWS->getInstrument()->getName() == "WISH") {
      if (bankNum < 10)
        bankString = "WISHpanel0";
      else
        bankString = "WISHpanel";
    }
    oss << bankString << bankNum;
    std::string bankName = oss.str();

    int seqNum;

    try {
      // Read the peak
      Peak peak = readPeak(outWS, s, in, seqNum, bankName, qSign);

      // Get the calculated goniometer matrix
      const Matrix<double> &gonMat = uniGonio.getR();

      peak.setGoniometerMatrix(gonMat);
      peak.setRunNumber(run);
      peak.setMonitorCount(monCount);

      double tof = peak.getTOF();
      Kernel::Units::Wavelength wl;

      wl.initialize(peak.getL1(), peak.getL2(), peak.getScattering(), 0,
                    peak.getInitialEnergy(), 0.0);

      peak.setWavelength(wl.singleFromTOF(tof));
      // Add the peak to workspace
      outWS->addPeak(peak);
    } catch (std::runtime_error &e) {
      g_log.error() << "Error reading peak SEQN " << seqNum << " : " << e.what()
                    << '\n';
      throw std::runtime_error("Corrupted input file. ");
    }

    prog.report(in.tellg());
  }
  if (m_isModulatedStructure) {
    IAlgorithm_sptr findUB = createChildAlgorithm("FindUBUsingIndexedPeaks");
    findUB->setPropertyValue("ToleranceForSatellite", "0.05");
    findUB->setProperty<PeaksWorkspace_sptr>("PeaksWorkspace", outWS);
    findUB->executeAsChildAlg();

    if (outWS->mutableSample().hasOrientedLattice()) {
      OrientedLattice o_lattice = outWS->mutableSample().getOrientedLattice();
      auto &peaks = outWS->getPeaks();
      for (auto &peak : peaks) {

        V3D hkl = peak.getHKL();
        V3D mnp = peak.getIntMNP();
        for (int i = 0; i <= 2; i++)
          hkl += o_lattice.getModVec(i) * mnp[i];
        peak.setHKL(hkl);
      }
    }
  }
}

//-----------------------------------------------------------------------------------------------
/** Count the peaks from a .peaks file and compare with the workspace
 * @param outWS :: the workspace in which to place the information
 * @param filename :: path to the .peaks file
 */
void LoadIsawPeaks::checkNumberPeaks(PeaksWorkspace_sptr outWS,
                                     std::string filename) {

  // Open the file
  std::ifstream in(filename.c_str());
  std::string first;
  int NumberPeaks = 0;
  while (getline(in, first)) {
    if (first[0] == '3' || first[0] == '9')
      NumberPeaks++;
  }
  if (NumberPeaks != outWS->getNumberPeaks()) {
    g_log.error() << "Number of peaks in file is " << NumberPeaks
                  << " but only read " << outWS->getNumberPeaks() << '\n';
    throw std::length_error("Wrong number of peaks read");
  }
}

//----------------------------------------------------------------------------------------------
/** Retrieves pointer to given bank from local cache.
 *
 * When the bank isn't in the local cache, it is loaded and
 * added to the cache for later use. Lifetime of the cache
 * is bound to the lifetime of this instance of the algorithm
 * (typically, the instance should be destroyed once exec()
 * finishes).
 *
 * Note that while this is used only for banks here, it would
 * work for caching any component without modification.
 *
 * @param bankname :: the name of the requested bank
 * @param inst :: the instrument from which to load the bank if it is not yet
 *cached
 * @return A shared pointer to the request bank (empty shared pointer if not
 *found)
 */
boost::shared_ptr<const IComponent> LoadIsawPeaks::getCachedBankByName(
    std::string bankname,
    const boost::shared_ptr<const Geometry::Instrument> &inst) {
  if (m_banks.count(bankname) == 0)
    m_banks[bankname] = inst->getComponentByName(bankname);
  return m_banks[bankname];
}

} // namespace Crystal
} // namespace Mantid
