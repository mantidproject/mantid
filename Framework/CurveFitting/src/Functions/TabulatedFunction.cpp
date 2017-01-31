//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/TabulatedFunction.h"
#include "MantidKernel/FileValidator.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

namespace {
/// static logger
Logger g_log("TabulatedFunction");
}

DECLARE_FUNCTION(TabulatedFunction)

const int TabulatedFunction::defaultIndexValue = 0;

/// Constructor
TabulatedFunction::TabulatedFunction()
    : m_setupFinished(false), m_explicitXY(false) {
  declareParameter("Scaling", 1.0, "A scaling factor");
  declareParameter("Shift", 0.0, "Shift in the abscissa");
  declareParameter("XScaling", 1.0, "Scaling factor in X");
  declareAttribute("FileName", Attribute("", true));
  declareAttribute("Workspace", Attribute(""));
  declareAttribute("WorkspaceIndex", Attribute(defaultIndexValue));
}

/// Evaluate the function for a list of arguments and given scaling factor
void TabulatedFunction::eval(double scaling, double xshift, double xscale,
                             double *out, const double *xValues,
                             const size_t nData) const {
  if (nData == 0)
    return;

  setupData();

  if (size() == 0)
    return;

  // shift and scale the domain over which the function is defined
  std::vector<double> xData(m_xData);
  for (double &value : xData) {
    value *= xscale;
    value += xshift;
  }

  const double xStart = xData.front();
  const double xEnd = xData.back();

  if (xStart >= xValues[nData - 1] || xEnd <= xValues[0])
    return;

  size_t i = 0;
  while (i < nData - 1 && xValues[i] < xStart) {
    out[i] = 0;
    i++;
  }
  size_t j = 0;
  for (; i < nData; i++) {
    double xi = xValues[i];
    while (j < size() - 1 && xi > xData[j])
      j++;
    if (j > size() - 1) {
      out[i] = 0;
    } else {
      if (xi == xData[j]) {
        out[i] = m_yData[j] * scaling;
      } else if (xi > xData[j]) {
        out[i] = 0;
      } else if (j > 0) {
        double x0 = xData[j - 1];
        double x1 = xData[j];
        double y0 = m_yData[j - 1];
        double y1 = m_yData[j];
        out[i] = y0 + (y1 - y0) * (xi - x0) / (x1 - x0);
        out[i] *= scaling;
      } else {
        out[i] = 0;
      }
    }
  }
}

/**
 * Calculate the function values.
 * @param out :: The output buffer for the calculated values.
 * @param xValues :: The array of x-values.
 * @param nData :: The size of the data.
 */
void TabulatedFunction::function1D(double *out, const double *xValues,
                                   const size_t nData) const {
  const double scaling = getParameter("Scaling");
  const double xshift = getParameter("Shift");
  const double xscale = getParameter("XScaling");
  eval(scaling, xshift, xscale, out, xValues, nData);
}

/**
 * function derivatives
 * @param out :: The output Jacobian matrix: function derivatives over its
 * parameters.
 * @param xValues :: The function arguments
 * @param nData :: The size of xValues.
 */
void TabulatedFunction::functionDeriv1D(API::Jacobian *out,
                                        const double *xValues,
                                        const size_t nData) {
  const double scaling = getParameter("Scaling");
  const double xshift = getParameter("Shift");
  const double xscale = getParameter("XScaling");
  std::vector<double> tmp(nData);
  // derivative with respect to Scaling parameter
  eval(1.0, xshift, xscale, tmp.data(), xValues, nData);
  for (size_t i = 0; i < nData; ++i) {
    out->set(i, 0, tmp[i]);
  }

  const double dx =
      (xValues[nData - 1] - xValues[0]) / static_cast<double>(nData);
  std::vector<double> tmpplus(nData);
  std::vector<double> tmpminus(nData);

  // There is no unique definition for the partial derivative with respect
  // to the Shift parameter. Here we take the central difference,
  eval(scaling, xshift + dx, xscale, tmpplus.data(), xValues, nData);
  eval(scaling, xshift - dx, xscale, tmpminus.data(), xValues, nData);
  for (size_t i = 0; i < nData; ++i) {
    out->set(i, 1, (tmpplus[i] - tmpminus[i]) / (2 * dx));
  }

  eval(scaling, xshift, xscale + dx, tmpplus.data(), xValues, nData);
  eval(scaling, xshift, xscale - dx, tmpminus.data(), xValues, nData);
  for (size_t i = 0; i < nData; ++i) {
    out->set(i, 2, (tmpplus[i] - tmpminus[i]) / (2 * dx));
  }
}

/// Clear all data
void TabulatedFunction::clear() const {
  m_xData.clear();
  m_yData.clear();
  m_setupFinished = false;
}

/** Set a value to attribute attName
 * @param attName :: The attribute name
 * @param value :: The new value
 */
void TabulatedFunction::setAttribute(const std::string &attName,
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
    m_setupFinished = false;
    m_explicitXY = false;
  } else if (attName == "Workspace") {
    std::string wsName = value.asString();
    if (!wsName.empty()) {
      storeAttributeValue(attName, value);
      storeAttributeValue("FileName", Attribute("", true));
      loadWorkspace(wsName);
      m_setupFinished = false;
      m_explicitXY = false;
    }
  } else if (attName == "X") {
    m_xData = value.asVector();
    if (m_xData.empty()) {
      m_setupFinished = false;
      m_explicitXY = false;
      if (!m_yData.empty()) {
        m_yData.clear();
      }
      return;
    }
    if (m_xData.size() != m_yData.size()) {
      m_yData.resize(m_xData.size());
    }
    storeAttributeValue("FileName", Attribute("", true));
    storeAttributeValue("Workspace", Attribute(""));
    m_setupFinished = true;
    m_explicitXY = true;
  } else if (attName == "Y") {
    m_yData = value.asVector();
    if (m_yData.empty()) {
      m_setupFinished = false;
      m_explicitXY = false;
      if (!m_xData.empty()) {
        m_xData.clear();
      }
      return;
    }
    if (m_xData.size() != m_yData.size()) {
      m_xData.resize(m_yData.size());
    }
    storeAttributeValue("FileName", Attribute("", true));
    storeAttributeValue("Workspace", Attribute(""));
    m_setupFinished = true;
    m_explicitXY = true;
  } else {
    IFunction::setAttribute(attName, value);
    m_setupFinished = false;
  }
}

/// Returns the number of attributes associated with the function
size_t TabulatedFunction::nAttributes() const {
  // additional X and Y attributes
  return IFunction::nAttributes() + 2;
}

/// Returns a list of attribute names
std::vector<std::string> TabulatedFunction::getAttributeNames() const {
  std::vector<std::string> attNames = IFunction::getAttributeNames();
  attNames.push_back("X");
  attNames.push_back("Y");
  return attNames;
}

/// Return a value of attribute attName
/// @param attName :: The attribute name
IFunction::Attribute
TabulatedFunction::getAttribute(const std::string &attName) const {
  if (attName == "X") {
    return m_explicitXY ? Attribute(m_xData) : Attribute(std::vector<double>());
  } else if (attName == "Y") {
    return m_explicitXY ? Attribute(m_yData) : Attribute(std::vector<double>());
  }
  return IFunction::getAttribute(attName);
}

/// Check if attribute attName exists
/// @param attName :: The attribute name
bool TabulatedFunction::hasAttribute(const std::string &attName) const {
  if (attName == "X" || attName == "Y") {
    return true;
  }
  return IFunction::hasAttribute(attName);
}

/**
 * Load input file as a Nexus file.
 * @param fname :: The file name
 */
void TabulatedFunction::load(const std::string &fname) {
  IAlgorithm_sptr loadAlg =
      Mantid::API::AlgorithmFactory::Instance().create("Load", -1);
  loadAlg->initialize();
  loadAlg->setChild(true);
  loadAlg->setLogging(false);
  try {
    loadAlg->setPropertyValue("Filename", fname);
    loadAlg->setPropertyValue("OutputWorkspace",
                              "_TabulatedFunction_fit_data_");
    loadAlg->execute();
  } catch (std::runtime_error &) {
    throw std::runtime_error(
        "Unable to load Nexus file for TabulatedFunction function.");
  }

  Workspace_sptr ws = loadAlg->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr resData =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
  loadWorkspace(resData);
}

/**
 * Load the points from a MatrixWorkspace
 * @param wsName :: The workspace to load from
 */
void TabulatedFunction::loadWorkspace(const std::string &wsName) const {
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
  loadWorkspace(ws);
}

/**
 * Load the points from a MatrixWorkspace
 * @param ws :: The workspace to load from
 */
void TabulatedFunction::loadWorkspace(
    boost::shared_ptr<API::MatrixWorkspace> ws) const {
  m_workspace = ws;
  m_setupFinished = false;
}

/**
  * Fill in the x and y value containers (m_xData and m_yData)
  */
void TabulatedFunction::setupData() const {
  if (m_setupFinished) {
    if (m_xData.size() != m_yData.size()) {
      throw std::invalid_argument(this->name() +
                                  ": X and Y vectors have different sizes.");
    }
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

  size_t index = static_cast<size_t>(getAttribute("WorkspaceIndex").asInt());

  g_log.debug() << "Setting up " << m_workspace->getName() << " index " << index
                << '\n';

  const auto &xData = m_workspace->points(index);
  const auto &yData = m_workspace->y(index);
  m_xData.assign(xData.begin(), xData.end());
  m_yData.assign(yData.begin(), yData.end());

  m_workspace.reset();
  m_setupFinished = true;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
