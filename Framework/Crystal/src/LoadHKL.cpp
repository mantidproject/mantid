// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/LoadHKL.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/AnvredCorrection.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Utils.h"
#include <fstream>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::PhysicalConstants;

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadHKL)

/** Initialize the algorithm's properties.
 */
void LoadHKL::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".hkl"),
                  "Path to an hkl file to save.");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace.");
}

/** Execute the algorithm.
 */
void LoadHKL::exec() {

  std::string filename = getPropertyValue("Filename");
  PeaksWorkspace_sptr ws(new PeaksWorkspace());

  std::fstream in;
  in.open(filename.c_str(), std::ios::in);

  // Anvred write from Art Schultz
  // hklFile.write('%4d%4d%4d%8.2f%8.2f%4d%8.4f%7.4f%7d%7d%7.4f%4d%9.5f%9.4f\n'
  //    % (H, K, L, FSQ, SIGFSQ, hstnum, WL, TBAR, CURHST, SEQNUM, TRANSMISSION,
  //    DN, TWOTH, DSP))
  // HKL is flipped by -1 due to different q convention in ISAW vs mantid.
  // Default for kf-ki has -q
  double qSign = -1.0;
  std::string convention = ConfigService::Instance().getString("Q.convention");
  if (convention == "Crystallography")
    qSign = 1.0;
  Instrument_sptr inst(new Geometry::Instrument);
  Detector *detector = new Detector("det1", -1, nullptr);
  detector->setPos(0.0, 0.0, 0.0);
  inst->add(detector); // This takes care of deletion
  inst->markAsDetector(detector);
  Mantid::Geometry::Component *sample = new Mantid::Geometry::Component("Sample");
  inst->add(sample); // This takes care of deletion
  inst->markAsSamplePos(sample);
  Mantid::Geometry::ObjComponent *source = new Mantid::Geometry::ObjComponent("Source");
  source->setPos(0.0, 0.0, -1.0);
  inst->add(source); // This takes care of deletion
  inst->markAsSource(source);

  std::string line;
  bool first = true;
  double mu1 = 0.0, mu2 = 0.0, wl1 = 0.0, wl2 = 0.0, sc1 = 0.0, astar1 = 0.0;
  do {
    getline(in, line);
    double h = 0.0, k = 0.0, l = 0.0, m = 0.0, n = 0.0, p = 0.0;
    bool cosines;
    bool modvec;
    if (line.length() < 95) {
      cosines = false;
      modvec = false;
    } else if (line.length() == 101) {
      cosines = false;
      modvec = true;
    } else if (line.length() == 157) {
      cosines = true;
      modvec = false;
    } else {
      cosines = true;
      modvec = true;
    }

    h = std::stod(line.substr(0, 4));
    k = std::stod(line.substr(4, 4));
    l = std::stod(line.substr(8, 4));
    if (h == 0.0 && k == 0.0 && l == 0.0) {
      break;
    }

    int offset = 0;

    if (modvec) {
      m = std::stod(line.substr(12, 4));
      n = std::stod(line.substr(16, 4));
      p = std::stod(line.substr(20, 4));
      offset += 12;
    }

    double Inti = std::stod(line.substr(12 + offset, 8));
    double SigI = std::stod(line.substr(20 + offset, 8));
    double wl = std::stod(line.substr(32 + offset, 8));
    double tbar, trans, scattering;
    int run, bank;
    int seqNum;

    tbar = std::stod(line.substr(40 + offset, 8)); // tbar

    if (cosines) {
      offset += 54;
    }

    run = std::stoi(line.substr(48 + offset, 6));
    seqNum = std::stoi(line.substr(54 + offset, 7));
    trans = std::stod(line.substr(61 + offset, 7)); // transmission
    bank = std::stoi(line.substr(68 + offset, 4));
    scattering = std::stod(line.substr(72 + offset, 9));

    if (first) {
      mu1 = -std::log(trans) / tbar;
      wl1 = wl / 1.8;
      sc1 = scattering;
      astar1 = 1.0 / trans;
      first = false;
    } else {
      mu2 = -std::log(trans) / tbar;
      wl2 = wl / 1.8;
    }

    Peak peak(inst, scattering, wl);
    peak.setHKL(qSign * h, qSign * k, qSign * l);
    peak.setIntHKL(V3D(h, k, l) * qSign);
    peak.setIntMNP(V3D(m, n, p) * qSign);
    peak.setIntensity(Inti);
    peak.setSigmaIntensity(SigI);
    peak.setRunNumber(run);
    peak.setPeakNumber(seqNum);
    std::ostringstream oss;
    oss << "bank" << bank;
    std::string bankName = oss.str();

    peak.setBankName(bankName);
    if (cosines) {
      int col = std::stoi(line.substr(142, 7));
      int row = std::stoi(line.substr(149, 7));
      peak.setCol(col);
      peak.setRow(row);
    }
    ws->addPeak(peak);

  } while (!in.eof());

  in.close();
  // solve 2 linear equations to find amu and smu
  double amu = (mu2 - 1.0 * mu1) / (-1.0 * wl1 + wl2);
  double smu = mu1 - wl1 * amu;
  double theta = sc1 * radtodeg * 0.5;

  // find roots of polynomial that describes
  double radius = 0.0;
  if (std::isfinite(astar1) && astar1 >= 1) {
    const size_t ndeg = sizeof pc / sizeof pc[0]; // order of poly
    double coefs[ndeg];
    std::vector<double> murs;
    murs.reserve(2);
    auto ith_lo = static_cast<size_t>(theta / 5.);
    for (size_t ith = ith_lo; ith < ith_lo + 2; ith++) {
      for (size_t ideg = 0; ideg < ndeg; ideg++) {
        coefs[ideg] = pc[ndeg - 1 - ideg][ith];
      }
      coefs[0] = coefs[0] - std::log(1.0 / astar1);
      double roots[2 * (ndeg - 1)];
      gsl_poly_complex_workspace *w = gsl_poly_complex_workspace_alloc(ndeg);
      gsl_poly_complex_solve(coefs, ndeg, w, roots);
      gsl_poly_complex_workspace_free(w);

      for (size_t irt = 0; irt < ndeg - 1; irt++) {
        if (roots[2 * irt] > 0 && roots[2 * irt] < 9 && std::abs(roots[2 * irt + 1]) < 1e-15) {
          murs.emplace_back(roots[2 * irt]); // real root in range 0 < muR < 9 cm^-1 (fitted in AnvredCorrection)
        }
      }
    }
    if (murs.size() == 2) {
      double frac = (theta - static_cast<double>(ith_lo) * 5.0) / 5.0;
      radius = (murs[0] * (1 - frac) + murs[1] * frac) / mu1;
      g_log.notice() << "LinearScatteringCoef = " << smu << " LinearAbsorptionCoef = " << amu << " Radius = " << radius
                     << " calculated from tbar and transmission of 2 peaks\n";
    } else {
      g_log.warning() << "Radius set to 0.0 cm - failed to find physical root to polynomial in AnvredCorrections\n";
    }
  } else {
    g_log.warning() << "Radius set to 0.0 cm - non-physical transmission supplied.\n";
  }

  API::Run &mrun = ws->mutableRun();
  mrun.addProperty<double>("Radius", radius, true);
  NeutronAtom neutron(0, 0, 0.0, 0.0, smu, 0.0, smu, amu);
  auto shape =
      std::shared_ptr<IObject>(ws->sample().getShape().cloneWithMaterial(Material("SetInLoadHKL", neutron, 1.0)));
  ws->mutableSample().setShape(shape);

  setProperty("OutputWorkspace", std::dynamic_pointer_cast<PeaksWorkspace>(ws));
}

} // namespace Mantid::Crystal
