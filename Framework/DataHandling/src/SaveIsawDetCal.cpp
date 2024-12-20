// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveIsawDetCal.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/ArrayProperty.h"

#include "MantidKernel/Strings.h"

#include <Poco/File.h>
#include <boost/algorithm/string/trim.hpp>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::Types::Core::DateAndTime;
using std::string;

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveIsawDetCal)

/** Initialize the algorithm's properties.
 */
void SaveIsawDetCal::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");

  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save, ".DetCal"),
                  "Path to an ISAW-style .detcal file to save.");

  declareProperty("TimeOffset", 0.0, "Offsets to be applied to times");
  declareProperty(std::make_unique<ArrayProperty<string>>("BankNames", Direction::Input),
                  "Optional: Only select the specified banks");
  declareProperty("AppendFile", false,
                  "Append to file if true.\n"
                  "If false, new file (default).");
}

/** Execute the algorithm.
 */
void SaveIsawDetCal::exec() {
  std::string filename = getPropertyValue("Filename");
  // MatrixWorkspace_sptr ws = getProperty("InputWorkspace");

  Workspace_sptr ws1 = getProperty("InputWorkspace");
  ExperimentInfo_sptr ws = std::dynamic_pointer_cast<ExperimentInfo>(ws1);

  double T0 = getProperty("TimeOffset");
  const API::Run &run = ws->run();
  // Use T0 from workspace if T0 not specified in input
  if (T0 == 0.0 && run.hasProperty("T0")) {
    T0 = run.getPropertyValueAsType<double>("T0");
    if (T0 != 0) {
      g_log.notice() << "T0 = " << T0 << '\n';
    }
  }

  std::vector<std::string> bankNames = getProperty("BankNames");

  inst = ws->getInstrument();
  if (!inst)
    throw std::runtime_error("No instrument in the Workspace. Cannot save DetCal file.");
  // We cannot assume the peaks have bank type detector modules, so we have a
  // string to check this
  std::string bankPart = "bank";
  if (inst->getName() == "WISH")
    bankPart = "WISHpanel";

  std::set<int> uniqueBanks;
  if (bankNames.empty()) {
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
      // Track unique bank numbers
      uniqueBanks.insert(bank);
    }
  } else {
    for (auto bankName : bankNames) {
      boost::trim(bankName);
      boost::erase_all(bankName, bankPart);
      int bank = 0;
      Strings::convert(bankName, bank);
      if (bank == 0)
        continue;
      // Track unique bank numbers
      uniqueBanks.insert(bank);
    }
  }
  if (!inst)
    throw std::runtime_error("No instrument in PeaksWorkspace. Cannot save peaks file.");

  double l1;
  V3D beamline;
  double beamline_norm;
  V3D samplePos;
  inst->getInstrumentParameters(l1, beamline, beamline_norm, samplePos);

  std::ofstream out;
  bool append = getProperty("AppendFile");

  // do not append if file does not exist
  if (!Poco::File(filename.c_str()).exists())
    append = false;

  if (append) {
    out.open(filename.c_str(), std::ios::app);
  } else {
    out.open(filename.c_str());
    out << "# NEW CALIBRATION FILE FORMAT (in NeXus/SNS coordinates):\n";
    out << "# Lengths are in centimeters.\n";
    out << "# Base and up give directions of unit vectors for a local \n";
    out << "# x,y coordinate system on the face of the detector.\n";
    out << "#\n";
    out << "#\n";
    out << "# " << DateAndTime::getCurrentTime().toISO8601String() << '\n';

    out << "6         L1     T0_SHIFT\n";
    out << "7 " << std::setw(10);
    out << std::setprecision(4) << std::fixed << (l1 * 100);
    out << std::setw(13) << std::setprecision(3) << T0 << '\n';

    out << "4 DETNUM  NROWS  NCOLS   WIDTH   HEIGHT   DEPTH   DETD   CenterX "
           "  CenterY   CenterZ    BaseX    BaseY    BaseZ      UpX      UpY "
           "     UpZ\n";
  }
  // Here would save each detector...
  std::set<int>::iterator it;
  for (it = uniqueBanks.begin(); it != uniqueBanks.end(); ++it) {
    // Build up the bank name
    int bank = *it;
    std::ostringstream mess;
    if (bankPart == "WISHpanel" && bank < 10)
      mess << bankPart << "0" << bank;
    else
      mess << bankPart << bank;

    std::string bankName = mess.str();
    // Retrieve it
    std::shared_ptr<const IComponent> det = inst->getComponentByName(bankName);
    if (inst->getName() == "CORELLI") // for Corelli with sixteenpack under bank
    {
      std::vector<Geometry::IComponent_const_sptr> children;
      std::shared_ptr<const Geometry::ICompAssembly> asmb =
          std::dynamic_pointer_cast<const Geometry::ICompAssembly>(inst->getComponentByName(bankName));
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
      // Since WISH banks are curved use center and increment 2 for tubedown
      int midX = NCOLS / 2;
      int midY = NROWS / 2;
      V3D base = findPixelPos(bankName, midX + 2, midY) - findPixelPos(bankName, midX, midY);
      base.normalize();

      // Up unit vector (along the vertical, Y axis)
      V3D up = findPixelPos(bankName, midX, midY + 1) - findPixelPos(bankName, midX, midY);
      up.normalize();

      // Write the line
      out << "5 " << std::setw(6) << std::right << bank << " " << std::setw(6) << std::right << NROWS << " "
          << std::setw(6) << std::right << NCOLS << " " << std::setw(7) << std::right << std::fixed
          << std::setprecision(4) << 100.0 * xsize << " " << std::setw(7) << std::right << std::fixed
          << std::setprecision(4) << 100.0 * ysize << " "
          << "  0.2000 " << std::setw(6) << std::right << std::fixed << std::setprecision(2) << 100.0 * detd << " "
          << std::setw(9) << std::right << std::fixed << std::setprecision(4) << 100.0 * center.X() << " "
          << std::setw(9) << std::right << std::fixed << std::setprecision(4) << 100.0 * center.Y() << " "
          << std::setw(9) << std::right << std::fixed << std::setprecision(4) << 100.0 * center.Z() << " "
          << std::setw(8) << std::right << std::fixed << std::setprecision(5) << base.X() << " " << std::setw(8)
          << std::right << std::fixed << std::setprecision(5) << base.Y() << " " << std::setw(8) << std::right
          << std::fixed << std::setprecision(5) << base.Z() << " " << std::setw(8) << std::right << std::fixed
          << std::setprecision(5) << up.X() << " " << std::setw(8) << std::right << std::fixed << std::setprecision(5)
          << up.Y() << " " << std::setw(8) << std::right << std::fixed << std::setprecision(5) << up.Z() << " \n";

    } else
      g_log.warning() << "Information about detector module " << bankName << " not found and recognised\n";
  }

  out.close();
}

V3D SaveIsawDetCal::findPixelPos(const std::string &bankName, int col, int row) {
  std::shared_ptr<const IComponent> parent = inst->getComponentByName(bankName);
  if (parent->type() == "RectangularDetector") {
    std::shared_ptr<const RectangularDetector> RDet = std::dynamic_pointer_cast<const RectangularDetector>(parent);

    std::shared_ptr<Detector> pixel = RDet->getAtXY(col, row);
    return pixel->getPos();
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    std::shared_ptr<const Geometry::ICompAssembly> asmb =
        std::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    if (children[0]->getName() == "sixteenpack") {
      asmb = std::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
      children.clear();
      asmb->getChildren(children, false);
    }
    int col0 = col - 1;
    // WISH detectors are in bank in this order in instrument
    if (inst->getName() == "WISH")
      col0 = (col % 2 == 0 ? col / 2 + 75 : (col - 1) / 2);
    std::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        std::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[col0]);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);
    Geometry::IComponent_const_sptr first = grandchildren[row - 1];
    return first->getPos();
  }
}

void SaveIsawDetCal::sizeBanks(const std::string &bankName, int &NCOLS, int &NROWS, double &xsize, double &ysize) {
  if (bankName == "None")
    return;
  std::shared_ptr<const IComponent> parent = inst->getComponentByName(bankName);
  if (parent->type() == "RectangularDetector") {
    std::shared_ptr<const RectangularDetector> RDet = std::dynamic_pointer_cast<const RectangularDetector>(parent);

    NCOLS = RDet->xpixels();
    NROWS = RDet->ypixels();
    xsize = RDet->xsize();
    ysize = RDet->ysize();
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    std::shared_ptr<const Geometry::ICompAssembly> asmb =
        std::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    if (children[0]->getName() == "sixteenpack") {
      asmb = std::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
      children.clear();
      asmb->getChildren(children, false);
    }
    std::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        std::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
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
} // namespace Mantid::DataHandling
