//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/SCDPanelErrors.h"
#include "MantidKernel/FileValidator.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include <boost/math/special_functions/round.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace Mantid {
namespace Crystal {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

namespace {
/// static logger
Logger g_log("SCDPanelErrors");
}

DECLARE_FUNCTION(SCDPanelErrors)

const int SCDPanelErrors::defaultIndexValue = 0;

/// Constructor
SCDPanelErrors::SCDPanelErrors() : m_setupFinished(false) {
  declareParameter("XShift", 0.0, "Shift factor in X");
  declareParameter("YShift", 0.0, "Shift factor in Y");
  declareParameter("ZShift", 0.0, "Shift factor in Z");
  declareParameter("XRotate", 0.0, "Rotate angle in X");
  declareParameter("YRotate", 0.0, "Rotate angle in Y");
  declareParameter("ZRotate", 0.0, "Rotate angle in Z");
  declareAttribute("FileName", Attribute("", true));
  declareAttribute("Workspace", Attribute(""));
  declareAttribute("Bank", Attribute(""));
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

void SCDPanelErrors::moveDetector(
    double x, double y, double z, double rotx, double roty, double rotz,
    std::string detname, Workspace_sptr inputW) const {

  IAlgorithm_sptr alg1 = Mantid::API::AlgorithmFactory::Instance().create("MoveInstrumentComponent", -1);
  alg1->initialize();
  alg1->setChild(true);
  alg1->setLogging(false);
  alg1->setProperty<Workspace_sptr>("Workspace", inputW);
  alg1->setPropertyValue("ComponentName", detname);
  // Move in m
  alg1->setProperty("X", x);
  alg1->setProperty("Y", y);
  alg1->setProperty("Z", z);
  alg1->setPropertyValue("RelativePosition", "1");
  alg1->execute();

  IAlgorithm_sptr algx = Mantid::API::AlgorithmFactory::Instance().create("RotateInstrumentComponent", -1);
  algx->initialize();
  algx->setChild(true);
  algx->setLogging(false);
  algx->setProperty<Workspace_sptr>("Workspace", inputW);
  algx->setPropertyValue("ComponentName", detname);
  algx->setProperty("X", 1.0);
  algx->setProperty("Y", 0.0);
  algx->setProperty("Z", 0.0);
  algx->setProperty("Angle", rotx);
  algx->setPropertyValue("RelativeRotation", "1");
  algx->execute();

  IAlgorithm_sptr algy = Mantid::API::AlgorithmFactory::Instance().create("RotateInstrumentComponent", -1);
  algy->initialize();
  algy->setChild(true);
  algy->setLogging(false);
  algy->setProperty<Workspace_sptr>("Workspace", inputW);
  algy->setPropertyValue("ComponentName", detname);
  algy->setProperty("X", 0.0);
  algy->setProperty("Y", 1.0);
  algy->setProperty("Z", 0.0);
  algy->setProperty("Angle", roty);
  algy->setPropertyValue("RelativeRotation", "1");
  algy->execute();

  IAlgorithm_sptr algz = Mantid::API::AlgorithmFactory::Instance().create("RotateInstrumentComponent", -1);
  algz->initialize();
  algz->setChild(true);
  algz->setLogging(false);
  algz->setProperty<Workspace_sptr>("Workspace", inputW);
  algz->setPropertyValue("ComponentName", detname);
  algz->setProperty("X", 0.0);
  algz->setProperty("Y", 0.0);
  algz->setProperty("Z", 1.0);
  algz->setProperty("Angle", rotz);
  algz->setPropertyValue("RelativeRotation", "1");
  algz->execute();
}

/// Evaluate the function for a list of arguments and given scaling factor
void SCDPanelErrors::eval(double xrotate, double yrotate, double zrotate, double xshift, double yshift, double zshift,
                             double *out, const double *xValues,
                             const size_t nData) const {
  UNUSED_ARG(xValues);
  if (nData == 0)
    return;

  setupData();

  moveDetector(xshift, yshift, zshift, xrotate, yrotate, zrotate, m_bank,m_workspace);
  DataObjects::PeaksWorkspace_sptr inputP =
      boost::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(m_workspace);
  Geometry::Instrument_sptr inst = boost::const_pointer_cast<Geometry::Instrument>(inputP->getInstrument());
  int j = 0;
  for (size_t i = 0; i < nData; i += 3) {
    DataObjects::Peak peak = inputP->getPeak(j);
    j++;
    V3D hkl =  V3D(boost::math::iround(peak.getH()), boost::math::iround(peak.getK()),
         boost::math::iround(peak.getL()));
    DataObjects::Peak peak2(inst,peak.getDetectorID(),peak.getWavelength(),hkl,peak.getGoniometerMatrix());
   //peak2.findDetector();
    V3D Q3 = peak2.getQSampleFrame();
    out[i] = Q3[0];
    out [i+1] = Q3[1];
    out[i+2] = Q3[2];
  }
  moveDetector(-xshift, -yshift, -zshift, -xrotate, -yrotate, -zrotate, m_bank,m_workspace);
}

/**
 * Calculate the function values.
 * @param out :: The output buffer for the calculated values.
 * @param xValues :: The array of x-values.
 * @param nData :: The size of the data.
 */
void SCDPanelErrors::function1D(double *out, const double *xValues,
                                   const size_t nData) const {
  const double xrotate = getParameter("XRotate");
  const double yrotate = getParameter("YRotate");
  const double zrotate = getParameter("ZRotate");
  const double xshift = getParameter("XShift");
  const double yshift = getParameter("YShift");
  const double zshift = getParameter("ZShift");
  eval(xrotate, yrotate, zrotate, xshift, yshift, zshift, out, xValues, nData);
}

/**
 * function derivatives
 * @param out :: The output Jacobian matrix: function derivatives over its
 * parameters.
 * @param xValues :: The function arguments
 * @param nData :: The size of xValues.
 */
void SCDPanelErrors::functionDeriv1D(API::Jacobian *out,
                                        const double *xValues,
                                        const size_t nData) {
  FunctionDomain1DView domain(xValues, nData);
  this->calNumericalDeriv(domain, *out);
}

/// Clear all data
void SCDPanelErrors::clear() const {
  m_setupFinished = false;
}

/** Set a value to attribute attName
 * @param attName :: The attribute name
 * @param value :: The new value
 */
void SCDPanelErrors::setAttribute(const std::string &attName,
                                     const IFunction::Attribute &value) {
  if (attName == "FileName") {
    std::string fileName = value.asUnquotedString();
    if (fileName.empty()) {
      storeAttributeValue("FileName", Attribute("", true));
      return;
    }
    FileValidator fval;
    std::string error = fval.isValid(fileName);
    if (error == "") {
      storeAttributeValue(attName, Attribute(fileName, true));
      storeAttributeValue("Workspace", Attribute(""));
    } else {
      // file not found
      throw Kernel::Exception::FileError(error, fileName);
    }
    load(fileName);
  } else if (attName == "Workspace") {
    std::string wsName = value.asString();
    if (!wsName.empty()) {
      storeAttributeValue(attName, value);
      storeAttributeValue("FileName", Attribute("", true));
      loadWorkspace(wsName);
    }
  } else {
    IFunction::setAttribute(attName, value);
    m_setupFinished = false;
  }
}

/**
 * Load input file as a Nexus file.
 * @param fname :: The file name
 */
void SCDPanelErrors::load(const std::string &fname) {
  IAlgorithm_sptr loadAlg =
      Mantid::API::AlgorithmFactory::Instance().create("Load", -1);
  loadAlg->initialize();
  loadAlg->setChild(true);
  loadAlg->setLogging(false);
  try {
    loadAlg->setPropertyValue("Filename", fname);
    loadAlg->setPropertyValue("OutputWorkspace",
                              "_SCDPanelErrors_fit_data_");
    loadAlg->execute();
  } catch (std::runtime_error &) {
    throw std::runtime_error(
        "Unable to load Nexus file for SCDPanelErrors function.");
  }

  Workspace_sptr ws = loadAlg->getProperty("OutputWorkspace");
  API::Workspace_sptr resData =
      boost::dynamic_pointer_cast<Mantid::API::Workspace>(ws);
  loadWorkspace(resData);
}

/**
 * Load the points from a PeaksWorkspace
 * @param wsName :: The workspace to load from
 */
void SCDPanelErrors::loadWorkspace(const std::string &wsName) const {
  auto ws = AnalysisDataService::Instance().retrieveWS<API::Workspace>(wsName);
  loadWorkspace(ws);
}

/**
 * Load the points from a PeaksWorkspace
 * @param ws :: The workspace to load from
 */
void SCDPanelErrors::loadWorkspace(
    boost::shared_ptr<API::Workspace> ws) const {
  m_workspace = ws;
  m_setupFinished = false;
}

/**
  * Fill in the workspace name and bank
  */
void SCDPanelErrors::setupData() const {
  if (m_setupFinished) {
    g_log.debug() << "Re-setting isn't required.";
    return;
  }

  if (!m_workspace) {
    std::string wsName = getAttribute("Workspace").asString();
    if (wsName.empty())
      throw std::invalid_argument("Data not set for function " + this->name());
    else
      loadWorkspace(wsName);
  }

  m_bank = getAttribute("Bank").asString();

  g_log.debug() << "Setting up " << m_workspace->name() << " bank " << m_bank
                << '\n';

  m_setupFinished = true;
}

} // namespace Crystal
} // namespace Mantid
