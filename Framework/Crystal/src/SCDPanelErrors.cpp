// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/SCDPanelErrors.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ResizeRectangularDetectorHelper.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/DynamicPointerCastHelper.h"
#include "MantidKernel/FileValidator.h"
#include <algorithm>
#include <boost/math/special_functions/round.hpp>
#include <cmath>
#include <fstream>
#include <sstream>
#include <utility>

namespace Mantid::Crystal {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

namespace {
/// static logger
Logger g_log("SCDPanelErrors");
} // namespace

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
  declareParameter("ScaleWidth", 1.0, "Scale width of detector");
  declareParameter("ScaleHeight", 1.0, "Scale height of detector");
  declareParameter("T0Shift", 0.0, "Shift for TOF");
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
 * @param scalex :: Scale width of rectangular detector
 * @param scaley :: Scale height of rectangular detector
 * @param detname :: The detector name
 * @param inputW :: The workspace
 */

void SCDPanelErrors::moveDetector(double x, double y, double z, double rotx, double roty, double rotz, double scalex,
                                  double scaley, std::string detname, const Workspace_sptr &inputW) const {
  if (detname.compare("none") == 0.0)
    return;
  // CORELLI has sixteenpack under bank
  DataObjects::PeaksWorkspace_sptr inputP =
      Kernel::DynamicPointerCastHelper::dynamicPointerCastWithCheck<DataObjects::PeaksWorkspace, API::Workspace>(
          inputW);
  Geometry::Instrument_sptr inst = std::const_pointer_cast<Geometry::Instrument>(inputP->getInstrument());
  if (inst->getName().compare("CORELLI") == 0.0 && detname != "moderator")
    detname.append("/sixteenpack");

  if (x != 0.0 || y != 0.0 || z != 0.0) {
    auto alg1 = Mantid::API::AlgorithmFactory::Instance().create("MoveInstrumentComponent", -1);
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
  }

  if (rotx != 0.0) {
    auto algx = Mantid::API::AlgorithmFactory::Instance().create("RotateInstrumentComponent", -1);
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
  }

  if (roty != 0.0) {
    auto algy = Mantid::API::AlgorithmFactory::Instance().create("RotateInstrumentComponent", -1);
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
  }

  if (rotz != 0.0) {
    auto algz = Mantid::API::AlgorithmFactory::Instance().create("RotateInstrumentComponent", -1);
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
  if (scalex != 1.0 || scaley != 1.0) {
    Geometry::IComponent_const_sptr comp = inst->getComponentByName(detname);
    std::shared_ptr<const Geometry::RectangularDetector> rectDet =
        std::dynamic_pointer_cast<const Geometry::RectangularDetector>(comp);
    if (rectDet) {
      Geometry::ParameterMap &pmap = inputP->instrumentParameters();
      auto oldscalex = pmap.getDouble(rectDet->getName(), "scalex");
      auto oldscaley = pmap.getDouble(rectDet->getName(), "scaley");
      double relscalex = scalex;
      double relscaley = scaley;
      if (!oldscalex.empty())
        relscalex /= oldscalex[0];
      if (!oldscaley.empty())
        relscaley /= oldscaley[0];
      pmap.addDouble(rectDet.get(), "scalex", scalex);
      pmap.addDouble(rectDet.get(), "scaley", scaley);
      applyRectangularDetectorScaleToComponentInfo(inputP->mutableComponentInfo(), rectDet->getComponentID(), relscalex,
                                                   relscaley);
    }
  }
}

/// Evaluate the function for a list of arguments and given scaling factor
void SCDPanelErrors::eval(double xshift, double yshift, double zshift, double xrotate, double yrotate, double zrotate,
                          double scalex, double scaley, double *out, const double *xValues, const size_t nData,
                          double tShift) const {
  UNUSED_ARG(xValues);
  if (nData == 0)
    return;

  setupData();

  std::shared_ptr<API::Workspace> cloned = m_workspace->clone();
  moveDetector(xshift, yshift, zshift, xrotate, yrotate, zrotate, scalex, scaley, m_bank, cloned);

  auto inputP =
      Kernel::DynamicPointerCastHelper::dynamicPointerCastWithCheck<DataObjects::PeaksWorkspace, API::Workspace>(
          cloned);
  auto inst = inputP->getInstrument();
  Geometry::OrientedLattice lattice = inputP->mutableSample().getOrientedLattice();
  for (int i = 0; i < inputP->getNumberPeaks(); i++) {
    const DataObjects::Peak &peak = inputP->getPeak(i);
    V3D hkl = V3D(boost::math::iround(peak.getH()), boost::math::iround(peak.getK()), boost::math::iround(peak.getL()));
    V3D Q2 = lattice.qFromHKL(hkl);
    try {
      if (hkl == V3D(0, 0, 0))
        throw std::runtime_error("unindexed peak");
      DataObjects::Peak peak2(inst, peak.getDetectorID(), peak.getWavelength(), hkl, peak.getGoniometerMatrix());
      Units::Wavelength wl;

      wl.initialize(peak2.getL1(), 0, {{UnitParams::l2, peak2.getL2()}, {UnitParams::twoTheta, peak2.getScattering()}});
      peak2.setWavelength(wl.singleFromTOF(peak.getTOF() + tShift));
      V3D Q3 = peak2.getQSampleFrame();
      out[i * 3] = Q3[0] - Q2[0];
      out[i * 3 + 1] = Q3[1] - Q2[1];
      out[i * 3 + 2] = Q3[2] - Q2[2];
    } catch (std::runtime_error &) {
      // set penalty for unindexed peaks greater than tolerance
      out[i * 3] = 0.15;
      out[i * 3 + 1] = 0.15;
      out[i * 3 + 2] = 0.15;
    }
  }
}

/**
 * Calculate the function values.
 * @param out :: The output buffer for the calculated values.
 * @param xValues :: The array of x-values.
 * @param nData :: The size of the data.
 */
void SCDPanelErrors::function1D(double *out, const double *xValues, const size_t nData) const {
  const double xshift = getParameter("XShift");
  const double yshift = getParameter("YShift");
  const double zshift = getParameter("ZShift");
  const double xrotate = getParameter("XRotate");
  const double yrotate = getParameter("YRotate");
  const double zrotate = getParameter("ZRotate");
  const double scalex = getParameter("ScaleWidth");
  const double scaley = getParameter("ScaleHeight");
  const double tShift = getParameter("T0Shift");
  eval(xshift, yshift, zshift, xrotate, yrotate, zrotate, scalex, scaley, out, xValues, nData, tShift);
}

/**
 * function derivatives
 * @param out :: The output Jacobian matrix: function derivatives over its
 * parameters.
 * @param xValues :: The function arguments
 * @param nData :: The size of xValues.
 */
void SCDPanelErrors::functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) {
  FunctionDomain1DView domain(xValues, nData);
  this->calNumericalDeriv(domain, *out);
}

/// Clear all data
void SCDPanelErrors::clear() const { m_setupFinished = false; }

/** Set a value to attribute attName
 * @param attName :: The attribute name
 * @param value :: The new value
 */
void SCDPanelErrors::setAttribute(const std::string &attName, const IFunction::Attribute &value) {
  if (attName == "FileName") {
    std::string fileName = value.asUnquotedString();
    if (fileName.empty()) {
      storeAttributeValue("FileName", Attribute("", true));
      return;
    }
    FileValidator fval;
    std::string error = fval.isValid(fileName);
    if (error.empty()) {
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
  auto loadAlg = Mantid::API::AlgorithmFactory::Instance().create("Load", -1);
  loadAlg->initialize();
  loadAlg->setChild(true);
  loadAlg->setLogging(false);
  try {
    loadAlg->setPropertyValue("Filename", fname);
    loadAlg->setPropertyValue("OutputWorkspace", "_SCDPanelErrors_fit_data_");
    loadAlg->execute();
  } catch (std::runtime_error &) {
    throw std::runtime_error("Unable to load Nexus file for SCDPanelErrors function.");
  }

  Workspace_sptr ws = loadAlg->getProperty("OutputWorkspace");
  API::Workspace_sptr resData = std::dynamic_pointer_cast<Mantid::API::Workspace>(ws);
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
void SCDPanelErrors::loadWorkspace(std::shared_ptr<API::Workspace> ws) const {
  m_workspace = std::move(ws);
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

  g_log.debug() << "Setting up " << m_workspace->getName() << " bank " << m_bank << '\n';

  m_setupFinished = true;
}

} // namespace Mantid::Crystal
