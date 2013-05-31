/*WIKI* 

Creates a single spectrum Workspace2D with X,Y, and E copied from an first non-integrated dimension of a IMDHistoWorkspace.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertMDHistoToMatrixWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ListValidator.h"

#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>
#include <sstream>


namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertMDHistoToMatrixWorkspace)

/// Sets documentation strings for this algorithm
void ConvertMDHistoToMatrixWorkspace::initDocs()
{
  this->setWikiSummary("Creates a single spectrum Workspace2D with X,Y, and E copied from an first non-integrated dimension of a IMDHistoWorkspace.");
  this->setOptionalMessage("Creates a single spectrum Workspace2D with X,Y, and E copied from an first non-integrated dimension of a IMDHistoWorkspace.");
}

using namespace Kernel;
using namespace API;

/// Decalare the properties
void ConvertMDHistoToMatrixWorkspace::init()
{
  declareProperty(new WorkspaceProperty<API::IMDHistoWorkspace>("InputWorkspace","",Direction::Input), "An input IMDHistoWorkspace.");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output Workspace2D.");

  std::vector<std::string> normalizations(3);
  normalizations[0] = "NoNormalization";
  normalizations[1] = "VolumeNormalization";
  normalizations[2] = "NumEventsNormalization";

  declareProperty("Normalization",normalizations[0],
    Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(normalizations)),
    "Signal normalization method");
}

/// Execute the algorithm
void ConvertMDHistoToMatrixWorkspace::exec()
{
  IMDHistoWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  // This code is copied from MantidQwtIMDWorkspaceData
  Mantid::Geometry::VecIMDDimension_const_sptr nonIntegDims = inputWorkspace->getNonIntegratedDimensions();
  std::string alongDim = "";
  if (!nonIntegDims.empty())
    alongDim = nonIntegDims[0]->getName();
  else
    alongDim = inputWorkspace->getDimension(0)->getName();

  size_t nd = inputWorkspace->getNumDims();
  Mantid::Kernel::VMD start = VMD(nd);
  Mantid::Kernel::VMD end = VMD(nd);
  size_t id = 0;
  for (size_t d=0; d<nd; d++)
  {
    Mantid::Geometry::IMDDimension_const_sptr dim = inputWorkspace->getDimension(d);
    if (dim->getDimensionId() == alongDim)
    {
      // All the way through in the single dimension
      start[d] = dim->getMinimum();
      end[d] = dim->getMaximum();
      id = d;
    }
    else
    {
      // Mid point along each dimension
      start[d] = (dim->getMaximum() + dim->getMinimum()) / 2.0f;
      end[d] = start[d];
    }
  }

  // Unit direction of the line
  Mantid::Kernel::VMD dir = end - start;
  dir.normalize();


  std::string normProp = getPropertyValue("Normalization");

  Mantid::API::MDNormalization normalization;
  if (normProp == "NoNormalization")
  {
    normalization = NoNormalization;
  }
  else if (normProp == "VolumeNormalization")
  {
    normalization = VolumeNormalization;
  }
  else if (normProp == "NumEventsNormalization")
  {
    normalization = NumEventsNormalization;
  }
  else
  {
    normalization = NoNormalization;
  }

  std::vector<Mantid::coord_t> X;
  std::vector<Mantid::signal_t> Y;
  std::vector<Mantid::signal_t> E;

  inputWorkspace->getLinePlot(start, end, normalization, X, Y, E);

  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create("Workspace2D",1,X.size(),Y.size());
  //outputWorkspace->dataX(0).assign(X.begin(),X.end());
  outputWorkspace->dataY(0).assign(Y.begin(),Y.end());
  outputWorkspace->dataE(0).assign(E.begin(),E.end());

  assert(X.size() == outputWorkspace->dataX(0).size());
  for(size_t i = 0; i < X.size(); ++i)
  {
    // Coordinates in the workspace being plotted
    VMD wsCoord = start + dir * X[i];
    // Transform to the original workspace's coordinates
    //VMD originalCoord = m_transform->applyVMD(wsCoord);
    // And pick only that coordinate
    //x = originalCoord[m_currentPlotAxis];
    outputWorkspace->dataX(0)[i] = wsCoord[id];
  }

  boost::shared_ptr<Kernel::Units::Label> labelX = boost::dynamic_pointer_cast<Kernel::Units::Label>(
    Kernel::UnitFactory::Instance().create("Label")
    );
  labelX->setLabel(alongDim);
  outputWorkspace->getAxis(0)->unit() = labelX;

  outputWorkspace->setYUnitLabel("Signal");

  setProperty("OutputWorkspace", outputWorkspace);

}

} // namespace Algorithms
} // namespace Mantid

