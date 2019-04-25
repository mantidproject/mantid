// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MDFFunctionPlotData.h"
#include "MantidQtWidgets/Plotting/Qwt/ErrorCurve.h"
#include "MultiDatasetFit.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Exception.h"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

namespace MantidQt {
namespace CustomInterfaces {
namespace MDF {

/// Default size of the function domain
size_t MDFFunctionPlotData::g_defaultDomainSize = 100;

namespace {
/// Default curve color
auto FUNCTION_CURVE_COLOR = Qt::magenta;
} // namespace

/// Constructor.
/// @param fun :: A function to plot.
/// @param startX :: A lower bound of the evaluation interval.
/// @param endX :: An upper bound of the evaluation interval.
/// @param nX :: A number of values to evaluate.
MDFFunctionPlotData::MDFFunctionPlotData(
    boost::shared_ptr<Mantid::API::IFunction> fun, double startX, double endX,
    size_t nX)
    : m_function(fun), m_functionCurve(new QwtPlotCurve()) {
  setDomain(startX, endX, nX);
  auto pen = m_functionCurve->pen();
  pen.setColor(FUNCTION_CURVE_COLOR);
  m_functionCurve->setPen(pen);
}

/// Destructor.
MDFFunctionPlotData::~MDFFunctionPlotData() {
  m_functionCurve->detach();
  delete m_functionCurve;
}

/// Define function's domain and set the data to the curve.
void MDFFunctionPlotData::setDomain(double startX, double endX, size_t nX) {
  Mantid::API::FunctionDomain1DVector x(startX, endX, nX);
  Mantid::API::FunctionValues y(x);
  try {
    m_function->function(x, y);
  } catch (std::invalid_argument &) {
    // Do nothing.
    // Maybe the function hasn't been set up yet.
  } catch (std::exception &e) {
    MultiDatasetFit::logWarning(e.what());
  }
  m_functionCurve->setData(x.getPointerAt(0), y.getPointerToCalculated(0),
                           static_cast<int>(x.size()));
}

/// Show the curves on a plot.
void MDFFunctionPlotData::show(QwtPlot *plot) {
  m_functionCurve->attach(plot);

  auto itemList = plot->itemList();

  // set the guess plot on the bottom
  double lowestZ = 0.0;
  for (auto item : itemList) {
    auto z = item->z();
    if (lowestZ > z)
      lowestZ = z;
  }

  m_functionCurve->setZ(lowestZ - 1.0);
}

/// Hide the curves from any plot.
void MDFFunctionPlotData::hide() { m_functionCurve->detach(); }

/// Get the bounding rect including all plotted data.
QwtDoubleRect MDFFunctionPlotData::boundingRect() const {
  QwtDoubleRect rect = m_functionCurve->boundingRect();
  return rect;
}

/// Update function parameters and attributes
/// @param fun :: A function to copy attributes and parameters from.
void MDFFunctionPlotData::updateFunction(const Mantid::API::IFunction &fun) {
  if (!m_function)
    return;
  if (m_function->nParams() != fun.nParams()) {
    throw std::logic_error(
        "Cannot update function: different number of parameters.");
  }
  if (m_function->nAttributes() != fun.nAttributes()) {
    throw std::logic_error(
        "Cannot update function: different number of attributes.");
  }
  // Copy the attributes
  auto attributes = fun.getAttributeNames();
  for (auto &attribute : attributes) {
    auto value = fun.getAttribute(attribute);
    m_function->setAttribute(attribute, value);
  }
  // Copy the parameters
  for (size_t i = 0; i < fun.nParams(); ++i) {
    const auto paramName = fun.parameterName(i);
    const auto paramValue = fun.getParameter(i);
    m_function->setParameter(paramName, paramValue);
  }
}

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt
