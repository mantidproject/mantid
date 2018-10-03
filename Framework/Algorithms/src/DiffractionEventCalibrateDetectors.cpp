// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/DiffractionEventCalibrateDetectors.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAlgorithms/GSLFunctions.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/UnitFactory.h"

#include <Poco/File.h>
#include <cmath>
#include <fstream>
#include <numeric>
#include <sstream>

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(DiffractionEventCalibrateDetectors)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using Types::Core::DateAndTime;

/**
 * The gsl_costFunction is optimized by GSL simplex
 * @param v :: vector containing center position and rotations
 * @param params :: names of detector, workspace, and instrument
 */

static double gsl_costFunction(const gsl_vector *v, void *params) {
  double x, y, z, rotx, roty, rotz;
  std::string detname, inname, outname, peakOpt, rb_param, groupWSName;
  std::string *p = reinterpret_cast<std::string *>(params);
  detname = p[0];
  inname = p[1];
  outname = p[2];
  peakOpt = p[3];
  rb_param = p[4];
  groupWSName = p[5];
  x = gsl_vector_get(v, 0);
  y = gsl_vector_get(v, 1);
  z = gsl_vector_get(v, 2);
  rotx = gsl_vector_get(v, 3);
  roty = gsl_vector_get(v, 4);
  rotz = gsl_vector_get(v, 5);
  Mantid::Algorithms::DiffractionEventCalibrateDetectors u;
  return u.intensity(x, y, z, rotx, roty, rotz, detname, inname, outname,
                     peakOpt, rb_param, groupWSName);
}

/**
 * The movedetector function changes detector position and angles
 * @param x :: The shift along the X-axis
 * @param y :: The shift along the Y-axis
 * @param z :: The shift along the Z-axis
 * @param rotx :: The rotation around the X-axis
 * @param roty :: The rotation around the Y-axis
 * @param rotz :: The rotation around the Z-axis
 * @param detname :: The detector name
 * @param inputW :: The workspace
 */

void DiffractionEventCalibrateDetectors::movedetector(
    double x, double y, double z, double rotx, double roty, double rotz,
    std::string detname, EventWorkspace_sptr inputW) {

  IAlgorithm_sptr alg1 = createChildAlgorithm("MoveInstrumentComponent");
  alg1->setProperty<EventWorkspace_sptr>("Workspace", inputW);
  alg1->setPropertyValue("ComponentName", detname);
  // Move in cm for small shifts
  alg1->setProperty("X", x * 0.01);
  alg1->setProperty("Y", y * 0.01);
  alg1->setProperty("Z", z * 0.01);
  alg1->setPropertyValue("RelativePosition", "1");
  alg1->executeAsChildAlg();

  IAlgorithm_sptr algx = createChildAlgorithm("RotateInstrumentComponent");
  algx->setProperty<EventWorkspace_sptr>("Workspace", inputW);
  algx->setPropertyValue("ComponentName", detname);
  algx->setProperty("X", 1.0);
  algx->setProperty("Y", 0.0);
  algx->setProperty("Z", 0.0);
  algx->setProperty("Angle", rotx);
  algx->setPropertyValue("RelativeRotation", "1");
  algx->executeAsChildAlg();

  IAlgorithm_sptr algy = createChildAlgorithm("RotateInstrumentComponent");
  algy->setProperty<EventWorkspace_sptr>("Workspace", inputW);
  algy->setPropertyValue("ComponentName", detname);
  algy->setProperty("X", 0.0);
  algy->setProperty("Y", 1.0);
  algy->setProperty("Z", 0.0);
  algy->setProperty("Angle", roty);
  algy->setPropertyValue("RelativeRotation", "1");
  algy->executeAsChildAlg();

  IAlgorithm_sptr algz = createChildAlgorithm("RotateInstrumentComponent");
  algz->setProperty<EventWorkspace_sptr>("Workspace", inputW);
  algz->setPropertyValue("ComponentName", detname);
  algz->setProperty("X", 0.0);
  algz->setProperty("Y", 0.0);
  algz->setProperty("Z", 1.0);
  algz->setProperty("Angle", rotz);
  algz->setPropertyValue("RelativeRotation", "1");
  algz->executeAsChildAlg();
}
/**
 * The intensity function calculates the intensity as a function of detector
 * position and angles
 * @param x :: The shift along the X-axis
 * @param y :: The shift along the Y-axis
 * @param z :: The shift along the Z-axis
 * @param rotx :: The rotation around the X-axis
 * @param roty :: The rotation around the Y-axis
 * @param rotz :: The rotation around the Z-axis
 * @param detname :: The detector name
 * @param inname :: The workspace name
 * @param outname :: The workspace name
 * @param peakOpt :: Location of optimized peak
 * @param rb_param :: Bin boundary string
 * @param groupWSName :: GroupingWorkspace for this detector only.
 *  */

double DiffractionEventCalibrateDetectors::intensity(
    double x, double y, double z, double rotx, double roty, double rotz,
    std::string detname, std::string inname, std::string outname,
    std::string peakOpt, std::string rb_param, std::string groupWSName) {

  EventWorkspace_sptr inputW = boost::dynamic_pointer_cast<EventWorkspace>(
      AnalysisDataService::Instance().retrieve(inname));

  CPUTimer tim;

  movedetector(x, y, z, rotx, roty, rotz, detname, inputW);
  g_log.debug() << tim << " to movedetector()\n";

  IAlgorithm_sptr alg3 = createChildAlgorithm("ConvertUnits");
  alg3->setProperty<EventWorkspace_sptr>("InputWorkspace", inputW);
  alg3->setPropertyValue("OutputWorkspace", outname);
  alg3->setPropertyValue("Target", "dSpacing");
  alg3->executeAsChildAlg();
  MatrixWorkspace_sptr outputW = alg3->getProperty("OutputWorkspace");

  g_log.debug() << tim << " to ConvertUnits\n";

  IAlgorithm_sptr alg4 = createChildAlgorithm("DiffractionFocussing");
  alg4->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputW);
  alg4->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputW);
  alg4->setPropertyValue("GroupingFileName", "");
  alg4->setPropertyValue("GroupingWorkspace", groupWSName);
  alg4->executeAsChildAlg();
  outputW = alg4->getProperty("OutputWorkspace");

  // Remove file
  g_log.debug() << tim << " to DiffractionFocussing\n";

  IAlgorithm_sptr alg5 = createChildAlgorithm("Rebin");
  alg5->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputW);
  alg5->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputW);
  alg5->setPropertyValue("Params", rb_param);
  alg5->executeAsChildAlg();
  outputW = alg5->getProperty("OutputWorkspace");

  g_log.debug() << tim << " to Rebin\n";

  // Find point of peak centre
  const MantidVec &yValues = outputW->readY(0);
  auto it = std::max_element(yValues.begin(), yValues.end());
  double peakHeight = *it;
  if (peakHeight == 0)
    return -0.000;
  double peakLoc = outputW->readX(0)[it - yValues.begin()];

  IAlgorithm_sptr fit_alg;
  try {
    // set the ChildAlgorithm no to log as this will be run once per spectra
    fit_alg = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    g_log.error("Can't locate Fit algorithm");
    throw;
  }
  std::ostringstream fun_str;
  fun_str << "name=Gaussian,Height=" << peakHeight
          << ",Sigma=0.01,PeakCentre=" << peakLoc;
  fit_alg->setProperty("Function", fun_str.str());
  fit_alg->setProperty("InputWorkspace", outputW);
  fit_alg->setProperty("WorkspaceIndex", 0);
  fit_alg->setProperty("StartX", outputW->readX(0)[0]);
  fit_alg->setProperty("EndX", outputW->readX(0)[outputW->blocksize()]);
  fit_alg->setProperty("MaxIterations", 200);
  fit_alg->setProperty("Output", "fit");
  fit_alg->executeAsChildAlg();

  g_log.debug() << tim << " to Fit\n";

  std::vector<double> params; // = fit_alg->getProperty("Parameters");
  Mantid::API::IFunction_sptr fun_res = fit_alg->getProperty("Function");
  for (size_t i = 0; i < fun_res->nParams(); ++i) {
    params.push_back(fun_res->getParameter(i));
  }
  peakHeight = params[0];
  peakLoc = params[1];

  movedetector(-x, -y, -z, -rotx, -roty, -rotz, detname, inputW);

  g_log.debug() << tim << " to movedetector()\n";

  // Optimize C/peakheight + |peakLoc-peakOpt|  where C is scaled by number of
  // events
  EventWorkspace_const_sptr inputE =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputW);
  return (static_cast<int>(inputE->getNumberEvents()) / 1.e6) / peakHeight +
         std::fabs(peakLoc - boost::lexical_cast<double>(peakOpt));
}

/** Initialisation method
 */
void DiffractionEventCalibrateDetectors::init() {
  declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "The workspace containing the geometry to be calibrated.");

  declareProperty("Params", "",
                  "A comma separated list of first bin boundary, width, last "
                  "bin boundary. Optionally "
                  "this can be followed by a comma and more widths and last "
                  "boundary pairs. "
                  "Use bin boundaries close to peak you wish to maximize. "
                  "Negative width values indicate logarithmic binning.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  declareProperty(
      "MaxIterations", 10, mustBePositive,
      "Stop after this number of iterations if a good fit is not found");

  auto dblmustBePositive = boost::make_shared<BoundedValidator<double>>();
  declareProperty("LocationOfPeakToOptimize", 2.0308, dblmustBePositive,
                  "Optimize this location of peak by moving detectors");

  declareProperty(make_unique<API::FileProperty>(
                      "DetCalFilename", "", API::FileProperty::Save, ".DetCal"),
                  "The output filename of the ISAW DetCal file");

  declareProperty(
      make_unique<PropertyWithValue<std::string>>("BankName", "",
                                                  Direction::Input),
      "Optional: To only calibrate one bank. Any bank whose name does not "
      "match the given string will have no events.");

  // Disable default gsl error handler (which is to call abort!)
  gsl_set_error_handler_off();
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void DiffractionEventCalibrateDetectors::exec() {
  // Try to retrieve optional properties
  const int maxIterations = getProperty("MaxIterations");
  const double peakOpt = getProperty("LocationOfPeakToOptimize");

  // Get the input workspace
  EventWorkspace_sptr inputW = getProperty("InputWorkspace");

  // retrieve the properties
  const std::string rb_params = getProperty("Params");

  // Get some stuff from the input workspace
  // We make a copy of the instrument since we will be moving detectors in
  // `inputW` but want to access original positions (etc.) via `detList` below.
  const auto &dummyW = create<EventWorkspace>(*inputW, 1, inputW->binEdges(0));
  Instrument_const_sptr inst = dummyW->getInstrument();

  // Build a list of Rectangular Detectors
  std::vector<boost::shared_ptr<RectangularDetector>> detList;
  // --------- Loading only one bank ----------------------------------
  std::string onebank = getProperty("BankName");
  bool doOneBank = (!onebank.empty());
  for (int i = 0; i < inst->nelements(); i++) {
    boost::shared_ptr<RectangularDetector> det;
    boost::shared_ptr<ICompAssembly> assem;
    boost::shared_ptr<ICompAssembly> assem2;

    det = boost::dynamic_pointer_cast<RectangularDetector>((*inst)[i]);
    if (det) {
      if (det->getName() == onebank)
        detList.push_back(det);
      if (!doOneBank)
        detList.push_back(det);
    } else {
      // Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
      // We are not doing a full recursive search since that will be very long
      // for lots of pixels.
      assem = boost::dynamic_pointer_cast<ICompAssembly>((*inst)[i]);
      if (assem) {
        for (int j = 0; j < assem->nelements(); j++) {
          det = boost::dynamic_pointer_cast<RectangularDetector>((*assem)[j]);
          if (det) {
            if (det->getName() == onebank)
              detList.push_back(det);
            if (!doOneBank)
              detList.push_back(det);

          } else {
            // Also, look in the second sub-level for RectangularDetectors (e.g.
            // PG3).
            // We are not doing a full recursive search since that will be very
            // long for lots of pixels.
            assem2 = boost::dynamic_pointer_cast<ICompAssembly>((*assem)[j]);
            if (assem2) {
              for (int k = 0; k < assem2->nelements(); k++) {
                det = boost::dynamic_pointer_cast<RectangularDetector>(
                    (*assem2)[k]);
                if (det) {
                  if (det->getName() == onebank)
                    detList.push_back(det);
                  if (!doOneBank)
                    detList.push_back(det);
                }
              }
            }
          }
        }
      }
    }
  }

  // set-up minimizer

  std::string inname = getProperty("InputWorkspace");
  std::string outname = inname + "2"; // getProperty("OutputWorkspace");

  IAlgorithm_sptr algS = createChildAlgorithm("SortEvents");
  algS->setProperty("InputWorkspace", inputW);
  algS->setPropertyValue("SortBy", "X Value");
  algS->executeAsChildAlg();

  // Write DetCal File
  std::string filename = getProperty("DetCalFilename");
  std::fstream outfile;
  outfile.open(filename.c_str(), std::ios::out);

  if (detList.size() > 1) {
    outfile << "#\n";
    outfile << "#  Mantid Optimized .DetCal file for SNAP with TWO detector "
               "panels\n";
    outfile << "#  Old Panel, nominal size and distance at -90 degrees.\n";
    outfile << "#  New Panel, nominal size and distance at +90 degrees.\n";
    outfile << "#\n";
    outfile << "# Lengths are in centimeters.\n";
    outfile << "# Base and up give directions of unit vectors for a local\n";
    outfile << "# x,y coordinate system on the face of the detector.\n";
    outfile << "#\n";
    outfile << "# " << DateAndTime::getCurrentTime().toFormattedString("%c")
            << "\n";
    outfile << "#\n";
    outfile << "6         L1     T0_SHIFT\n";
    IComponent_const_sptr source = inst->getSource();
    IComponent_const_sptr sample = inst->getSample();
    outfile << "7  " << source->getDistance(*sample) * 100 << "            0\n";
    outfile << "4 DETNUM  NROWS  NCOLS  WIDTH   HEIGHT   DEPTH   DETD   "
               "CenterX   CenterY   CenterZ    BaseX    BaseY    BaseZ      "
               "UpX      UpY      UpZ\n";
  }

  Progress prog(this, 0.0, 1.0, detList.size());
  for (int det = 0; det < static_cast<int>(detList.size()); det++) {
    std::string par[6];
    par[0] = detList[det]->getName();
    par[1] = inname;
    par[2] = outname;
    std::ostringstream strpeakOpt;
    strpeakOpt << peakOpt;
    par[3] = strpeakOpt.str();
    par[4] = rb_params;

    // --- Create a GroupingWorkspace for this detector name ------
    CPUTimer tim;
    IAlgorithm_sptr alg2 =
        AlgorithmFactory::Instance().create("CreateGroupingWorkspace", 1);
    alg2->initialize();
    alg2->setProperty("InputWorkspace", inputW);
    alg2->setPropertyValue("GroupNames", detList[det]->getName());
    std::string groupWSName = "group_" + detList[det]->getName();
    alg2->setPropertyValue("OutputWorkspace", groupWSName);
    alg2->executeAsChildAlg();
    par[5] = groupWSName;
    std::cout << tim << " to CreateGroupingWorkspace\n";

    const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex;
    gsl_vector *ss, *x;
    gsl_multimin_function minex_func;

    // finally do the fitting

    int nopt = 6;
    int iter = 0;
    int status = 0;

    /* Starting point */
    x = gsl_vector_alloc(nopt);
    gsl_vector_set(x, 0, 0.0);
    gsl_vector_set(x, 1, 0.0);
    gsl_vector_set(x, 2, 0.0);
    gsl_vector_set(x, 3, 0.0);
    gsl_vector_set(x, 4, 0.0);
    gsl_vector_set(x, 5, 0.0);

    /* Set initial step sizes to 0.1 */
    ss = gsl_vector_alloc(nopt);
    gsl_vector_set_all(ss, 0.1);

    /* Initialize method and iterate */
    minex_func.n = nopt;
    minex_func.f = &Mantid::Algorithms::gsl_costFunction;
    minex_func.params = &par;

    gsl_multimin_fminimizer *s = gsl_multimin_fminimizer_alloc(T, nopt);
    gsl_multimin_fminimizer_set(s, &minex_func, x, ss);

    do {
      iter++;
      status = gsl_multimin_fminimizer_iterate(s);

      if (status)
        break;

      double size = gsl_multimin_fminimizer_size(s);
      status = gsl_multimin_test_size(size, 1e-2);

    } while (status == GSL_CONTINUE && iter < maxIterations &&
             s->fval != -0.000);

    // Output summary to log file
    if (s->fval != -0.000)
      movedetector(gsl_vector_get(s->x, 0), gsl_vector_get(s->x, 1),
                   gsl_vector_get(s->x, 2), gsl_vector_get(s->x, 3),
                   gsl_vector_get(s->x, 4), gsl_vector_get(s->x, 5), par[0],
                   getProperty("InputWorkspace"));
    else {
      gsl_vector_set(s->x, 0, 0.0);
      gsl_vector_set(s->x, 1, 0.0);
      gsl_vector_set(s->x, 2, 0.0);
      gsl_vector_set(s->x, 3, 0.0);
      gsl_vector_set(s->x, 4, 0.0);
      gsl_vector_set(s->x, 5, 0.0);
    }

    std::string reportOfDiffractionEventCalibrateDetectors =
        gsl_strerror(status);
    if (s->fval == -0.000)
      reportOfDiffractionEventCalibrateDetectors = "No events";

    g_log.information() << "Detector = " << det << "\n"
                        << "Method used = "
                        << "Simplex"
                        << "\n"
                        << "Iteration = " << iter << "\n"
                        << "Status = "
                        << reportOfDiffractionEventCalibrateDetectors << "\n"
                        << "Minimize PeakLoc-" << peakOpt << " = " << s->fval
                        << "\n";
    // Move in cm for small shifts
    g_log.information() << "Move (X)   = " << gsl_vector_get(s->x, 0) * 0.01
                        << "  \n";
    g_log.information() << "Move (Y)   = " << gsl_vector_get(s->x, 1) * 0.01
                        << "  \n";
    g_log.information() << "Move (Z)   = " << gsl_vector_get(s->x, 2) * 0.01
                        << "  \n";
    g_log.information() << "Rotate (X) = " << gsl_vector_get(s->x, 3) << "  \n";
    g_log.information() << "Rotate (Y) = " << gsl_vector_get(s->x, 4) << "  \n";
    g_log.information() << "Rotate (Z) = " << gsl_vector_get(s->x, 5) << "  \n";

    Kernel::V3D CalCenter =
        V3D(gsl_vector_get(s->x, 0) * 0.01, gsl_vector_get(s->x, 1) * 0.01,
            gsl_vector_get(s->x, 2) * 0.01);
    Kernel::V3D Center = detList[det]->getPos() + CalCenter;
    int pixmax = detList[det]->xpixels() - 1;
    int pixmid = (detList[det]->ypixels() - 1) / 2;
    BoundingBox box;
    detList[det]->getAtXY(pixmax, pixmid)->getBoundingBox(box);
    double baseX = box.xMax();
    double baseY = box.yMax();
    double baseZ = box.zMax();
    Kernel::V3D Base = V3D(baseX, baseY, baseZ) + CalCenter;
    pixmid = (detList[det]->xpixels() - 1) / 2;
    pixmax = detList[det]->ypixels() - 1;
    detList[det]->getAtXY(pixmid, pixmax)->getBoundingBox(box);
    double upX = box.xMax();
    double upY = box.yMax();
    double upZ = box.zMax();
    Kernel::V3D Up = V3D(upX, upY, upZ) + CalCenter;
    Base -= Center;
    Up -= Center;
    // Rotate around x
    baseX = Base[0];
    baseY = Base[1];
    baseZ = Base[2];
    double deg2rad = M_PI / 180.0;
    double angle = gsl_vector_get(s->x, 3) * deg2rad;
    Base = V3D(baseX, baseY * cos(angle) - baseZ * sin(angle),
               baseY * sin(angle) + baseZ * cos(angle));
    upX = Up[0];
    upY = Up[1];
    upZ = Up[2];
    Up = V3D(upX, upY * cos(angle) - upZ * sin(angle),
             upY * sin(angle) + upZ * cos(angle));
    // Rotate around y
    baseX = Base[0];
    baseY = Base[1];
    baseZ = Base[2];
    angle = gsl_vector_get(s->x, 4) * deg2rad;
    Base = V3D(baseZ * sin(angle) + baseX * cos(angle), baseY,
               baseZ * cos(angle) - baseX * sin(angle));
    upX = Up[0];
    upY = Up[1];
    upZ = Up[2];
    Up = V3D(upZ * cos(angle) - upX * sin(angle), upY,
             upZ * sin(angle) + upX * cos(angle));
    // Rotate around z
    baseX = Base[0];
    baseY = Base[1];
    baseZ = Base[2];
    angle = gsl_vector_get(s->x, 5) * deg2rad;
    Base = V3D(baseX * cos(angle) - baseY * sin(angle),
               baseX * sin(angle) + baseY * cos(angle), baseZ);
    upX = Up[0];
    upY = Up[1];
    upZ = Up[2];
    Up = V3D(upX * cos(angle) - upY * sin(angle),
             upX * sin(angle) + upY * cos(angle), upZ);
    Base.normalize();
    Up.normalize();
    Center *= 100.0;
    // << det+1  << "  "
    outfile << "5  " << detList[det]->getName().substr(4) << "  "
            << detList[det]->xpixels() << "  " << detList[det]->ypixels()
            << "  " << 100.0 * detList[det]->xsize() << "  "
            << 100.0 * detList[det]->ysize() << "  "
            << "0.2000"
            << "  " << Center.norm() << "  ";
    Center.write(outfile);
    outfile << "  ";
    Base.write(outfile);
    outfile << "  ";
    Up.write(outfile);
    outfile << "\n";

    // clean up dynamically allocated gsl stuff
    gsl_vector_free(x);
    gsl_vector_free(ss);
    gsl_multimin_fminimizer_free(s);

    // Remove the now-unneeded grouping workspace
    AnalysisDataService::Instance().remove(groupWSName);
    prog.report(detList[det]->getName());
  }

  // Closing
  outfile.close();
}

} // namespace Algorithms
} // namespace Mantid
