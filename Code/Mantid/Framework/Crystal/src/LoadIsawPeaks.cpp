#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <math.h>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "MantidKernel/Unit.h"

using Mantid::Kernel::Strings::readToEndOfLine;
using Mantid::Kernel::Strings::getWord;
using Mantid::Kernel::Units::Wavelength;

namespace Mantid {
namespace Crystal {

DECLARE_FILELOADER_ALGORITHM(LoadIsawPeaks);

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadIsawPeaks::LoadIsawPeaks() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadIsawPeaks::~LoadIsawPeaks() {}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadIsawPeaks::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &extn = descriptor.extension();
  // If the extension is peaks or integrate then give it a go
  if (extn.compare(".peaks") != 0 && extn.compare(".integrate") != 0)
    return 0;

  int confidence(0);
  try {
    auto &in = descriptor.data();
    // Read the header, load the instrument
    std::string tag;
    std::string r = getWord(in, false);

    if (r.length() < 1)
      throw std::logic_error(std::string("No first line of Peaks file"));

    if (r.compare("Version:") != 0)
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
    Kernel::DateAndTime C_experimentDate;
    tag = getWord(in, false);
    if (tag == "Date:")
      getWord(in, false);
    readToEndOfLine(in, true);
    confidence = 95;
  }
  catch (std::exception &) {
  }
  return confidence;
}

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadIsawPeaks::init() {
  std::vector<std::string> exts;
  exts.push_back(".peaks");
  exts.push_back(".integrate");

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "Path to an ISAW-style .peaks filename.");
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Name of the output workspace.");
}

std::string
LoadIsawPeaks::ApplyCalibInfo(std::ifstream &in, std::string startChar,
                              Geometry::Instrument_const_sptr instr_old,
                              Geometry::Instrument_const_sptr instr,
                              double &T0) {
  ParameterMap_sptr parMap1 = instr_old->getParameterMap();

  ParameterMap_sptr parMap = instr->getParameterMap();

  while (in.good() && (startChar.size() < 1 || startChar != "7")) {
    readToEndOfLine(in, true);
    startChar = getWord(in, false);
  }
  if (!(in.good())) {
    // g_log.error()<<"Peaks file has no time shift and L0 info"<<std::endl;
    throw std::invalid_argument("Peaks file has no time shift and L0 info");
  }
  std::string L1s = getWord(in, false);
  std::string T0s = getWord(in, false);
  if (L1s.length() < 1 || T0s.length() < 1) {
    g_log.error() << "Missing L1 or Time offset" << std::endl;
    throw std::invalid_argument("Missing L1 or Time offset");
  }

  try {
    std::istringstream iss(L1s + " " + T0s, std::istringstream::in);
    double L1;
    iss >> L1;
    iss >> T0;
    V3D sampPos = instr->getSample()->getPos();
    SCDCalibratePanels::FixUpSourceParameterMap(instr, L1 / 100, sampPos,
                                                parMap1);
  }
  catch (...) {
    g_log.error() << "Invalid L1 or Time offset" << std::endl;
    throw std::invalid_argument("Invalid L1 or Time offset");
  }

  readToEndOfLine(in, true);
  startChar = getWord(in, false);
  while (in.good() && (startChar.size() < 1 || startChar != "5")) {
    readToEndOfLine(in, true);
    startChar = getWord(in, false);
  }

  if (!(in.good())) {
    g_log.error() << "Peaks file has no detector panel info" << std::endl;
    throw std::invalid_argument("Peaks file has no detector panel info");
  }

  while (startChar == "5") {

    std::string line;
    for (int i = 0; i < 16; i++) {
      std::string s = getWord(in, false);
      if (s.size() < 1) {
        g_log.error() << "Not enough info to describe panel " << std::endl;
        throw std::length_error("Not enough info to describe panel ");
      }
      line += " " + s;
      ;
    }

    readToEndOfLine(in, true);
    startChar = getWord(in, false); // blank lines ?? and # lines ignore

    std::istringstream iss(line, std::istringstream::in);
    int bankNum;
    double width, height, Centx, Centy, Centz, Basex, Basey, Basez, Upx, Upy,
        Upz;
    try {
      int nrows, ncols;
      double depth, detD;
      iss >> bankNum >> nrows >> ncols >> width >> height >> depth >> detD >>
          Centx >> Centy >> Centz >> Basex >> Basey >> Basez >> Upx >> Upy >>
          Upz;
    }
    catch (...) {

      g_log.error() << "incorrect type of data for panel " << std::endl;
      throw std::length_error("incorrect type of data for panel ");
    }

    std::string SbankNum = boost::lexical_cast<std::string>(bankNum);
    std::string bankName = "bank";
    if (instr->getName() == "WISH")
      bankName = "WISHpanel0";
    bankName += SbankNum;
    boost::shared_ptr<const Geometry::IComponent> bank =
        instr_old->getComponentByName(bankName);

    if (!bank) {
      g_log.error() << "There is no bank " << bankName << " in the instrument"
                    << std::endl;
      throw std::length_error("There is no bank " + bankName +
                              " in the instrument");
    }

    V3D dPos = V3D(Centx, Centy, Centz) / 100.0 - bank->getPos();
    V3D Base(Basex, Basey, Basez), Up(Upx, Upy, Upz);
    V3D ToSamp = Base.cross_prod(Up);
    Base.normalize();
    Up.normalize();
    ToSamp.normalize();
    Quat thisRot(Base, Up, ToSamp);
    Quat bankRot(bank->getRotation());
    bankRot.inverse();
    Quat dRot = thisRot * bankRot;

    boost::shared_ptr<const Geometry::RectangularDetector> bankR =
        boost::dynamic_pointer_cast<const Geometry::RectangularDetector>(bank);

    if (!bankR)
      return startChar;

    double DetWScale = 1, DetHtScale = 1;
    if (bank) {
      DetWScale = width / bankR->xsize() / 100;
      DetHtScale = height / bankR->ysize() / 100;
    }
    std::vector<std::string> bankNames;
    bankNames.push_back(bankName);

    SCDCalibratePanels::FixUpBankParameterMap(
        bankNames, instr, dPos, dRot, DetWScale, DetHtScale, parMap1, false);
  }
  return startChar;
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

  if (r.compare(std::string("Version:")) != 0)
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
  Kernel::DateAndTime C_experimentDate;
  std::string date;
  tag = getWord(in, false);
  if (tag.empty())
    date = Kernel::DateAndTime::getCurrentTime().toISO8601String();
  else if (tag == "Date:")
    date = getWord(in, false);
  readToEndOfLine(in, true);

  // Now we load the instrument using the name and date
  MatrixWorkspace_sptr tempWS =
      WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
  tempWS->mutableRun().addProperty<std::string>("run_start", date);

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("InstrumentName", C_Instrument);
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
  loadInst->executeAsChildAlg();

  // Populate the instrument parameters in this workspace - this works around a
  // bug
  tempWS->populateInstrumentParameters();
  Geometry::Instrument_const_sptr instr_old = tempWS->getInstrument();
  boost::shared_ptr<ParameterMap> map(new ParameterMap());
  Geometry::Instrument_const_sptr instr(
      new Geometry::Instrument(instr_old->baseInstrument(), map));

  // std::string s;
  std::string s = ApplyCalibInfo(in, "", instr_old, instr, T0);
  outWS->setInstrument(instr);

  // Now skip all lines on L1, detector banks, etc. until we get to a block of
  // peaks. They start with 0.
  // readToEndOfLine( in ,  true );
  // readToEndOfLine( in ,  true );
  // s = getWord(in, false);
  while (s != "0" && in.good()) {
    readToEndOfLine(in, true);
    s = getWord(in, false);
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
 * @return the Peak the Peak object created
 */
Mantid::DataObjects::Peak readPeak(PeaksWorkspace_sptr outWS,
                                   std::string &lastStr, std::ifstream &in,
                                   int &seqNum, std::string bankName) {
  double h;
  double k;
  double l;
  double col;
  double row;
  double wl;
  double IPK;
  double Inti;
  double SigI;

  seqNum = -1;

  std::string s = lastStr;
  if (s.length() < 1 && in.good()) // blank line
  {
    readToEndOfLine(in, true);
    s = getWord(in, false);
    ;
  }

  if (s.length() < 1)
    throw std::runtime_error("Empty peak line encountered.");

  if (s.compare("2") == 0) {
    readToEndOfLine(in, true);
    for (s = getWord(in, false); s.length() < 1 && in.good();
         s = getWord(in, true)) {
      s = getWord(in, false);
    }
  }

  if (s.length() < 1)
    throw std::runtime_error("Empty peak line encountered.");

  if (s.compare("3") != 0)
    throw std::runtime_error("Empty peak line encountered.");

  seqNum = atoi(getWord(in, false).c_str());

  h = strtod(getWord(in, false).c_str(), 0);
  k = strtod(getWord(in, false).c_str(), 0);
  l = strtod(getWord(in, false).c_str(), 0);

  col = strtod(getWord(in, false).c_str(), 0);
  row = strtod(getWord(in, false).c_str(), 0);
  strtod(getWord(in, false).c_str(), 0); // chan
  strtod(getWord(in, false).c_str(), 0); // L2
  strtod(getWord(in, false).c_str(), 0); // ScatAng

  strtod(getWord(in, false).c_str(), 0); // Az
  wl = strtod(getWord(in, false).c_str(), 0);
  strtod(getWord(in, false).c_str(), 0); // D
  IPK = strtod(getWord(in, false).c_str(), 0);

  Inti = strtod(getWord(in, false).c_str(), 0);
  SigI = strtod(getWord(in, false).c_str(), 0);
  atoi(getWord(in, false).c_str()); // iReflag

  // Finish the line and get the first word of next line
  readToEndOfLine(in, true);
  lastStr = getWord(in, false);

  // Find the detector ID from row/col
  Instrument_const_sptr inst = outWS->getInstrument();
  if (!inst)
    throw std::runtime_error("No instrument in PeaksWorkspace!");
  LoadIsawPeaks u;
  int pixelID = u.findPixelID(inst, bankName, static_cast<int>(col),
                              static_cast<int>(row));

  // Create the peak object
  Peak peak(outWS->getInstrument(), pixelID, wl);
  // HKL's are flipped by -1 because of the internal Q convention
  peak.setHKL(-h, -k, -l);
  peak.setIntensity(Inti);
  peak.setSigmaIntensity(SigI);
  peak.setBinCount(IPK);
  // Return the peak
  return peak;
}

int LoadIsawPeaks::findPixelID(Instrument_const_sptr inst, std::string bankName,
                               int col, int row) {
  boost::shared_ptr<const IComponent> parent =
      inst->getComponentByName(bankName);
  if (parent->type().compare("RectangularDetector") == 0) {
    boost::shared_ptr<const RectangularDetector> RDet =
        boost::dynamic_pointer_cast<const RectangularDetector>(parent);

    boost::shared_ptr<Detector> pixel = RDet->getAtXY(col, row);
    return pixel->getID();
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    boost::shared_ptr<const Geometry::ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    int col0 = col;
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
std::string readPeakBlockHeader(std::string lastStr, std::ifstream &in,
                                int &run, int &detName, double &chi,
                                double &phi, double &omega, double &monCount) {
  std::string s = lastStr;

  if (s.length() < 1 && in.good()) // blank line
  {
    readToEndOfLine(in, true);
    s = getWord(in, false);
  }

  if (s.length() < 1)
    return std::string();

  if (s.compare("0") == 0) {
    readToEndOfLine(in, true);
    s = getWord(in, false);
    while (s.length() < 1) {
      readToEndOfLine(in, true);
      s = getWord(in, false);
    }
  }

  if (s.compare(std::string("1")) != 0)
    return s;

  run = atoi(getWord(in, false).c_str());
  detName = atoi(getWord(in, false).c_str());
  chi = strtod(getWord(in, false).c_str(), 0);
  phi = strtod(getWord(in, false).c_str(), 0);

  omega = strtod(getWord(in, false).c_str(), 0);
  monCount = strtod(getWord(in, false).c_str(), 0);
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

  // Open the file
  std::ifstream in(filename.c_str());

  // Read the header, load the instrument
  double T0;
  std::string s = readHeader(outWS, in, T0);
  // set T0 in the run parameters
  API::Run &m_run = outWS->mutableRun();
  m_run.addProperty<double>("T0", T0, true);

  if (!in.good() || s.length() < 1)
    throw std::runtime_error("End of Peaks file before peaks");

  if (s.compare(std::string("0")) != 0)
    throw std::logic_error("No header for Peak segments");

  readToEndOfLine(in, true);
  s = getWord(in, false);

  int run, bankNum;
  double chi, phi, omega, monCount;

  // Build the universal goniometer that will build the rotation matrix.
  Mantid::Geometry::Goniometer uniGonio;
  uniGonio.makeUniversalGoniometer();

  // TODO: Can we find the number of peaks to get better progress reporting?
  Progress prog(this, 0.0, 1.0, 100);

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
    if (outWS->getInstrument()->getName() == "WISH")
      bankString = "WISHpanel0";
    oss << bankString << bankNum;
    std::string bankName = oss.str();

    int seqNum = -1;

    try {
      // Read the peak
      Peak peak = readPeak(outWS, s, in, seqNum, bankName);

      // Get the calculated goniometer matrix
      Matrix<double> gonMat = uniGonio.getR();

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
    }
    catch (std::runtime_error &e) {
      g_log.warning() << "Error reading peak SEQN " << seqNum << " : "
                      << e.what() << std::endl;
    }

    prog.report();
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
    if (first[0] == '3')
      NumberPeaks++;
  }
  if (NumberPeaks != outWS->getNumberPeaks()) {
    g_log.error() << "Number of peaks in file is " << NumberPeaks
                  << " but only read " << outWS->getNumberPeaks() << std::endl;
    throw std::length_error("Wrong number of peaks read");
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadIsawPeaks::exec() {
  // Create the workspace
  PeaksWorkspace_sptr ws(new PeaksWorkspace());

  // This loads (appends) the peaks
  this->appendFile(ws, getPropertyValue("Filename"));

  // Save it in the output
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(ws));

  this->checkNumberPeaks(ws, getPropertyValue("Filename"));
}

} // namespace Mantid
} // namespace Crystal
