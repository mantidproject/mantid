#include "MantidAPI/FileProperty.h"
#include "MantidCrystal/SaveLauenorm.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ListValidator.h"
#include "MantidCrystal/AnvredCorrection.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Strings.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cmath>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::PhysicalConstants;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveLauenorm)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveLauenorm::init() {
  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace> >(
                      "InputWorkspace", "", Direction::Input),
                  "An input PeaksWorkspace.");
  declareProperty(
      make_unique<API::FileProperty>("Filename", "", API::FileProperty::Save),
      "Select the directory and base name for the output files.");
  auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
  mustBePositive->setLower(0.0);
  declareProperty("ScalePeaks", 1.0, mustBePositive,
                  "Multiply FSQ and sig(FSQ) by scaleFactor");
  declareProperty("MinDSpacing", 0.0, "Minimum d-spacing (Angstroms)");
  declareProperty("MinWavelength", 0.0, "Minimum wavelength (Angstroms)");
  declareProperty("MaxWavelength", EMPTY_DBL(),
                  "Maximum wavelength (Angstroms)");
  std::vector<std::string> histoTypes{ "Bank", "RunNumber",
                                       "Both Bank and RunNumber" };
  declareProperty("SortFilesBy", histoTypes[0],
                  boost::make_shared<StringListValidator>(histoTypes),
                  "Sort into files by bank(default), run number or both.");
  declareProperty("MinIsigI", EMPTY_DBL(), mustBePositive,
                  "The minimum I/sig(I) ratio");
  declareProperty("WidthBorder", EMPTY_INT(), "Width of border of detectors");
  declareProperty("MinIntensity", EMPTY_DBL(), mustBePositive,
                  "The minimum Intensity");
  declareProperty("UseDetScale", false, "Scale intensity and sigI by scale "
                                        "factor of detector if set in "
                                        "SetDetScale.\n"
                                        "If false, no change (default).");
  declareProperty(
      Kernel::make_unique<ArrayProperty<std::string> >("EliminateBankNumbers",
                                                       Direction::Input),
      "Comma deliminated string of bank numbers to exclude for example 1,2,5");
  declareProperty("LaueScaleFormat", false, "New format for Lauescale");
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

  // sequenceNo and run number
  int sequenceNo = 0;
  int oldSequence = -1;

  std::fstream out;
  std::ostringstream ss;

  // We must sort the peaks
  std::vector<std::pair<std::string, bool> > criteria;
  if (type.compare(0, 2, "Ba") == 0)
    criteria.push_back(std::pair<std::string, bool>("BankName", true));
  else if (type.compare(0, 2, "Ru") == 0)
    criteria.push_back(std::pair<std::string, bool>("RunNumber", true));
  else {
    criteria.push_back(std::pair<std::string, bool>("RunNumber", true));
    criteria.push_back(std::pair<std::string, bool>("BankName", true));
  }
  criteria.push_back(std::pair<std::string, bool>("h", true));
  criteria.push_back(std::pair<std::string, bool>("k", true));
  criteria.push_back(std::pair<std::string, bool>("l", true));
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
  type = "RunNumber";
  if (!ws->sample().hasOrientedLattice()) {

    const std::string fft("FindUBUsingIndexedPeaks");
    API::IAlgorithm_sptr findUB = this->createChildAlgorithm(fft);
    findUB->initialize();
    findUB->setProperty<PeaksWorkspace_sptr>("PeaksWorkspace", ws);
    findUB->executeAsChildAlg();

    if (!ws->sample().hasOrientedLattice()) {
      g_log.notice(std::string("Could not find UB for ") +
                   std::string(ws->getName()));
      throw std::invalid_argument(std::string("Could not find UB for ") +
                                  std::string(ws->getName()));
    }
  } 
  lattice = ws->sample().getOrientedLattice();
}

  // Go through each peak at this run / bank
  for (int wi = 0; wi < ws->getNumberPeaks(); wi++) {

    Peak &p = peaks[wi];
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
    if (widthBorder != EMPTY_INT() &&
        (p.getCol() < widthBorder || p.getRow() < widthBorder ||
         p.getCol() > (nCols - widthBorder) ||
         p.getRow() > (nRows - widthBorder)))
      continue;
    // Take out the "bank" part of the bank name and convert to an int
    bankName.erase(remove_if(bankName.begin(), bankName.end(),
                             not1(std::ptr_fun(::isdigit))),
                   bankName.end());
    if (type.compare(0, 2, "Ba") == 0) {
      Strings::convert(bankName, sequence);
    }
    // Do not use peaks from these banks
    std::vector<std::string> notBanks = getProperty("EliminateBankNumbers");
    if (std::find(notBanks.begin(), notBanks.end(), bankName) != notBanks.end())
      continue;
    if (scaleDet) {
      if (inst->hasParameter("detScale" + bankName)) {
        double correc = static_cast<double>(
            inst->getNumberParameter("detScale" + bankName)[0]);
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
    if (dsp < dMin || lambda < wlMin ||
        (minIsigI != EMPTY_DBL() && lambda > wlMax))
      continue;
    // This can be bank number of run number depending on
    if (sequence != oldSequence) {
      oldSequence = sequence;
      out.flush();
      out.close();
      sequenceNo++;
      ss.str("");
      ss.clear();
      ss << std::setw(3) << std::setfill('0') << sequenceNo;

      // Chop off filename
      path.makeParent();
      path.append(basename + ss.str());
      Poco::File fileobj(path);
      out.open(path.toString().c_str(), std::ios::out);
  std::cout << basename<<" open\n";
  if (newFormat) {
  out << "TITL\n";
  out << basename << "\n";
  out << "CRYS " << basename.substr(0,6) << "\n";
  out << "CELL "
      << std::setw(11) << std::setprecision(4) << lattice.a() << std::setw(12)
      << std::setprecision(4) << lattice.b() << std::setw(12) << std::setprecision(4)
      << lattice.c() << std::setw(12) << std::setprecision(4) << lattice.alpha()
      << std::setw(12) << std::setprecision(4) << lattice.beta() << std::setw(12)
      << std::setprecision(4) << lattice.gamma() 
      << "\n";
  out << "SYST    1   1   0   0" << "\n";
  out << "RAST      0.050" << "\n";
  out << "IBOX   1  1   1   1   1" << "\n";
        Goniometer gon(p.getGoniometerMatrix());
        std::vector<double> angles = gon.getEulerAngles("yzy");

        double phi = angles[2];
        double chi = angles[1];
        double omega = angles[0];

  out << "PHIS " 
      << std::setw(11) << std::setprecision(4) << phi << std::setw(12)
      << std::setprecision(4) << chi << std::setw(12) << std::setprecision(4)
      << omega << "\n";
  out << "LAMS      ";
  if (wlMax != EMPTY_DBL() ) {
  out << (wlMin+wlMax)/2 << "  " << wlMin << "  " << wlMax << "\n";
  }
  else {
  out << "0.0 0.0 0.0\n";
  }
  out << "DMIN      ";
  if (dMin != EMPTY_DBL() ) {
  out << dMin << "\n";
  }
  else {
  out << "0.0\n";
  }
  out << "RADI     59.000" << "\n";
  out << "SPIN      0.000" << "\n";
  out << "XC_S     0.23000    0.23000    0.24000    0.26000    0.26000    0.22000" << "\n";
  out << "YC_S     0.09000    0.11000    0.09000    0.11000    0.11000    0.09000" << "\n";
  out << "WC_S     3.79368    3.80879    3.78424    3.80775    3.80466    3.82321" << "\n";
  out << "DELT       0.4000" << "\n";
  out << "TWIS    -2.33700   -4.30406   -5.62749   -0.10663   -0.99198    3.61740" << "\n";
  out << "TILT    -4.79623    0.37310   -3.58647    0.53322   -1.06657   -3.66751" << "\n";
  out << "BULG    45.40230   56.51379   34.71940   31.53628   36.29953   42.53154" << "\n";
  out << "CTOF     61.067" << "\n";
  out << "YSCA     0.99748    0.99738    0.99696    0.99726    0.99758    0.99716" << "\n";
  out << "CRAT     0.99494    0.99720    1.00672    1.01395    1.01970    1.02644" << "\n";
  out << "MINI          " ;
  if (minIntensity != EMPTY_DBL() ) {
  out << minIntensity << "\n";
  }
  else {
  out << "0.0\n";
  }
  out << "MULT" << "\n";
  out << "   1670    175     40     12      8      5      0      0      0      0" << "\n";
  out << "      0      0      0      0      0      0      0      0      0      0" << "\n";
  out << "      0" << "\n";
  out << "LAMH" << "\n";
  out << "     401     383     389     391     282     158      89      53" << "\n";
  out << "      38      21      19      11      11       6       6       0" << "\n";
  out << "VERS    2" << "\n";
  out << "PACK        0" << "\n";
  out << "NSPT     1910     240       0       0    1605" << "\n";
  out << "NODH" << "\n";
  out << "       6      23      63     124     243     356" << "\n";
  out << "     571     775    1076    1312    1499    1605" << "\n";
  out << "INTF        0" << "\n";
  out << "REFLECTION DATA   " << ws->getNumberPeaks() << " REFLECTIONS" << "\n";
  }
}
    // h k l lambda theta intensity and  sig(intensity)  in format
    // (3I5,2F10.5,2I10)
    // HKL is flipped by -1 due to different q convention in ISAW vs mantid.
    // unless Crystallography convention
    if (p.getH() == 0 && p.getK() == 0 && p.getL() == 0)
      continue;
    out << std::setw(5) << Utils::round(qSign * p.getH()) << std::setw(5)
        << Utils::round(qSign * p.getK()) << std::setw(5)
        << Utils::round(qSign * p.getL());
    if (newFormat) {
      // Convert to mm from centre
      out << std::setw(10) << std::fixed << std::setprecision(5)
          << (p.getCol() - 127.5) * 150.0 / 256.0;
      out << std::setw(10) << std::fixed << std::setprecision(5)
          << (p.getRow() - 127.5) * 150.0 / 256.0;
    }
    out << std::setw(10) << std::fixed << std::setprecision(5) << lambda;
    if (newFormat) {
      // mult nodal ovlp close h2 k2 l2 nidx lambda2 ipoint
      out << " 1 0 0 0 0 0 0 0 0 0 ";
    }

    if (newFormat) {
      // Dmin threshold squared for next harmonic
      out << std::setw(10) << std::fixed << std::setprecision(5)
          << dsp * dsp * 0.25;
    } else {
      // Assume that want theta not two-theta
      out << std::setw(10) << std::fixed << std::setprecision(5)
          << 0.5 * scattering;
    }

    // SHELX can read data without the space between the l and intensity
    if (p.getDetectorID() != -1) {
      double ckIntensity = scaleFactor * intensity;
      if (ckIntensity > 999999999.985)
        g_log.warning() << "Scaled intensity, " << ckIntensity
                        << " is too large for format.  Decrease ScalePeaks.\n";
      out << std::setw(10) << Utils::round(ckIntensity);
      if (newFormat) {
        // mult nodal ovlp close h2 k2 l2 nidx lambda2 ipoint
        out << " 0 0 0 0 0 ";
      }

      out << std::setw(10) << Utils::round(scaleFactor * sigI);
      if (newFormat) {
        // mult nodal ovlp close h2 k2 l2 nidx lambda2 ipoint
        out << " 0 0 0 0 0 ";
        out << std::setw(10) << Utils::round(ckIntensity);
        out << " 0 0 0 0 0 ";
        out << std::setw(10) << Utils::round(scaleFactor * sigI);
        out << " 0 0 0 0 0 * ";
      }
    } else {
      // This is data from LoadLauenorm which is already corrected
      out << std::setw(10) << Utils::round(intensity);
      if (newFormat) {
        // 5 more films (dummy)
        out << " 0 0 0 0 0 ";
      }
      out << std::setw(10) << Utils::round(sigI);
      if (newFormat) {
        // 5 more films (dummy)
        out << " 0 0 0 0 0 ";
        out << std::setw(10) << Utils::round(intensity);
        out << " 0 0 0 0 0 ";
        out << std::setw(10) << Utils::round(sigI);
        out << " 0 0 0 0 0 * ";
      }
    }

    out << '\n';
  }

  out.flush();
  out.close();
}
void SaveLauenorm::sizeBanks(std::string bankName, int &nCols, int &nRows) {
  if (bankName == "None")
    return;
  boost::shared_ptr<const IComponent> parent =
      ws->getInstrument()->getComponentByName(bankName);
  if (!parent)
    return;
  if (parent->type() == "RectangularDetector") {
    boost::shared_ptr<const RectangularDetector> RDet =
        boost::dynamic_pointer_cast<const RectangularDetector>(parent);

    nCols = RDet->xpixels();
    nRows = RDet->ypixels();
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    boost::shared_ptr<const Geometry::ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    boost::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);
    nRows = static_cast<int>(grandchildren.size());
    nCols = static_cast<int>(children.size());
  }
}

} // namespace Mantid
} // namespace Crystal
