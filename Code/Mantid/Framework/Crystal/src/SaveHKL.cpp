#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/SaveHKL.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ListValidator.h"
#include "MantidCrystal/AnvredCorrection.h"
#include <fstream>
#include "Poco/File.h"
#include "boost/assign.hpp"

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::PhysicalConstants;
using namespace boost::assign;
std::map<int, double> detScale = map_list_of(17, 1.092114823)(18, 0.869105443)(
    22, 1.081377685)(26, 1.055199489)(27, 1.070308725)(28, 0.886157884)(
    36, 1.112773972)(37, 1.012894506)(38, 1.049384146)(39, 0.890313805)(
    47, 1.068553893)(48, 0.900566426)(58, 0.911249203);

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveHKL)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveHKL::SaveHKL() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SaveHKL::~SaveHKL() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveHKL::init() {
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace", "",
                                                        Direction::Input),
                  "An input PeaksWorkspace.");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("ScalePeaks", 1.0, mustBePositive,
                  "Multiply FSQ and sig(FSQ) by scaleFactor");
  declareProperty("MinDSpacing", 0.0, "Minimum d-spacing (Angstroms)");
  declareProperty("MinWavelength", 0.0, "Minimum wavelength (Angstroms)");
  declareProperty("MaxWavelength", 100.0, "Maximum wavelength (Angstroms)");

  declareProperty("AppendFile", false,
                  "Append to file if true. Use same corrections as file.\n"
                  "If false, new file (default).");
  declareProperty("ApplyAnvredCorrections", false,
                  "Apply anvred corrections to peaks if true.\n"
                  "If false, no corrections during save (default).");
  declareProperty("LinearScatteringCoef", EMPTY_DBL(), mustBePositive,
                  "Linear scattering coefficient in 1/cm if not set with "
                  "SetSampleMaterial");
  declareProperty("LinearAbsorptionCoef", EMPTY_DBL(), mustBePositive,
                  "Linear absorption coefficient at 1.8 Angstroms in 1/cm if "
                  "not set with SetSampleMaterial");
  declareProperty("Radius", EMPTY_DBL(), mustBePositive,
                  "Radius of the sample in centimeters");
  declareProperty("PowerLambda", 4.0, "Power of lamda ");
  declareProperty(new FileProperty("SpectraFile", "",
                                   API::FileProperty::OptionalLoad, ".dat"),
                  " Spectrum data read from a spectrum file.");

  std::vector<std::string> exts;
  exts.push_back(".hkl");

  declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
                  "Path to an hkl file to save.");

  std::vector<std::string> histoTypes;
  histoTypes.push_back("Bank");
  histoTypes.push_back("RunNumber");
  histoTypes.push_back("");
  declareProperty("SortBy", histoTypes[2],
                  boost::make_shared<StringListValidator>(histoTypes),
                  "Sort the histograms by bank, run number or both (default).");
  declareProperty("MinIsigI", EMPTY_DBL(), mustBePositive,
                  "The minimum I/sig(I) ratio");
  declareProperty("WidthBorder", EMPTY_INT(), "Width of border of detectors");
  declareProperty("MinIntensity", EMPTY_DBL(), mustBePositive,
                  "The minimum Intensity");
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "SaveHKLOutput",
                                                        Direction::Output), "Output PeaksWorkspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveHKL::exec() {

  std::string filename = getPropertyValue("Filename");
  ws = getProperty("InputWorkspace");
  // HKL will be overwritten by equivalent HKL but never seen by user
  PeaksWorkspace_sptr peaksW = getProperty("OutputWorkspace");
  if (peaksW != ws)
    peaksW = ws->clone();
  double scaleFactor = getProperty("ScalePeaks");
  double dMin = getProperty("MinDSpacing");
  double wlMin = getProperty("MinWavelength");
  double wlMax = getProperty("MaxWavelength");
  std::string type = getProperty("SortBy");
  double minIsigI = getProperty("MinIsigI");
  double minIntensity = getProperty("MinIntensity");
  int widthBorder = getProperty("WidthBorder");

  // Sequence and run number
  int seqNum = 1;
  int bankSequence = 0;
  int runSequence = 0;
  int bankold = -1;
  int runold = -1;

  std::fstream out;
  bool append = getProperty("AppendFile");
  if (append && Poco::File(filename.c_str()).exists()) {
    IAlgorithm_sptr load_alg = createChildAlgorithm("LoadHKL");
    load_alg->setPropertyValue("Filename", filename.c_str());
    load_alg->setProperty("OutputWorkspace", "peaks");
    load_alg->executeAsChildAlg();
    // Get back the result
    DataObjects::PeaksWorkspace_sptr ws2 =
        load_alg->getProperty("OutputWorkspace");
    ws2->setInstrument(peaksW->getInstrument());

    IAlgorithm_sptr plus_alg = createChildAlgorithm("CombinePeaksWorkspaces");
    plus_alg->setProperty("LHSWorkspace", peaksW);
    plus_alg->setProperty("RHSWorkspace", ws2);
    plus_alg->executeAsChildAlg();
    // Get back the result
    peaksW = plus_alg->getProperty("OutputWorkspace");
    out.open(filename.c_str(), std::ios::out);
  } else {
    out.open(filename.c_str(), std::ios::out);
  }

  // We must sort the peaks by bank #
  std::vector<std::pair<std::string, bool>> criteria;
  if (type.compare(0, 2, "Ba") == 0)
    criteria.push_back(std::pair<std::string, bool>("BankName", true));
  else if (type.compare(0, 2, "Ru") == 0)
    criteria.push_back(std::pair<std::string, bool>("RunNumber", true));
  else
    criteria.push_back(std::pair<std::string, bool>("BankName", true));
  criteria.push_back(std::pair<std::string, bool>("h", true));
  criteria.push_back(std::pair<std::string, bool>("k", true));
  criteria.push_back(std::pair<std::string, bool>("l", true));
  peaksW->sort(criteria);

  bool correctPeaks = getProperty("ApplyAnvredCorrections");
  std::vector<Peak> &peaks = peaksW->getPeaks();
  std::vector<std::vector<double>> spectra;
  std::vector<std::vector<double>> time;
  int iSpec = 0;
  smu = getProperty("LinearScatteringCoef"); // in 1/cm
  amu = getProperty("LinearAbsorptionCoef"); // in 1/cm
  radius = getProperty("Radius");            // in cm
  power_th = getProperty("PowerLambda");     // in cm
  const Material &sampleMaterial = peaksW->sample().getMaterial();
  if (sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda) !=
      0.0) {
    double rho = sampleMaterial.numberDensity();
    if (smu == EMPTY_DBL())
      smu = sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda) *
            rho;
    if (amu == EMPTY_DBL())
      amu = sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda) * rho;
    g_log.notice() << "Sample number density = " << rho
                   << " atoms/Angstrom^3\n";
    g_log.notice() << "Cross sections for wavelength = "
                   << NeutronAtom::ReferenceLambda << "Angstroms\n"
                   << "    Coherent = " << sampleMaterial.cohScatterXSection()
                   << " barns\n"
                   << "    Incoherent = "
                   << sampleMaterial.incohScatterXSection() << " barns\n"
                   << "    Total = " << sampleMaterial.totalScatterXSection()
                   << " barns\n"
                   << "    Absorption = " << sampleMaterial.absorbXSection()
                   << " barns\n";
  } else if (smu != EMPTY_DBL() && amu != EMPTY_DBL()) // Save input in Sample
                                                       // with wrong atomic
                                                       // number and name
  {
    NeutronAtom neutron(static_cast<uint16_t>(EMPTY_DBL()),
                        static_cast<uint16_t>(0), 0.0, 0.0, smu, 0.0, smu, amu);
    Object shape = peaksW->sample().getShape(); // copy
    shape.setMaterial(Material("SetInSaveHKL", neutron, 1.0));
    peaksW->mutableSample().setShape(shape);
  }
  if (smu != EMPTY_DBL() && amu != EMPTY_DBL())
    g_log.notice() << "LinearScatteringCoef = " << smu << " 1/cm\n"
                   << "LinearAbsorptionCoef = " << amu << " 1/cm\n"
                   << "Radius = " << radius << " cm\n"
                   << "Power Lorentz corrections = " << power_th << " \n";
  API::Run &run = peaksW->mutableRun();
  if (run.hasProperty("Radius")) {
    Kernel::Property *prop = run.getProperty("Radius");
    if (radius == EMPTY_DBL())
      radius = boost::lexical_cast<double, std::string>(prop->value());
  } else {
    run.addProperty<double>("Radius", radius, true);
  }
  if (correctPeaks) {
    std::vector<double> spec(11);
    std::string STRING;
    std::ifstream infile;
    std::string spectraFile = getPropertyValue("SpectraFile");
    infile.open(spectraFile.c_str());
    if (infile.is_open()){
      size_t a = 0;
      if (iSpec == 1) {
        while (!infile.eof()) // To get you all the lines.
        {
          // Set up sizes. (HEIGHT x WIDTH)
          spectra.resize(a + 1);
          getline(infile, STRING); // Saves the line in STRING.
          infile >> spec[0] >> spec[1] >> spec[2] >> spec[3] >> spec[4] >>
              spec[5] >> spec[6] >> spec[7] >> spec[8] >> spec[9] >> spec[10];
          for (int i = 0; i < 11; i++)
            spectra[a].push_back(spec[i]);
          a++;
        }
      } else {
        for (int wi = 0; wi < 8; wi++)
          getline(infile, STRING); // Saves the line in STRING.
        while (!infile.eof())      // To get you all the lines.
        {
          time.resize(a + 1);
          spectra.resize(a + 1);
          getline(infile, STRING); // Saves the line in STRING.
          std::stringstream ss(STRING);
          if (STRING.find("Bank") == std::string::npos) {
            double time0, spectra0;
            ss >> time0 >> spectra0;
            time[a].push_back(time0);
            spectra[a].push_back(spectra0);

          } else {
            a++;
          }
        }
      }
      infile.close();
    }
  }
  // ============================== Save all Peaks
  // =========================================
  std::vector<int> banned;
  // Go through each peak at this run / bank
  for (int wi = 0; wi < peaksW->getNumberPeaks(); wi++) {

    Peak &p = peaks[wi];
    if (p.getIntensity() == 0.0 || boost::math::isnan(p.getIntensity()) ||
        boost::math::isnan(p.getSigmaIntensity())){
      banned.push_back(wi);
      continue;
    }
    if (minIsigI != EMPTY_DBL() &&
        p.getIntensity() < std::abs(minIsigI * p.getSigmaIntensity())){
      banned.push_back(wi);
      continue;
    }
    if (minIntensity != EMPTY_DBL() && p.getIntensity() < minIntensity){
      banned.push_back(wi);
      continue;
    }
    int run = p.getRunNumber();
    int bank = 0;
    std::string bankName = p.getBankName();
    int nCols, nRows;
    sizeBanks(bankName, nCols, nRows);
    if (widthBorder != EMPTY_INT() &&
        (p.getCol() < widthBorder || p.getRow() < widthBorder ||
         p.getCol() > (nCols - widthBorder) ||
         p.getRow() > (nRows - widthBorder))){
      banned.push_back(wi);
      continue;
    }
    // Take out the "bank" part of the bank name and convert to an int
    bankName.erase(remove_if(bankName.begin(), bankName.end(),
                             not1(std::ptr_fun(::isdigit))),
                   bankName.end());
    Strings::convert(bankName, bank);

    double tbar = 0;

    // Two-theta = polar angle = scattering angle = between +Z vector and the
    // scattered beam
    double scattering = p.getScattering();
    double lambda = p.getWavelength();
    double dsp = p.getDSpacing();
    if (dsp < dMin || lambda < wlMin || lambda > wlMax){
      banned.push_back(wi);
      continue;
    }
    double transmission = 0;
    if (smu != EMPTY_DBL() && amu != EMPTY_DBL()) {
      transmission = absor_sphere(scattering, lambda, tbar);
    }

    // Anvred write from Art Schultz/
    // hklFile.write('%4d%4d%4d%8.2f%8.2f%4d%8.4f%7.4f%7d%7d%7.4f%4d%9.5f%9.4f\n'
    //    % (H, K, L, FSQ, SIGFSQ, hstnum, WL, TBAR, CURHST, SEQNUM,
    //    TRANSMISSION, DN, TWOTH, DSP))
    // HKL is flipped by -1 due to different q convention in ISAW vs mantid.
    if (p.getH() == 0 && p.getK() == 0 && p.getL() == 0){
      banned.push_back(wi);
      continue;
    }
    out << std::setw(4) << Utils::round(-p.getH()) << std::setw(4)
        << Utils::round(-p.getK()) << std::setw(4) << Utils::round(-p.getL());
    double correc = scaleFactor;
    double relSigSpect = 0.0;
    if (bank != bankold)
      bankSequence++;
    if (run != runold)
      runSequence++;
    if (correctPeaks) {
      // correct for the slant path throught the scintillator glass
      double mu = (9.614 * lambda) + 0.266; // mu for GS20 glass
      double depth = 0.2;
      double eff_center =
          1.0 - std::exp(-mu * depth); // efficiency at center of detector
      IComponent_const_sptr sample = peaksW->getInstrument()->getSample();
      double cosA =
          peaksW->getInstrument()->getComponentByName(p.getBankName())->getDistance(
              *sample) /
          p.getL2();
      double pathlength = depth / cosA;
      double eff_R = 1.0 - exp(-mu * pathlength); // efficiency at point R
      double sp_ratio = eff_center / eff_R;       // slant path efficiency ratio

      double sinsqt = std::pow(lambda / (2.0 * dsp), 2);
      double wl4 = std::pow(lambda, power_th);
      double cmonx = 1.0;
      if (p.getMonitorCount() > 0)
        cmonx = 100e6 / p.getMonitorCount();
      double spect =
          spectrumCalc(p.getTOF(), iSpec, time, spectra, bankSequence);
      // Find spectra at wavelength of 1 for normalization
      std::vector<double> xdata(1, 1.0); // wl = 1
      std::vector<double> ydata;
      double theta2 = p.getScattering();
      double l1 = p.getL1();
      double l2 = p.getL2();
      Mantid::Kernel::Unit_sptr unit =
          UnitFactory::Instance().create("Wavelength");
      unit->toTOF(xdata, ydata, l1, l2, theta2, 0, 0.0, 0.0);
      double one = xdata[0];
      double spect1 = spectrumCalc(one, iSpec, time, spectra, bankSequence);
      relSigSpect = std::sqrt((1.0 / spect) + (1.0 / spect1));
      if (spect1 != 0.0) {
        spect /= spect1;
      } else {
        throw std::runtime_error(
            "Wavelength for normalizing to spectrum is out of range.");
      }
      correc = scaleFactor * sinsqt * cmonx * sp_ratio /
               (wl4 * spect * transmission);
      if (peaksW->getInstrument()->getName() == "TOPAZ" &&
          detScale.find(bank) != detScale.end())
        correc *= detScale[bank];
    }

    // SHELX can read data without the space between the l and intensity
    if (p.getDetectorID() != -1) {
      double ckIntensity = correc * p.getIntensity();
      double cksigI = std::sqrt(std::pow(correc * p.getSigmaIntensity(), 2) +
          std::pow(relSigSpect * correc * p.getIntensity(), 2));
      p.setIntensity(ckIntensity);
      p.setSigmaIntensity(cksigI);
      if (ckIntensity > 99999.985)
        g_log.warning() << "Scaled intensity, " << ckIntensity
                        << " is too large for format.  Decrease ScalePeaks.\n";
      out << std::setw(8) << std::fixed << std::setprecision(2) << ckIntensity;

      out << std::setw(8) << std::fixed << std::setprecision(2)
          << cksigI;
    } else {
      // This is data from LoadHKL which is already corrected
      out << std::setw(8) << std::fixed << std::setprecision(2)
          << p.getIntensity();

      out << std::setw(8) << std::fixed << std::setprecision(2)
          << p.getSigmaIntensity();
    }
    if (type.compare(0, 2, "Ba") == 0)
      out << std::setw(4) << bankSequence;
    else
      out << std::setw(4) << runSequence;

    out << std::setw(8) << std::fixed << std::setprecision(4) << lambda;

    out << std::setw(7) << std::fixed << std::setprecision(4) << tbar;

    out << std::setw(7) << run;

    out << std::setw(7) << wi + seqNum;

    out << std::setw(7) << std::fixed << std::setprecision(4) << transmission;

    out << std::setw(4) << std::right << bank;
    bankold = bank;
    runold = run;

    out << std::setw(9) << std::fixed << std::setprecision(5)
        << scattering; // two-theta scattering

    out << std::setw(9) << std::fixed << std::setprecision(4) << dsp;

    out << std::endl;
  }
  out << "   0   0   0    0.00    0.00   0  0.0000 0.0000      0      0 0.0000 "
         "  0";
  out << std::endl;

  out.flush();
  out.close();
  // delete banned peaks
  for (std::vector<int>::const_reverse_iterator it = banned.rbegin();
       it != banned.rend(); ++it) {
    peaksW->removePeak(*it);
  }
  setProperty("OutputWorkspace", peaksW);
}
/**
 *       function to calculate a spherical absorption correction
 *       and tbar. based on values in:
 *
 *       c. w. dwiggins, jr., acta cryst. a31, 395 (1975).
 *
 *       in this paper, a is the transmission and a* = 1/a is
 *       the absorption correction.
 *
 *       input are the smu (scattering) and amu (absorption at 1.8 ang.)
 *       linear absorption coefficients, the radius r of the sample
 *       the theta angle and wavelength.
 *       the absorption (absn) and tbar are returned.
 *
 *       a. j. schultz, june, 2008
 */
double SaveHKL::absor_sphere(double &twoth, double &wl, double &tbar) {
  int i;
  double mu, mur; // mu is the linear absorption coefficient,
  // r is the radius of the spherical sample.
  double theta, astar1, astar2, frac, astar;
  double trans;

  //  For each of the 19 theta values in dwiggins (theta = 0.0 to 90.0
  //  in steps of 5.0 deg.), the astar values vs.mur were fit to a third
  //  order polynomial in excel. these values are given in the static array
  //  pc[][]

  mu = smu + (amu / 1.8f) * wl;

  mur = mu * radius;
  if (mur < 0. || mur > 2.5) {
    std::ostringstream s;
    s << mur;
    throw std::runtime_error("muR is not in range of Dwiggins' table :" +
                             s.str());
  }

  theta = twoth * radtodeg_half;
  if (theta < 0. || theta > 90.) {
    std::ostringstream s;
    s << theta;
    throw std::runtime_error("theta is not in range of Dwiggins' table :" +
                             s.str());
  }

  //  using the polymial coefficients, calulate astar (= 1/transmission) at
  //  theta values below and above the actual theta value.

  i = (int)(theta / 5.);
  astar1 = pc[0][i] + mur * (pc[1][i] + mur * (pc[2][i] + pc[3][i] * mur));

  i = i + 1;
  astar2 = pc[0][i] + mur * (pc[1][i] + mur * (pc[2][i] + pc[3][i] * mur));

  //  do a linear interpolation between theta values.

  frac = theta -
         static_cast<double>(static_cast<int>(theta / 5.)) * 5.; // theta%5.
  frac = frac / 5.;

  astar = astar1 * (1 - frac) + astar2 * frac; // astar is the correction
  trans = 1.f / astar;                         // trans is the transmission
                                               // trans = exp(-mu*tbar)

  //  calculate tbar as defined by coppens.
  if (std::fabs(mu) < 1e-300)
    tbar = 0.0;
  else
    tbar = -(double)std::log(trans) / mu;

  return trans;
}

double SaveHKL::spectrumCalc(double TOF, int iSpec,
                             std::vector<std::vector<double>> time,
                             std::vector<std::vector<double>> spectra,
                             size_t id) {
  double spect = 0;
  if (iSpec == 1) {
    //"Calculate the spectrum using spectral coefficients for the GSAS Type 2
    // incident spectrum."
    double T = TOF / 1000.; // time-of-flight in milliseconds

    double c1 = spectra[id][0];
    double c2 = spectra[id][1];
    double c3 = spectra[id][2];
    double c4 = spectra[id][3];
    double c5 = spectra[id][4];
    double c6 = spectra[id][5];
    double c7 = spectra[id][6];
    double c8 = spectra[id][7];
    double c9 = spectra[id][8];
    double c10 = spectra[id][9];
    double c11 = spectra[id][10];

    spect = c1 + c2 * exp(-c3 / std::pow(T, 2)) / std::pow(T, 5) +
            c4 * exp(-c5 * std::pow(T, 2)) + c6 * exp(-c7 * std::pow(T, 3)) +
            c8 * exp(-c9 * std::pow(T, 4)) + c10 * exp(-c11 * std::pow(T, 5));
  } else {
    size_t i = 1;
    for (i = 1; i < spectra[id].size(); ++i)
      if (TOF < time[id][i])
        break;
    spect = spectra[id][i - 1] +
            (TOF - time[id][i - 1]) / (time[id][i] - time[id][i - 1]) *
                (spectra[id][i] - spectra[id][i - 1]);
  }

  return spect;
}
void SaveHKL::sizeBanks(std::string bankName, int &nCols, int &nRows) {
  if (bankName.compare("None") == 0)
    return;
  boost::shared_ptr<const IComponent> parent =
      ws->getInstrument()->getComponentByName(bankName);
  if (!parent)
    return;
  if (parent->type().compare("RectangularDetector") == 0) {
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
