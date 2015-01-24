#include "MantidMDEvents/ReflectometryTransform.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidKernel/UnitFactory.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace MDEvents {

ReflectometryTransform::ReflectometryTransform(int numberOfBinsQx,
                                               int numberOfBinsQz)
    : m_nbinsx(numberOfBinsQx), m_nbinsz(numberOfBinsQz) {}

ReflectometryTransform::~ReflectometryTransform() {}

boost::shared_ptr<MDEventWorkspace2Lean>
ReflectometryTransform::createMDWorkspace(
    Mantid::Geometry::IMDDimension_sptr a,
    Mantid::Geometry::IMDDimension_sptr b,
    BoxController_sptr boxController) const {
  auto ws = boost::make_shared<MDEventWorkspace2Lean>();

  ws->addDimension(a);
  ws->addDimension(b);

  BoxController_sptr wsbc = ws->getBoxController(); // Get the box controller
  wsbc->setSplitInto(boxController->getSplitInto(0));
  wsbc->setMaxDepth(boxController->getMaxDepth());
  wsbc->setSplitThreshold(boxController->getSplitThreshold());

  // Initialize the workspace.
  ws->initialize();

  // Start with a MDGridBox.
  ws->splitBox();
  return ws;
}

/**
 * Create a new X-Axis for the output workspace
 * @param ws : Workspace to attach the axis to
 * @param gradX : Gradient used in the linear transform from index to X-scale
 * @param cxToUnit : C-offset used in the linear transform
 * @param nBins : Number of bins along this axis
 * @param caption : Caption for the axis
 * @param units : Units label for the axis
 * @return Vector containing increments along the axis.
 */
MantidVec createXAxis(MatrixWorkspace *const ws, const double gradX,
                      const double cxToUnit, const size_t nBins,
                      const std::string &caption, const std::string &units) {
  // Create an X - Axis.
  auto *const xAxis = new BinEdgeAxis(nBins);
  ws->replaceAxis(0, xAxis);
  auto unitXBasePtr = UnitFactory::Instance().create("Label");
  boost::shared_ptr<Mantid::Kernel::Units::Label> xUnit =
      boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(unitXBasePtr);
  xUnit->setLabel(caption, units);
  xAxis->unit() = xUnit;
  xAxis->title() = caption;
  MantidVec xAxisVec(nBins);
  for (size_t i = 0; i < nBins; ++i) {
    double qxIncrement = ((1 / gradX) * ((double)i + 1) + cxToUnit);
    xAxis->setValue(i, qxIncrement);
    xAxisVec[i] = qxIncrement;
  }
  return xAxisVec;
}

/**
 * Create a new Y, or Vertical Axis for the output workspace
 * @param ws : Workspace to attache the vertical axis to
 * @param xAxisVec : Vector of x axis increments
 * @param gradY : Gradient used in linear transform from index to Y-scale
 * @param cyToUnit : C-offset used in the linear transform
 * @param nBins : Number of bins along the axis
 * @param caption : Caption for the axis
 * @param units : Units label for the axis
 */
void createVerticalAxis(MatrixWorkspace *const ws, const MantidVec &xAxisVec,
                        const double gradY, const double cyToUnit,
                        const size_t nBins, const std::string &caption,
                        const std::string &units) {
  // Create a Y (vertical) Axis
  auto *const verticalAxis = new BinEdgeAxis(nBins);
  ws->replaceAxis(1, verticalAxis);
  auto unitZBasePtr = UnitFactory::Instance().create("Label");
  boost::shared_ptr<Mantid::Kernel::Units::Label> verticalUnit =
      boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(unitZBasePtr);
  verticalAxis->unit() = verticalUnit;
  verticalUnit->setLabel(caption, units);
  verticalAxis->title() = caption;
  for (size_t i = 0; i < nBins; ++i) {
    ws->setX(i, xAxisVec);
    double qzIncrement = ((1 / gradY) * ((double)i + 1) + cyToUnit);
    verticalAxis->setValue(i, qzIncrement);
  }
}
}
}
