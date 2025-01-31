// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/SaveLauenorm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/AnvredCorrection.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Utils.h"
#include "boost/math/special_functions/round.hpp"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <cmath>
#include <fstream>
#include <iomanip>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::PhysicalConstants;

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveLauenorm)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveLauenorm::init() {
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input PeaksWorkspace.");
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Save),
                  "Select the directory and base name for the output files.");
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("ScalePeaks", 1.0, mustBePositive, "Multiply FSQ and sig(FSQ) by scaleFactor");
  declareProperty("MinDSpacing", 0.0, "Minimum d-spacing (Angstroms)");
  declareProperty("MinWavelength", 0.0, "Minimum wavelength (Angstroms)");
  declareProperty("MaxWavelength", EMPTY_DBL(), "Maximum wavelength (Angstroms)");
  std::vector<std::string> histoTypes{"Bank", "RunNumber", "Both Bank and RunNumber"};
  declareProperty("SortFilesBy", histoTypes[0], std::make_shared<StringListValidator>(histoTypes),
                  "Sort into files by bank(default), run number or both.");
  declareProperty("MinIsigI", EMPTY_DBL(), mustBePositive, "The minimum I/sig(I) ratio");
  declareProperty("WidthBorder", EMPTY_INT(), "Width of border of detectors");
  declareProperty("MinIntensity", EMPTY_DBL(), mustBePositive, "The minimum Intensity");
  declareProperty("UseDetScale", false,
                  "Scale intensity and sigI by scale "
                  "factor of detector if set in "
                  "SetDetScale.\n"
                  "If false, no change (default).");
  declareProperty(std::make_unique<ArrayProperty<std::string>>("EliminateBankNumbers", Direction::Input),
                  "Comma deliminated string of bank numbers to exclude for example 1,2,5");
  declareProperty("LaueScaleFormat", false, "New format for Lauescale");

  declareProperty("CrystalSystem", m_typeList[0], std::make_shared<Kernel::StringListValidator>(m_typeList),
                  "The conventional cell type to use");

  declareProperty("Centering", m_centeringList[0], std::make_shared<Kernel::StringListValidator>(m_centeringList),
                  "The centering for the conventional cell");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveLauenorm::exec() {

  std::string filename = getProperty("Filename");
  Poco::Path path(filename);
  std::string basename = path.getBaseName(); // Filename minus extension
  ws = getProperty("InputWorkspace");
  double scaleFactor = getProperty("ScalePeaks");
  double dMin = getProperty("MinDSpacing");
  double wlMin = getProperty("MinWavelength");
  double wlMax = getProperty("MaxWavelength");
  std::string type = getProperty("SortFilesBy");
  double minIsigI = getProperty("MinIsigI");
  double minIntensity = getProperty("MinIntensity");
  int widthBorder = getProperty("WidthBorder");
  bool newFormat = getProperty("LaueScaleFormat");
  std::string cellType = getProperty("CrystalSystem");
  auto iter = std::find(m_typeList.begin(), m_typeList.end(), cellType);
  auto cellNo = 1 + std::distance(m_typeList.begin(), iter);
  std::string cent = getProperty("Centering");
  auto iter2 = std::find(m_centeringList.begin(), m_centeringList.end(), cent);
  auto centerNo = 1 + std::distance(m_centeringList.begin(), iter2);
  // sequenceNo and run number
  int sequenceNo = 0;
  int oldSequence = -1;

  std::fstream out;
  std::ostringstream ss;

  // We must sort the peaks
  std::vector<std::pair<std::string, bool>> criteria;
  if (type.compare(0, 2, "Ba") == 0)
    criteria.emplace_back("BankName", true);
  else if (type.compare(0, 2, "Ru") == 0)
    criteria.emplace_back("RunNumber", true);
  else {
    criteria.emplace_back("RunNumber", true);
    criteria.emplace_back("BankName", true);
  }
  criteria.emplace_back("h", true);
  criteria.emplace_back("k", true);
  criteria.emplace_back("l", true);
  ws->sort(criteria);

  std::vector<Peak> peaks = ws->getPeaks();

  // ============================== Save all Peaks
  // =========================================
  // HKL is flipped by -1 due to different q convention in ISAW vs mantid.
  // Default for kf-ki has -q
  double qSign = -1.0;
  std::string convention = ConfigService::Instance().getString("Q.convention");
  if (convention == "Crystallography")
    qSign = 1.0;
  // scaleDet scales intensity and sigI for detector banks
  bool scaleDet = getProperty("UseDetScale");
  auto inst = ws->getInstrument();
  OrientedLattice lattice;
  if (newFormat) {
    type = "Both Bank and RunNumber";
    if (!ws->sample().hasOrientedLattice()) {

      const std::string fft("FindUBUsingIndexedPeaks");
      auto findUB = createChildAlgorithm(fft);
      findUB->initialize();
      findUB->setProperty<PeaksWorkspace_sptr>("PeaksWorkspace", ws);
      findUB->executeAsChildAlg();

      if (!ws->sample().hasOrientedLattice()) {
        g_log.notice(std::string("Could not find UB for ") + std::string(ws->getName()));
        throw std::invalid_argument(std::string("Could not find UB for ") + std::string(ws->getName()));
      }
    }
    lattice = ws->sample().getOrientedLattice();
  }
  // Count peaks
  std::vector<int> numPeaks;
  int count = 0;
  std::vector<double> maxLamVec;
  std::vector<double> minLamVec;
  std::vector<double> sumLamVec;
  std::vector<double> minDVec;
  double maxLam = 0;
  double minLam = EMPTY_DBL();
  double sumLam = 0;
  double minD = EMPTY_DBL();
  for (int wi = 0; wi < ws->getNumberPeaks(); wi++) {

    const Peak &p = peaks[wi];
    double intensity = p.getIntensity();
    double sigI = p.getSigmaIntensity();
    if (intensity == 0.0 || !(std::isfinite(sigI)))
      continue;
    if (minIsigI != EMPTY_DBL() && intensity < std::abs(minIsigI * sigI))
      continue;
    int sequence = p.getRunNumber();
    std::string bankName = p.getBankName();
    int nCols, nRows;
    sizeBanks(bankName, nCols, nRows);
    if (widthBorder != EMPTY_INT() && (p.getCol() < widthBorder || p.getRow() < widthBorder ||
                                       p.getCol() > (nCols - widthBorder) || p.getRow() > (nRows - widthBorder)))
      continue;
    // Take out the "bank" part of the bank name and convert to an int
    bankName.erase(remove_if(bankName.begin(), bankName.end(), std::not_fn(::isdigit)), bankName.end());
    if (type.compare(0, 2, "Ba") == 0) {
      Strings::convert(bankName, sequence);
    }
    // Do not use peaks from these banks
    std::vector<std::string> notBanks = getProperty("EliminateBankNumbers");
    if (std::find(notBanks.begin(), notBanks.end(), bankName) != notBanks.end())
      continue;
    if (scaleDet) {
      if (inst->hasParameter("detScale" + bankName)) {
        double correc = static_cast<double>(inst->getNumberParameter("detScale" + bankName)[0]);
        intensity *= correc;
        sigI *= correc;
      }
    }
    if (minIntensity != EMPTY_DBL() && intensity < minIntensity)
      continue;
    double lambda = p.getWavelength();
    double dsp = p.getDSpacing();
    if (dsp < dMin || lambda < wlMin || (wlMax != EMPTY_DBL() && lambda > wlMax))
      continue;
    if (p.getH() == 0 && p.getK() == 0 && p.getL() == 0)
      continue;

    if (sequence != oldSequence) {
      oldSequence = sequence;
      numPeaks.emplace_back(count);
      maxLamVec.emplace_back(maxLam);
      minLamVec.emplace_back(minLam);
      sumLamVec.emplace_back(sumLam);
      minDVec.emplace_back(minD);
      count = 0;
      maxLam = 0;
      minLam = EMPTY_DBL();
      sumLam = 0;
      minD = EMPTY_DBL();
    }
    count++;
    if (lambda < minLam)
      minLam = lambda;
    if (lambda > maxLam)
      maxLam = lambda;
    if (dsp < minD)
      minD = dsp;
    sumLam += lambda;
  }
  numPeaks.emplace_back(count);
  maxLamVec.emplace_back(maxLam);
  minLamVec.emplace_back(minLam);
  sumLamVec.emplace_back(sumLam);
  minDVec.emplace_back(minD);
  oldSequence = -1;
  // Go through each peak at this run / bank
  for (int wi = 0; wi < ws->getNumberPeaks(); wi++) {

    const Peak &p = peaks[wi];
    double intensity = p.getIntensity();
    double sigI = p.getSigmaIntensity();
    if (intensity == 0.0 || !(std::isfinite(sigI)))
      continue;
    if (minIsigI != EMPTY_DBL() && intensity < std::abs(minIsigI * sigI))
      continue;
    int sequence = p.getRunNumber();
    std::string bankName = p.getBankName();
    int nCols, nRows;
    sizeBanks(bankName, nCols, nRows);
    if (widthBorder != EMPTY_INT() && (p.getCol() < widthBorder || p.getRow() < widthBorder ||
                                       p.getCol() > (nCols - widthBorder) || p.getRow() > (nRows - widthBorder)))
      continue;
    // Take out the "bank" part of the bank name and convert to an int
    bankName.erase(remove_if(bankName.begin(), bankName.end(), std::not_fn(::isdigit)), bankName.end());
    if (type.compare(0, 2, "Ba") == 0) {
      Strings::convert(bankName, sequence);
    }
    // Do not use peaks from these banks
    std::vector<std::string> notBanks = getProperty("EliminateBankNumbers");
    if (std::find(notBanks.begin(), notBanks.end(), bankName) != notBanks.end())
      continue;
    if (scaleDet) {
      if (inst->hasParameter("detScale" + bankName)) {
        double correc = static_cast<double>(inst->getNumberParameter("detScale" + bankName)[0]);
        intensity *= correc;
        sigI *= correc;
      }
    }
    if (minIntensity != EMPTY_DBL() && intensity < minIntensity)
      continue;
    // Two-theta = polar angle = scattering angle = between +Z vector and the
    // scattered beam
    double scattering = p.getScattering();
    double lambda = p.getWavelength();
    double dsp = p.getDSpacing();
    if (dsp < dMin || lambda < wlMin || (wlMax != EMPTY_DBL() && lambda > wlMax))
      continue;
    // This can be bank number of run number depending on
    if (sequence != oldSequence) {
      oldSequence = sequence;
      if (newFormat) {
        out << "END-OF-REFLECTION-DATA\n";
        out << "HARMONICS DATA    0 REFLECTIONS\n";
        out << "END-OF-FILE\n";
      }
      out.flush();
      out.close();
      sequenceNo++;
      ss.str("");
      ss.clear();
      ss << std::setw(3) << std::setfill('0') << sequenceNo;

      // Chop off filename
      path.makeParent();
      path.append(basename + ss.str());
      if (newFormat)
        path.setExtension("geasc");
      Poco::File fileobj(path);
      out.open(path.toString().c_str(), std::ios::out);
      if (newFormat) {
        out << "TITL\n";
        out << basename << "\n";
        out << "CRYS " << basename.substr(0, 6) << "\n";
        out << "FIDX     1.00000     1.00000     1.00000     1.00000     "
               "1.00000     1.00000\n";
        out << "FIDY     1.00000     1.00000     1.00000     1.00000     "
               "1.00000     1.00000\n";
        out << "OMEG     1.00000     1.00000     1.00000     1.00000     "
               "1.00000     1.00000\n";
        out << "CELL " << std::setw(11) << std::setprecision(4) << 1.0 / lattice.a() << std::setw(12)
            << std::setprecision(4) << 1.0 / lattice.b() << std::setw(12) << std::setprecision(4) << 1.0 / lattice.c()
            << std::setw(9) << boost::math::iround(lattice.alpha()) << std::setw(9)
            << boost::math::iround(lattice.beta()) << std::setw(9) << boost::math::iround(lattice.gamma()) << '\n';

        out << "SYST    " << cellNo << "   " << centerNo << "   1   3"
            << "\n";
        out << "RAST      0.050"
            << "\n";
        out << "IBOX   1  1   1   1   1"
            << "\n";
        Goniometer gon(p.getGoniometerMatrix());
        std::vector<double> angles = gon.getEulerAngles("yzy");

        double phi = angles[2];
        double chi = angles[1];
        double omega = angles[0];

        out << "PHIS " << std::setw(11) << std::setprecision(4) << phi << std::setw(12) << std::setprecision(4) << chi
            << std::setw(12) << std::setprecision(4) << omega << "\n";
        out << "LAMS      ";

        out << std::setprecision(1) << std::fixed << sumLamVec[sequenceNo] / numPeaks[sequenceNo] << " "
            << minLamVec[sequenceNo] << " " << maxLamVec[sequenceNo] << "\n";

        out << "DMIN      ";
        out << std::setprecision(2) << std::fixed << minDVec[sequenceNo] << "\n";

        // distance from sample to detector (use first pixel) in mm
        double L2 = 500.0;
        out << "RADI     " << std::setprecision(0) << std::fixed << L2 << "\n";
        out << "SPIN      0.000"
            << "\n";
        out << "XC_S     0.00000     0.00000     0.00000     0.00000     "
               "0.00000     0.00000\n";
        out << "YC_S     0.00000     0.00000     0.00000     0.00000     "
               "0.00000     0.00000\n";
        out << "WC_S     0.00000     0.00000     0.00000     0.00000     "
               "0.00000     0.00000\n";
        out << "DELT       0.0000"
            << "\n";
        out << "TWIS    0.00000     0.00000     0.00000     0.00000     "
               "0.00000     0.00000 \n";
        out << "TILT    0.00000     0.00000     0.00000     0.00000     "
               "0.00000     0.00000 \n";
        out << "BULG    0.00000     0.00000     0.00000     0.00000     "
               "0.00000     0.00000 \n";
        out << "CTOF     " << L2 << "\n";
        out << "YSCA     1.00000     1.00000     1.00000     1.00000     "
               "1.00000     1.00000\n";
        out << "CRAT     1.00000     1.00000     1.00000     1.00000     "
               "1.00000     1.00000\n";
        out << "MINI          ";
        if (minIntensity != EMPTY_DBL()) {
          out << minIntensity << "\n";
        } else {
          out << "0.0\n";
        }
        out << "MULT  ";
        out << numPeaks[sequenceNo]
            << "     0      0      0      0      0      0      0      "
               "0      0\n";
        out << "      0      0      0      0      0      0      0      0      "
               "0      0\n";
        out << "      0 \n";
        out << "LAMH  " << numPeaks[sequenceNo]
            << "     0      0      0      0      0      0      0      0      "
               "0\n";
        out << "      0      0      0      0      0      0\n";
        out << "VERS  1"
            << "\n";
        out << "PACK        0"
            << "\n";
        out << "NSPT   " << numPeaks[sequenceNo] << "      0      0      0      0"
            << "\n";
        out << "NODH " << numPeaks[sequenceNo] << "    0      0      0      0      0      0      0      0      0\n"
            << "      0      0\n";
        out << "INTF        0"
            << "\n";
        out << "REFLECTION DATA   " << numPeaks[sequenceNo] << " REFLECTIONS"
            << "\n";
      }
    }
    // h k l lambda theta intensity and  sig(intensity)  in format
    // (3I5,2F10.5,2I10)
    // HKL is flipped by -1 due to different q convention in ISAW vs mantid.
    // unless Crystallography convention
    if (p.getH() == 0 && p.getK() == 0 && p.getL() == 0)
      continue;
    out << std::setw(5) << Utils::round(qSign * p.getH()) << std::setw(5) << Utils::round(qSign * p.getK())
        << std::setw(5) << Utils::round(qSign * p.getL());
    if (newFormat) {
      // Convert to mm from centre
      out << std::setw(10) << std::fixed << std::setprecision(5) << (p.getCol() - 127.5) * 150.0 / 256.0;
      out << std::setw(10) << std::fixed << std::setprecision(5) << (p.getRow() - 127.5) * 150.0 / 256.0 << "\n";
    }
    out << std::setw(10) << std::fixed << std::setprecision(5) << lambda;
    if (newFormat) {
      // mult nodal ovlp close h2 k2 l2 nidx lambda2 ipoint
      out << " 1 0 0 0 0 0 0 0 0.0 0 ";
      // Dmin threshold squared for next harmonic
      out << std::setw(10) << std::fixed << std::setprecision(5) << dsp * dsp * 0.25 << "\n";
    } else {
      // Assume that want theta not two-theta
      out << std::setw(10) << std::fixed << std::setprecision(5) << 0.5 * scattering;
    }

    // SHELX can read data without the space between the l and intensity
    if (p.getDetectorID() != -1) {
      double ckIntensity = scaleFactor * intensity;
      if (ckIntensity > 999999999.985)
        g_log.warning() << "Scaled intensity, " << ckIntensity << " is too large for format.  Decrease ScalePeaks.\n";
      out << std::setw(10) << Utils::round(ckIntensity);
      if (newFormat) {
        // mult nodal ovlp close h2 k2 l2 nidx lambda2 ipoint
        out << " -9999 -9999 -9999 -9999 -9999 \n";
      }

      out << std::setw(10) << Utils::round(scaleFactor * sigI);
      if (newFormat) {
        // mult nodal ovlp close h2 k2 l2 nidx lambda2 ipoint
        out << " -9999 -9999 -9999 -9999 -9999 \n";
        out << std::setw(10) << Utils::round(ckIntensity);
        out << " -9999 -9999 -9999 -9999 -9999 \n";
        out << std::setw(10) << Utils::round(scaleFactor * sigI);
        out << " -9999 -9999 -9999 -9999 -9999 * ";
      }
    } else {
      // This is data from LoadLauenorm which is already corrected
      out << std::setw(10) << Utils::round(intensity);
      if (newFormat) {
        // 5 more films (dummy)
        out << " -9999 -9999 -9999 -9999 -9999 \n";
      }
      out << std::setw(10) << Utils::round(sigI);
      if (newFormat) {
        // 5 more films (dummy)
        out << " -9999 -9999 -9999 -9999 -9999 \n";
        out << std::setw(10) << Utils::round(intensity);
        out << " -9999 -9999 -9999 -9999 -9999 \n";
        out << std::setw(10) << Utils::round(sigI);
        out << " -9999 -9999 -9999 -9999 -9999 * ";
      }
    }

    out << '\n';
  }
  if (newFormat) {
    out << "END-OF-REFLECTION-DATA\n";
    out << "HARMONICS DATA    0 REFLECTIONS\n";
    out << "END-OF-FILE\n";
  }
  out.flush();
  out.close();
}
void SaveLauenorm::sizeBanks(const std::string &bankName, int &nCols, int &nRows) {
  if (bankName == "None")
    return;
  std::shared_ptr<const IComponent> parent = ws->getInstrument()->getComponentByName(bankName);
  if (!parent)
    return;
  if (parent->type() == "RectangularDetector") {
    std::shared_ptr<const RectangularDetector> RDet = std::dynamic_pointer_cast<const RectangularDetector>(parent);

    nCols = RDet->xpixels();
    nRows = RDet->ypixels();
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    std::shared_ptr<const Geometry::ICompAssembly> asmb =
        std::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    std::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        std::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);
    nRows = static_cast<int>(grandchildren.size());
    nCols = static_cast<int>(children.size());
  }
}

} // namespace Mantid::Crystal
