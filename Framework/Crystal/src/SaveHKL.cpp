// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/SaveHKL.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/AnvredCorrection.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Utils.h"
#include <fstream>

#include <Poco/File.h>
#include <cmath>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Strings;
using namespace Mantid::API;
using namespace Mantid::PhysicalConstants;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveHKL)

/** Initialize the algorithm's properties.
 */
void SaveHKL::init() {
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
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
  declareProperty("PowerLambda", 4.0, "Power of lambda ");
  declareProperty(std::make_unique<FileProperty>("SpectraFile", "",
                                            API::FileProperty::OptionalLoad,
                                            ".dat"),
                  " Spectrum data read from a spectrum file.");

  declareProperty(
      std::make_unique<FileProperty>("Filename", "", FileProperty::Save, ".hkl"),
      "Path to an hkl file to save.");

  std::vector<std::string> histoTypes{"Bank", "RunNumber", ""};
  declareProperty("SortBy", histoTypes[2],
                  boost::make_shared<StringListValidator>(histoTypes),
                  "Sort the histograms by bank, run number or both (default).");
  declareProperty("MinIsigI", EMPTY_DBL(), mustBePositive,
                  "The minimum I/sig(I) ratio");
  declareProperty("WidthBorder", EMPTY_INT(), "Width of border of detectors");
  declareProperty("MinIntensity", EMPTY_DBL(), mustBePositive,
                  "The minimum Intensity");
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "OutputWorkspace", "SaveHKLOutput", Direction::Output),
                  "Output PeaksWorkspace");
  declareProperty(
      "HKLDecimalPlaces", EMPTY_INT(),
      "Number of decimal places for fractional HKL.  Default is integer HKL.");
  declareProperty(
      "DirectionCosines", false,
      "Extra columns (22 total) in file if true for direction cosines.\n"
      "If false, original 14 columns (default).");
  const std::vector<std::string> exts{".mat", ".ub", ".txt"};
  declareProperty(std::make_unique<FileProperty>(
                      "UBFilename", "", FileProperty::OptionalLoad, exts),
                  "Path to an ISAW-style UB matrix text file only needed for "
                  "DirectionCosines if workspace does not have lattice.");
}

/** Execute the algorithm.
 */
void SaveHKL::exec() {

  std::string filename = getPropertyValue("Filename");
  ws = getProperty("InputWorkspace");

  PeaksWorkspace_sptr peaksW = getProperty("OutputWorkspace");
  if (peaksW != ws)
    peaksW = ws->clone();
  auto inst = peaksW->getInstrument();
  std::vector<Peak> peaks = peaksW->getPeaks();
  double scaleFactor = getProperty("ScalePeaks");
  double dMin = getProperty("MinDSpacing");
  double wlMin = getProperty("MinWavelength");
  double wlMax = getProperty("MaxWavelength");
  std::string type = getProperty("SortBy");
  double minIsigI = getProperty("MinIsigI");
  double minIntensity = getProperty("MinIntensity");
  int widthBorder = getProperty("WidthBorder");
  int decimalHKL = getProperty("HKLDecimalPlaces");
  bool cosines = getProperty("DirectionCosines");
  Kernel::DblMatrix UB(3, 3);
  if (cosines) {
    if (peaksW->sample().hasOrientedLattice()) {
      UB = peaksW->sample().getOrientedLattice().getUB();
    } else {
      // Find OrientedLattice
      std::string fileUB = getProperty("UBFilename");
      // Open the file
      std::ifstream in(fileUB.c_str());
      if (!in)
        throw std::runtime_error(
            "A file containing the UB matrix must be input into UBFilename.");
      std::string s;
      double val;

      // Read the ISAW UB matrix
      for (size_t row = 0; row < 3; row++) {
        for (size_t col = 0; col < 3; col++) {
          s = getWord(in, true);
          if (!convert(s, val))
            throw std::runtime_error(
                "The string '" + s +
                "' in the file was not understood as a number.");
          UB[row][col] = val;
        }
        readToEndOfLine(in, true);
      }
    }
  }

  // Sequence and run number
  int bankSequence = 0;
  int runSequence = 0;
  // HKL is flipped by -1 due to different q convention in ISAW vs mantid.
  // Default for kf-ki has -q
  double qSign = -1.0;
  std::string convention = ConfigService::Instance().getString("Q.convention");
  if (convention == "Crystallography")
    qSign = 1.0;

  std::fstream out;
  bool append = getProperty("AppendFile");
  if (append && Poco::File(filename.c_str()).exists()) {
    IAlgorithm_sptr load_alg = createChildAlgorithm("LoadHKL");
    load_alg->setPropertyValue("Filename", filename);
    load_alg->setProperty("OutputWorkspace", "peaks");
    load_alg->executeAsChildAlg();
    // Get back the result
    DataObjects::PeaksWorkspace_sptr ws2 =
        load_alg->getProperty("OutputWorkspace");
    ws2->setInstrument(inst);

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

  // We cannot assume the peaks have bank type detector modules, so we have a
  // string to check this
  std::string bankPart = "?";
  // We must sort the peaks first by run, then bank #, and save the list of
  // workspace indices of it
  using bankMap_t = std::map<int, std::vector<size_t>>;
  using runMap_t = std::map<int, bankMap_t>;
  std::set<int> uniqueBanks;
  std::set<int> uniqueRuns;
  runMap_t runMap;
  for (size_t i = 0; i < peaks.size(); ++i) {
    Peak &p = peaks[i];
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
    if (type.compare(0, 2, "Ru") == 0)
      runMap[run][bank].push_back(i);
    else
      runMap[bank][run].push_back(i);
    // Track unique bank numbers
    uniqueBanks.insert(bank);
    uniqueRuns.insert(run);
  }

  bool correctPeaks = getProperty("ApplyAnvredCorrections");
  std::vector<std::vector<double>> spectra;
  std::vector<std::vector<double>> time;
  int iSpec = 0;
  m_smu = getProperty("LinearScatteringCoef"); // in 1/cm
  m_amu = getProperty("LinearAbsorptionCoef"); // in 1/cm
  m_radius = getProperty("Radius");            // in cm
  m_power_th = getProperty("PowerLambda");     // in cm
  const Material &sampleMaterial = peaksW->sample().getMaterial();
  if (sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda) !=
      0.0) {
    double rho = sampleMaterial.numberDensity();
    if (m_smu == EMPTY_DBL())
      m_smu =
          sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda) *
          rho;
    if (m_amu == EMPTY_DBL())
      m_amu = sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda) * rho;
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
  } else if (m_smu != EMPTY_DBL() &&
             m_amu != EMPTY_DBL()) // Save input in Sample
                                   // with wrong atomic
                                   // number and name
  {
    NeutronAtom neutron(0, 0, 0.0, 0.0, m_smu, 0.0, m_smu, m_amu);
    auto shape = boost::shared_ptr<IObject>(
        peaksW->sample().getShape().cloneWithMaterial(
            Material("SetInSaveHKL", neutron, 1.0)));
    peaksW->mutableSample().setShape(shape);
  }
  if (m_smu != EMPTY_DBL() && m_amu != EMPTY_DBL())
    g_log.notice() << "LinearScatteringCoef = " << m_smu << " 1/cm\n"
                   << "LinearAbsorptionCoef = " << m_amu << " 1/cm\n"
                   << "Radius = " << m_radius << " cm\n"
                   << "Power Lorentz corrections = " << m_power_th << " \n";
  API::Run &run = peaksW->mutableRun();
  if (run.hasProperty("Radius")) {
    if (m_radius == EMPTY_DBL())
      m_radius = run.getPropertyValueAsType<double>("Radius");
  } else {
    run.addProperty<double>("Radius", m_radius, true);
  }
  if (correctPeaks) {
    std::vector<double> spec(11);
    std::string STRING;
    std::ifstream infile;
    std::string spectraFile = getPropertyValue("SpectraFile");
    infile.open(spectraFile.c_str());
    if (infile.is_open()) {
      size_t a = 1;
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
          std::string temp;
          size_t a0 = 1;
          ss >> temp >> a0 >> temp >> a;
        }
      }
      infile.close();
    }
  }
  // ============================== Save all Peaks
  // =========================================
  std::set<size_t> banned;
  // Go through each peak at this run / bank

  // Go in order of run numbers
  runMap_t::iterator runMap_it;
  for (runMap_it = runMap.begin(); runMap_it != runMap.end(); ++runMap_it) {
    // Start of a new run
    // int run = runMap_it->first;
    bankMap_t &bankMap = runMap_it->second;

    bankMap_t::iterator bankMap_it;
    for (bankMap_it = bankMap.begin(); bankMap_it != bankMap.end();
         ++bankMap_it) {
      // Start of a new bank.
      // int bank = bankMap_it->first;
      std::vector<size_t> &ids = bankMap_it->second;

      // Go through each peak at this run / bank
      for (auto wi : ids) {

        Peak &p = peaks[wi];
        if (p.getIntensity() == 0.0 || !(std::isfinite(p.getIntensity())) ||
            !(std::isfinite(p.getSigmaIntensity()))) {
          banned.insert(wi);
          continue;
        }
        if (minIsigI != EMPTY_DBL() &&
            p.getIntensity() < std::abs(minIsigI * p.getSigmaIntensity())) {
          banned.insert(wi);
          continue;
        }
        if (minIntensity != EMPTY_DBL() && p.getIntensity() < minIntensity) {
          banned.insert(wi);
          continue;
        }
        const int runNumber = p.getRunNumber();
        int seqNum = p.getPeakNumber();
        int bank = 0;
        std::string bankName = p.getBankName();
        int nCols, nRows;
        sizeBanks(bankName, nCols, nRows);
        // peaks with detectorID=-1 are from LoadHKL
        if (widthBorder != EMPTY_INT() && p.getDetectorID() != -1 &&
            (p.getCol() < widthBorder || p.getRow() < widthBorder ||
             p.getCol() > (nCols - widthBorder) ||
             p.getRow() > (nRows - widthBorder))) {
          banned.insert(wi);
          continue;
        }
        // Take out the "bank" part of the bank name and convert to an int
        bankName.erase(remove_if(bankName.begin(), bankName.end(),
                                 not1(std::ptr_fun(::isdigit))),
                       bankName.end());
        Strings::convert(bankName, bank);

        double tbar = 0;

        // Two-theta = polar angle = scattering angle = between +Z vector and
        // the
        // scattered beam
        double scattering = p.getScattering();
        double lambda = p.getWavelength();
        double dsp = p.getDSpacing();
        if (dsp < dMin || lambda < wlMin || lambda > wlMax) {
          banned.insert(wi);
          continue;
        }
        double transmission = 0;
        if (m_smu != EMPTY_DBL() && m_amu != EMPTY_DBL()) {
          transmission = absor_sphere(scattering, lambda, tbar);
        }

        // Anvred write from Art Schultz/
        // hklFile.write('%4d%4d%4d%8.2f%8.2f%4d%8.4f%7.4f%7d%7d%7.4f%4d%9.5f%9.4f\n'
        //    % (H, K, L, FSQ, SIGFSQ, hstnum, WL, TBAR, CURHST, SEQNUM,
        //    TRANSMISSION, DN, TWOTH, DSP))
        if (p.getH() == 0 && p.getK() == 0 && p.getL() == 0) {
          banned.insert(wi);
          continue;
        }
        if (decimalHKL == EMPTY_INT())
          out << std::setw(4) << Utils::round(qSign * p.getH()) << std::setw(4)
              << Utils::round(qSign * p.getK()) << std::setw(4)
              << Utils::round(qSign * p.getL());
        else
          out << std::setw(5 + decimalHKL) << std::fixed
              << std::setprecision(decimalHKL) << -p.getH()
              << std::setw(5 + decimalHKL) << std::fixed
              << std::setprecision(decimalHKL) << -p.getK()
              << std::setw(5 + decimalHKL) << std::fixed
              << std::setprecision(decimalHKL) << -p.getL();
        double correc = scaleFactor;
        double instBkg = 0;
        double relSigSpect = 0.0;
        bankSequence = static_cast<int>(
            std::distance(uniqueBanks.begin(), uniqueBanks.find(bank)));
        runSequence = static_cast<int>(
            std::distance(uniqueRuns.begin(), uniqueRuns.find(runNumber)));
        if (correctPeaks) {
          // correct for the slant path throught the scintillator glass
          double mu = (9.614 * lambda) + 0.266; // mu for GS20 glass
          double depth = 0.2;
          double eff_center =
              1.0 - std::exp(-mu * depth); // efficiency at center of detector

          // Distance to center of detector
          boost::shared_ptr<const IComponent> det0 =
              inst->getComponentByName(p.getBankName());
          if (inst->getName() ==
              "CORELLI") // for Corelli with sixteenpack under bank
          {
            std::vector<Geometry::IComponent_const_sptr> children;
            boost::shared_ptr<const Geometry::ICompAssembly> asmb =
                boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(
                    inst->getComponentByName(p.getBankName()));
            asmb->getChildren(children, false);
            det0 = children[0];
          }
          IComponent_const_sptr sample = inst->getSample();
          double cosA = det0->getDistance(*sample) / p.getL2();
          double pathlength = depth / cosA;
          double eff_R = 1.0 - exp(-mu * pathlength); // efficiency at point R
          double sp_ratio = eff_center / eff_R; // slant path efficiency ratio
          double sinsqt = std::pow(lambda / (2.0 * dsp), 2);
          double wl4 = std::pow(lambda, m_power_th);
          double cmonx = 1.0;
          if (p.getMonitorCount() > 0)
            cmonx = 100e6 / p.getMonitorCount();
          double spect = spectrumCalc(p.getTOF(), iSpec, time, spectra, bank);
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
          double spect1 = spectrumCalc(one, iSpec, time, spectra, bank);
          relSigSpect = std::sqrt((1.0 / spect) + (1.0 / spect1));
          if (spect1 != 0.0) {
            spect /= spect1;
          } else {
            throw std::runtime_error(
                "Wavelength for normalizing to spectrum is out of range.");
          }
          correc = scaleFactor * sinsqt * cmonx * sp_ratio /
                   (wl4 * spect * transmission);

          if (inst->hasParameter("detScale" + bankName))
            correc *= static_cast<double>(
                inst->getNumberParameter("detScale" + bankName)[0]);

          // instrument background constant for sigma
          instBkg = 0. * 12.28 / cmonx * scaleFactor;
        }

        // SHELX can read data without the space between the l and intensity
        if (p.getDetectorID() != -1) {
          double ckIntensity = correc * p.getIntensity();
          double cksigI = std::sqrt(
              std::pow(correc * p.getSigmaIntensity(), 2) +
              std::pow(relSigSpect * correc * p.getIntensity(), 2) + instBkg);
          p.setIntensity(ckIntensity);
          p.setSigmaIntensity(cksigI);
          if (ckIntensity > 99999.985)
            g_log.warning()
                << "Scaled intensity, " << ckIntensity
                << " is too large for format.  Decrease ScalePeaks.\n";
          out << std::setw(8) << std::fixed << std::setprecision(2)
              << ckIntensity;

          out << std::setw(8) << std::fixed << std::setprecision(2) << cksigI;
        } else {
          // This is data from LoadHKL which is already corrected
          out << std::setw(8) << std::fixed << std::setprecision(2)
              << p.getIntensity();

          out << std::setw(8) << std::fixed << std::setprecision(2)
              << p.getSigmaIntensity();
        }
        if (type.compare(0, 2, "Ba") == 0)
          out << std::setw(4) << bankSequence + 1;
        else
          out << std::setw(4) << runSequence + 1;

        if (cosines) {
          out << std::setw(8) << std::fixed << std::setprecision(5) << lambda;
          out << std::setw(8) << std::fixed << std::setprecision(5) << tbar;
          Kernel::DblMatrix oriented = p.getGoniometerMatrix();
          Kernel::DblMatrix orientedIPNS(3, 3);
          V3D dir_cos_1, dir_cos_2;

          orientedIPNS[0][0] = oriented[0][0];
          orientedIPNS[0][1] = oriented[0][2];
          orientedIPNS[0][2] = oriented[0][1];
          orientedIPNS[1][0] = oriented[2][0];
          orientedIPNS[1][1] = oriented[2][2];
          orientedIPNS[1][2] = oriented[2][1];
          orientedIPNS[2][0] = oriented[1][0];
          orientedIPNS[2][1] = oriented[1][2];
          orientedIPNS[2][2] = oriented[1][1];
          Kernel::DblMatrix orientedUB = UB * orientedIPNS;
          double l2 = p.getL2();
          V3D R_reverse_incident = V3D(-l2, 0., 0.);
          V3D R_IPNS;
          double twoth = p.getScattering();
          // This is the scattered beam direction
          V3D dir = p.getDetPos() - inst->getSample()->getPos();

          // "Azimuthal" angle: project the scattered beam direction onto the XY
          // plane,
          // and calculate the angle between that and the +X axis (right-handed)
          double az = atan2(dir.Y(), dir.X());
          R_IPNS[0] = std::cos(twoth) * l2;
          R_IPNS[1] = std::cos(az) * std::sin(twoth) * l2;
          R_IPNS[2] = std::sin(az) * std::sin(twoth) * l2;

          for (int k = 0; k < 3; ++k) {
            V3D q_abc_star =
                V3D(orientedUB[k][0], orientedUB[k][1], orientedUB[k][2]);
            double length_q_abc_star = q_abc_star.norm();
            dir_cos_1[k] = R_reverse_incident.scalar_prod(q_abc_star) /
                           (l2 * length_q_abc_star);
            dir_cos_2[k] =
                R_IPNS.scalar_prod(q_abc_star) / (l2 * length_q_abc_star);
          }

          for (int k = 0; k < 3; ++k) {
            out << std::setw(9) << std::fixed << std::setprecision(5)
                << dir_cos_1[k];
            out << std::setw(9) << std::fixed << std::setprecision(5)
                << dir_cos_2[k];
          }

          out << std::setw(6) << runNumber;

          out << std::setw(6) << seqNum;

          out << std::setw(7) << std::fixed << std::setprecision(4)
              << transmission;

          out << std::setw(4) << std::right << bank;

          out << std::setw(9) << std::fixed << std::setprecision(5)
              << scattering; // two-theta scattering

          out << std::setw(8) << std::fixed << std::setprecision(4) << dsp;
          out << std::setw(7) << std::fixed << std::setprecision(2)
              << static_cast<double>(p.getCol());
          out << std::setw(7) << std::fixed << std::setprecision(2)
              << static_cast<double>(p.getRow());
        } else {
          out << std::setw(8) << std::fixed << std::setprecision(4) << lambda;

          out << std::setw(7) << std::fixed << std::setprecision(4) << tbar;

          out << std::setw(7) << runNumber;

          out << std::setw(7) << wi + 1;

          out << std::setw(7) << std::fixed << std::setprecision(4)
              << transmission;

          out << std::setw(4) << std::right << bank;

          out << std::setw(9) << std::fixed << std::setprecision(5)
              << scattering; // two-theta scattering

          out << std::setw(9) << std::fixed << std::setprecision(4) << dsp;
        }

        out << '\n';
      }
    }
  }
  if (decimalHKL == EMPTY_INT())
    out << std::setw(4) << 0 << std::setw(4) << 0 << std::setw(4) << 0;
  else
    out << std::setw(5 + decimalHKL) << std::fixed
        << std::setprecision(decimalHKL) << 0.0 << std::setw(5 + decimalHKL)
        << std::fixed << std::setprecision(decimalHKL) << 0.0
        << std::setw(5 + decimalHKL) << std::fixed
        << std::setprecision(decimalHKL) << 0.0;
  if (cosines) {
    out << "    0.00    0.00   0 0.00000 0.00000  0.00000  0.00000  0.00000"
           "  0.00000  0.00000  0.00000     0     0 0.0000   0  0.00000  "
           "0.0000   0.00   0.00";
  } else {
    out << "    0.00    0.00   0  0.0000 0.0000      0      0 0.0000 "
           "  0  0.00000   0.0000";
  }
  out << '\n';
  out.flush();
  out.close();
  // delete banned peaks
  std::vector<int> badPeaks;
  for (auto it = banned.crbegin(); it != banned.crend(); ++it) {
    badPeaks.push_back(static_cast<int>(*it));
  }
  peaksW->removePeaks(std::move(badPeaks));
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
p *       input are the smu (scattering) and amu (absorption at 1.8 ang.)
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

  mu = m_smu + (m_amu / 1.8f) * wl;

  mur = mu * m_radius;
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

  i = static_cast<int>(theta / 5.);
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
    tbar = -std::log(trans) / mu;

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
    spect = spectra[id][i - 1] + (TOF - time[id][i - 1]) /
                                     (time[id][i] - time[id][i - 1]) *
                                     (spectra[id][i] - spectra[id][i - 1]);
  }

  return spect;
}
void SaveHKL::sizeBanks(std::string bankName, int &nCols, int &nRows) {
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
    if (ws->getInstrument()->getName() ==
        "CORELLI") // for Corelli with sixteenpack under bank
    {
      std::vector<Geometry::IComponent_const_sptr> children;
      boost::shared_ptr<const Geometry::ICompAssembly> asmb =
          boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
      asmb->getChildren(children, false);
      parent = children[0];
    }
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

} // namespace Crystal
} // namespace Mantid
