// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/SaveIsawPeaks.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Utils.h"
#include <Poco/File.h>
#include <boost/algorithm/string/trim.hpp>
#include <fstream>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveIsawPeaks)

/** Initialize the algorithm's properties.
 */
void SaveIsawPeaks::init() {
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "An input PeaksWorkspace with an instrument.");

  declareProperty("AppendFile", false,
                  "Append to file if true.\n"
                  "If false, new file (default).");

  const std::vector<std::string> exts{".peaks", ".integrate"};
  declareProperty(
      std::make_unique<FileProperty>("Filename", "", FileProperty::Save, exts),
      "Path to an ISAW-style peaks or integrate file to save.");

  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace2D>>(
          "ProfileWorkspace", "", Direction::Input, PropertyMode::Optional),
      "An optional Workspace2D of profiles from integrating cylinder.");

  declareProperty("RenumberPeaks", false,
                  "If true, sequential peak numbers\n"
                  "If false, keep original numbering (default).");
}

/** Execute the algorithm.
 */
void SaveIsawPeaks::exec() {
  // Section header
  std::string header = "2   SEQN    H    K    L     COL      ROW     CHAN      "
                       "  L2   2_THETA        AZ         WL         D      IPK "
                       "      INTI    SIGI  RFLG";

  std::string filename = getPropertyValue("Filename");
  PeaksWorkspace_sptr ws = getProperty("InputWorkspace");
  std::vector<Peak> peaks = ws->getPeaks();
  inst = ws->getInstrument();
  const auto &detectorInfo = ws->detectorInfo();

  // We must sort the peaks first by run, then bank #, and save the list of
  // workspace indices of it
  using bankMap_t = std::map<int, std::vector<size_t>>;
  using runMap_t = std::map<int, bankMap_t>;
  std::set<int, std::less<int>> uniqueBanks;
  if (!inst)
    throw std::runtime_error(
        "No instrument in the Workspace. Cannot save DetCal file.");
  // We cannot assume the peaks have bank type detector modules, so we have a
  // string to check this
  std::string bankPart = "bank";
  if (inst->getName() == "WISH")
    bankPart = "WISHpanel";

  // Get all children
  std::vector<IComponent_const_sptr> comps;
  inst->getChildren(comps, true);

  for (auto &comp : comps) {
    std::string bankName = comp->getName();
    boost::trim(bankName);
    boost::erase_all(bankName, bankPart);
    int bank = 0;
    Strings::convert(bankName, bank);
    if (bank == 0)
      continue;
    if (bankMasked(comp, detectorInfo))
      continue;
    // Track unique bank numbers
    uniqueBanks.insert(bank);
  }
  runMap_t runMap;
  for (size_t i = 0; i < peaks.size(); ++i) {
    Peak &p = peaks[i];
    if (p.getIntMNP() != V3D(0, 0, 0))
      m_isModulatedStructure = true;
    int run = p.getRunNumber();
    int bank = 0;
    std::string bankName = p.getBankName();
    if (bankName.size() <= 4) {
      g_log.information() << "Could not interpret bank number of peak " << i
                          << "(" << bankName << ")\n";
      continue;
    }
    // Save the "bank" part once to check whether it really is a bank
    if (bankPart == "?")
      bankPart = bankName.substr(0, 4);
    // Take out the "bank" part of the bank name and convert to an int
    if (bankPart == "bank")
      bankName = bankName.substr(4, bankName.size() - 4);
    else if (bankPart == "WISH")
      bankName = bankName.substr(9, bankName.size() - 9);
    Strings::convert(bankName, bank);

    // Save in the map
    runMap[run][bank].push_back(i);
  }
  if (m_isModulatedStructure)
    header =
        "2   SEQN    H    K    L    M    N    P     COL      ROW     CHAN      "
        "  L2   2_THETA        AZ         WL         D      IPK "
        "      INTI    SIGI  RFLG";

  if (!inst)
    throw std::runtime_error(
        "No instrument in PeaksWorkspace. Cannot save peaks file.");

  if (bankPart != "bank" && bankPart != "WISHpanel" && bankPart != "?") {
    std::ostringstream mess;
    mess << "Detector module of type " << bankPart
         << " not supported in ISAWPeaks. Cannot save peaks file";
    throw std::runtime_error(mess.str());
  }

  double l1;
  V3D beamline;
  double beamline_norm;
  V3D samplePos;
  inst->getInstrumentParameters(l1, beamline, beamline_norm, samplePos);

  std::ofstream out;
  bool append = getProperty("AppendFile");
  bool renumber = getProperty("RenumberPeaks");

  // do not append if file does not exist
  if (!Poco::File(filename.c_str()).exists())
    append = false;

  int appendPeakNumb = 0;
  if (append) {
    std::ifstream infile(filename.c_str());
    std::string line;
    while (!infile.eof()) // To get you all the lines.
    {
      getline(infile, line); // Saves the line in STRING.
      if (infile.eof())
        break;
      std::stringstream ss(line);
      double three;
      ss >> three;
      if (three == 3) {
        int peakNumber;
        ss >> peakNumber;
        appendPeakNumb = std::max(peakNumber, appendPeakNumb);
      }
    }

    infile.close();
    out.open(filename.c_str(), std::ios::app);
    appendPeakNumb = appendPeakNumb + 1;
  } else {
    out.open(filename.c_str());

    out << "Version: 2.0  Facility: SNS ";
    out << " Instrument: " << inst->getName() << "  Date: ";

    // TODO: The experiment date might be more useful than the instrument date.
    // For now, this allows the proper instrument to be loaded back after
    // saving.
    Types::Core::DateAndTime expDate = inst->getValidFromDate() + 1.0;
    out << expDate.toISO8601String();
    if (m_isModulatedStructure)
      out << " MOD";
    out << '\n';

    out << "6         L1    T0_SHIFT\n";
    out << "7 " << std::setw(10);
    out << std::setprecision(4) << std::fixed << (l1 * 100);
    out << std::setw(12) << std::setprecision(3) << std::fixed;
    // Time offset from property
    const API::Run &run = ws->run();
    double T0 = 0.0;
    if (run.hasProperty("T0")) {
      T0 = run.getPropertyValueAsType<double>("T0");
      if (T0 != 0) {
        g_log.notice() << "T0 = " << T0 << '\n';
      }
    }
    out << T0 << '\n';

    // ============================== Save .detcal info
    // =========================================
    if (true) {
      out << "4 DETNUM  NROWS  NCOLS   WIDTH   HEIGHT   DEPTH   DETD   CenterX "
             "  CenterY   CenterZ    BaseX    BaseY    BaseZ      UpX      UpY "
             "     UpZ\n";
      // Here would save each detector...
      for (const auto bank : uniqueBanks) {
        // Build up the bank name
        std::ostringstream mess;
        if (bankPart == "bank")
          mess << "bank" << bank;
        else if (bankPart == "WISH" && bank < 10)
          mess << "WISHpanel0" << bank;
        else if (bankPart == "WISH")
          mess << "WISHpanel" << bank;

        std::string bankName = mess.str();
        // Retrieve it
        boost::shared_ptr<const IComponent> det =
            inst->getComponentByName(bankName);
        if (inst->getName() ==
            "CORELLI") // for Corelli with sixteenpack under bank
        {
          std::vector<Geometry::IComponent_const_sptr> children;
          boost::shared_ptr<const Geometry::ICompAssembly> asmb =
              boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(
                  inst->getComponentByName(bankName));
          asmb->getChildren(children, false);
          det = children[0];
        }
        if (det) {
          // Center of the detector
          V3D center = det->getPos();

          // Distance to center of detector
          double detd = (center - inst->getSample()->getPos()).norm();
          int NCOLS, NROWS;
          double xsize, ysize;
          sizeBanks(bankName, NCOLS, NROWS, xsize, ysize);
          // Base unit vector (along the horizontal, X axis)
          int midX = NCOLS / 2;
          int midY = NROWS / 2;
          V3D base = findPixelPos(bankName, midX + 1, midY) -
                     findPixelPos(bankName, midX, midY);
          base.normalize();

          // Up unit vector (along the vertical, Y axis)
          V3D up = findPixelPos(bankName, midX, midY + 1) -
                   findPixelPos(bankName, midX, midY);
          up.normalize();

          // Write the line
          out << "5 " << std::setw(6) << std::right << bank << " "
              << std::setw(6) << std::right << NROWS << " " << std::setw(6)
              << std::right << NCOLS << " " << std::setw(7) << std::right
              << std::fixed << std::setprecision(4) << 100.0 * xsize << " "
              << std::setw(7) << std::right << std::fixed
              << std::setprecision(4) << 100.0 * ysize << " "
              << "  0.2000 " << std::setw(6) << std::right << std::fixed
              << std::setprecision(2) << 100.0 * detd << " " << std::setw(9)
              << std::right << std::fixed << std::setprecision(4)
              << 100.0 * center.X() << " " << std::setw(9) << std::right
              << std::fixed << std::setprecision(4) << 100.0 * center.Y() << " "
              << std::setw(9) << std::right << std::fixed
              << std::setprecision(4) << 100.0 * center.Z() << " "
              << std::setw(8) << std::right << std::fixed
              << std::setprecision(5) << base.X() << " " << std::setw(8)
              << std::right << std::fixed << std::setprecision(5) << base.Y()
              << " " << std::setw(8) << std::right << std::fixed
              << std::setprecision(5) << base.Z() << " " << std::setw(8)
              << std::right << std::fixed << std::setprecision(5) << up.X()
              << " " << std::setw(8) << std::right << std::fixed
              << std::setprecision(5) << up.Y() << " " << std::setw(8)
              << std::right << std::fixed << std::setprecision(5) << up.Z()
              << " \n";

        } else
          g_log.warning() << "Information about detector module " << bankName
                          << " not found and recognised\n";
      }
    }
  }
  // HKL's are flipped by -1 because of the internal Q convention
  // unless Crystallography convention
  double qSign = -1.0;
  if (ws->getConvention() == "Crystallography")
    qSign = 1.0;
  // ============================== Save all Peaks
  // =========================================

  // Go in order of run numbers
  int sequenceNumber = appendPeakNumb;
  runMap_t::iterator runMap_it;
  for (runMap_it = runMap.begin(); runMap_it != runMap.end(); ++runMap_it) {
    // Start of a new run
    int run = runMap_it->first;
    bankMap_t &bankMap = runMap_it->second;

    bankMap_t::iterator bankMap_it;
    for (bankMap_it = bankMap.begin(); bankMap_it != bankMap.end();
         ++bankMap_it) {
      // Start of a new bank.
      int bank = bankMap_it->first;
      std::vector<size_t> &ids = bankMap_it->second;

      if (!ids.empty()) {
        // Write the bank header
        out << "0  NRUN DETNUM     CHI      PHI    OMEGA       MONCNT\n";
        out << "1 " << std::setw(5) << run << std::setw(7) << std::right
            << bank;

        // Determine goniometer angles by calculating from the goniometer matrix
        // of a peak in the list
        Goniometer gon(peaks[ids[0]].getGoniometerMatrix());
        std::vector<double> angles = gon.getEulerAngles("yzy");

        double phi = angles[2];
        double chi = angles[1];
        double omega = angles[0];

        out << std::setw(8) << std::fixed << std::setprecision(2) << chi << " ";
        out << std::setw(8) << std::fixed << std::setprecision(2) << phi << " ";
        out << std::setw(8) << std::fixed << std::setprecision(2) << omega
            << " ";

        // Get the monitor count from the first peak (should all be the same for
        // one run)
        size_t first_peak_index = ids[0];
        Peak &first_peak = peaks[first_peak_index];
        double monct = first_peak.getMonitorCount();
        out << std::setw(12) << static_cast<int>(monct) << '\n';

        out << header << '\n';

        // Go through each peak at this run / bank
        for (auto wi : ids) {
          Peak &p = peaks[wi];

          // Sequence (run) number
          std::string firstNumber = "3";
          if (m_isModulatedStructure) {
            firstNumber = "9";
          }
          if (renumber) {
            out << firstNumber << std::setw(7) << sequenceNumber;
            sequenceNumber++;
          } else {
            out << firstNumber << std::setw(7)
                << p.getPeakNumber() + appendPeakNumb;
          }

          // HKL's are flipped by -1 because of the internal Q convention
          // unless Crystallography convention
          if (m_isModulatedStructure) {
            V3D mod = p.getIntMNP();
            auto intHKL = p.getIntHKL();
            out << std::setw(5) << Utils::round(qSign * intHKL.X())
                << std::setw(5) << Utils::round(qSign * intHKL.Y())
                << std::setw(5) << Utils::round(qSign * intHKL.Z());

            out << std::setw(5) << Utils::round(qSign * mod[0]) << std::setw(5)
                << Utils::round(qSign * mod[1]) << std::setw(5)
                << Utils::round(qSign * mod[2]);
          } else {
            out << std::setw(5) << Utils::round(qSign * p.getH())
                << std::setw(5) << Utils::round(qSign * p.getK())
                << std::setw(5) << Utils::round(qSign * p.getL());
          }

          // Row/column
          out << std::setw(8) << std::fixed << std::setprecision(2)
              << static_cast<double>(p.getCol()) << " ";

          out << std::setw(8) << std::fixed << std::setprecision(2)
              << static_cast<double>(p.getRow()) << " ";

          out << std::setw(8) << std::fixed << std::setprecision(0)
              << p.getTOF() << " ";

          out << std::setw(9) << std::fixed << std::setprecision(3)
              << (p.getL2() * 100.0) << " ";

          // This is the scattered beam direction
          V3D dir = p.getDetPos() - inst->getSample()->getPos();
          double scattering, azimuth;

          // Two-theta = polar angle = scattering angle = between +Z vector and
          // the scattered beam
          scattering = dir.angle(V3D(0.0, 0.0, 1.0));

          // "Azimuthal" angle: project the scattered beam direction onto the XY
          // plane,
          // and calculate the angle between that and the +X axis (right-handed)
          azimuth = atan2(dir.Y(), dir.X());

          out << std::setw(9) << std::fixed << std::setprecision(5)
              << scattering << " "; // two-theta scattering

          out << std::setw(9) << std::fixed << std::setprecision(5) << azimuth
              << " ";

          out << std::setw(10) << std::fixed << std::setprecision(6)
              << p.getWavelength() << " ";

          out << std::setw(9) << std::fixed << std::setprecision(4)
              << p.getDSpacing() << " ";

          out << std::setw(8) << std::fixed << std::setprecision(0)
              << int(p.getBinCount()) << " ";

          out << std::setw(10) << std::fixed << std::setprecision(2)
              << p.getIntensity() << " ";

          out << std::setw(7) << std::fixed << std::setprecision(2)
              << p.getSigmaIntensity() << " ";

          int thisReflag = 310;
          out << std::setw(5) << thisReflag;

          out << '\n';

          Workspace2D_sptr wsProfile2D = getProperty("ProfileWorkspace");
          if (wsProfile2D) {
            out << "8";
            const auto &yValues = wsProfile2D->y(wi);
            for (size_t j = 0; j < yValues.size(); j++) {
              out << std::setw(8) << static_cast<int>(yValues[j]);
              if ((j + 1) % 10 == 0) {
                out << '\n';
                if (j + 1 != yValues.size())
                  out << "8";
              }
            }
          }
        }
      }
    }
  }

  out.flush();
  out.close();
}

bool SaveIsawPeaks::bankMasked(IComponent_const_sptr parent,
                               const Geometry::DetectorInfo &detectorInfo) {
  std::vector<Geometry::IComponent_const_sptr> children;
  boost::shared_ptr<const Geometry::ICompAssembly> asmb =
      boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
  asmb->getChildren(children, false);
  if (children[0]->getName() == "sixteenpack") {
    asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
    children.clear();
    asmb->getChildren(children, false);
  }

  for (auto &col : children) {
    boost::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(col);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);

    for (auto &row : grandchildren) {
      Detector *d =
          dynamic_cast<Detector *>(const_cast<IComponent *>(row.get()));
      if (d) {
        auto detID = d->getID();
        if (detID < 0)
          continue;
        const auto index = detectorInfo.indexOf(detID);
        if (!detectorInfo.isMasked(index))
          return false;
      }
    }
  }
  return true;
}

V3D SaveIsawPeaks::findPixelPos(std::string bankName, int col, int row) {
  boost::shared_ptr<const IComponent> parent =
      inst->getComponentByName(bankName);
  if (parent->type() == "RectangularDetector") {
    boost::shared_ptr<const RectangularDetector> RDet =
        boost::dynamic_pointer_cast<const RectangularDetector>(parent);

    boost::shared_ptr<Detector> pixel = RDet->getAtXY(col, row);
    return pixel->getPos();
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
    return first->getPos();
  }
}
void SaveIsawPeaks::sizeBanks(std::string bankName, int &NCOLS, int &NROWS,
                              double &xsize, double &ysize) {
  if (bankName == "None")
    return;
  boost::shared_ptr<const IComponent> parent =
      inst->getComponentByName(bankName);
  if (parent->type() == "RectangularDetector") {
    boost::shared_ptr<const RectangularDetector> RDet =
        boost::dynamic_pointer_cast<const RectangularDetector>(parent);

    NCOLS = RDet->xpixels();
    NROWS = RDet->ypixels();
    xsize = RDet->xsize();
    ysize = RDet->ysize();
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
    boost::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);
    NROWS = static_cast<int>(grandchildren.size());
    NCOLS = static_cast<int>(children.size());
    Geometry::IComponent_const_sptr first = children[0];
    Geometry::IComponent_const_sptr last = children[NCOLS - 1];
    xsize = first->getDistance(*last);
    first = grandchildren[0];
    last = grandchildren[NROWS - 1];
    ysize = first->getDistance(*last);
  }
}
void SaveIsawPeaks::writeOffsets(std::ofstream &out, double qSign,
                                 std::vector<double> offset) {
  for (size_t i = 0; i < 3; i++) {
    out << std::setw(12) << std::fixed << std::setprecision(6)
        << qSign * offset[i] << " ";
  }
}
} // namespace Crystal
} // namespace Mantid
