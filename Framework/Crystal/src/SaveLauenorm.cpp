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
#include <fstream>
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
  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input PeaksWorkspace.");
  declareProperty(
      make_unique<API::FileProperty>("Filename", "", API::FileProperty::Save),
      "Select the directory and base name for the output files.");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("ScalePeaks", 1.0, mustBePositive,
                  "Multiply FSQ and sig(FSQ) by scaleFactor");
  declareProperty("MinDSpacing", 0.0, "Minimum d-spacing (Angstroms)");
  declareProperty("MinWavelength", 0.0, "Minimum wavelength (Angstroms)");
  declareProperty("MaxWavelength", EMPTY_DBL(),
                  "Maximum wavelength (Angstroms)");
  std::vector<std::string> histoTypes{"Bank", "RunNumber",
                                      "Both Bank and RunNumber"};
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
      Kernel::make_unique<ArrayProperty<std::string>>("EliminateBankNumbers",
                                                      Direction::Input),
      "Comma deliminated string of bank numbers to exclude for example 1,2,5");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveLauenorm::exec() {

  std::string filename = getProperty("Filename");
  ws = getProperty("InputWorkspace");
  double scaleFactor = getProperty("ScalePeaks");
  double dMin = getProperty("MinDSpacing");
  double wlMin = getProperty("MinWavelength");
  double wlMax = getProperty("MaxWavelength");
  std::string type = getProperty("SortFilesBy");
  double minIsigI = getProperty("MinIsigI");
  double minIntensity = getProperty("MinIntensity");
  int widthBorder = getProperty("WidthBorder");

  // sequenceNo and run number
  int sequenceNo = 0;
  int oldSequence = -1;

  std::fstream out;
  std::ostringstream ss;

  // We must sort the peaks
  std::vector<std::pair<std::string, bool>> criteria;
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
    if (type.compare(0, 2, "Ru") != 0) {
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

      Poco::Path path(filename);
      std::string basename = path.getBaseName(); // Filename minus extension
      // Chop off filename
      path.makeParent();
      path.append(basename + ss.str());
      Poco::File fileobj(path);
      out.open(path.toString().c_str(), std::ios::out);
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
    out << std::setw(10) << std::fixed << std::setprecision(5) << lambda;
    // Assume that want theta not two-theta
    out << std::setw(10) << std::fixed << std::setprecision(5)
        << 0.5 * scattering;

    // SHELX can read data without the space between the l and intensity
    if (p.getDetectorID() != -1) {
      double ckIntensity = scaleFactor * intensity;
      if (ckIntensity > 999999999.985)
        g_log.warning() << "Scaled intensity, " << ckIntensity
                        << " is too large for format.  Decrease ScalePeaks.\n";
      out << std::setw(10) << Utils::round(ckIntensity);

      out << std::setw(10) << Utils::round(scaleFactor * sigI);
    } else {
      // This is data from LoadLauenorm which is already corrected
      out << std::setw(10) << Utils::round(intensity);

      out << std::setw(10) << Utils::round(sigI);
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
