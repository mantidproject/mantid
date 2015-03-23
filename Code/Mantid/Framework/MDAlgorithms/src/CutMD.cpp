#include "MantidMDAlgorithms/CutMD.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
std::pair<double, double> getDimensionExtents(IMDEventWorkspace_sptr ws,
                                              size_t index) {
  if (!ws)
    throw std::runtime_error(
        "Invalid workspace passed to getDimensionExtents.");
  auto dim = ws->getDimension(index);
  return std::make_pair(dim->getMinimum(), dim->getMaximum());
}

Matrix<double> matrixFromProjection(ITableWorkspace_sptr projection) {
  if (!projection) {
    return Matrix<double>(3, 3, true /* makeIdentity */);
  }

  const size_t numDims = projection->rowCount();
  Matrix<double> ret(3, 3);
  for (size_t i = 0; i < numDims; i++) {
    const std::string name =
        projection->getColumn("name")->cell<std::string>(i);
    const std::string valueStr =
        projection->getColumn("value")->cell<std::string>(i);
    std::vector<std::string> valueStrVec;
    boost::split(valueStrVec, valueStr, boost::is_any_of(","));

    std::vector<double> valueDblVec;
    for (auto it = valueStrVec.begin(); it != valueStrVec.end(); ++it)
      valueDblVec.push_back(boost::lexical_cast<double>(*it));

    if (name == "u")
      ret.setRow(0, valueDblVec);
    else if (name == "v")
      ret.setRow(1, valueDblVec);
    else if (name == "w")
      ret.setRow(2, valueDblVec);
  }
  return ret;
}

std::vector<std::string> unitsFromProjection(ITableWorkspace_sptr projection) {
  std::vector<std::string> ret(3);
  const size_t numDims = projection->rowCount();
  for (size_t i = 0; i < numDims; i++) {
    const std::string name =
        projection->getColumn("name")->cell<std::string>(i);
    const std::string unit =
        projection->getColumn("type")->cell<std::string>(i);

    if (name == "u")
      ret[0] = unit;
    else if (name == "v")
      ret[1] = unit;
    else if (name == "w")
      ret[2] = unit;
  }
  return ret;
}

Matrix<double> scaleProjection(const Matrix<double> &inMatrix,
                               std::vector<std::string> inUnits,
                               std::vector<std::string> outUnits,
                               IMDEventWorkspace_sptr inWS) {
  Matrix<double> ret = inMatrix;
  //Check if we actually need to do anything
  if (std::equal(inUnits.begin(), inUnits.end(), outUnits.begin()))
    return ret;

  if(inUnits.size() != outUnits.size())
    throw std::runtime_error("scaleProjection given different quantity of input and output units");

  const OrientedLattice& orientedLattice = inWS->getExperimentInfo(0)->sample().getOrientedLattice();

  const size_t numDims = inUnits.size();
  for(size_t i = 0; i < numDims; ++i) {
    const double dStar = 2 * M_PI * orientedLattice.dstar(inMatrix[i][0], inMatrix[i][1], inMatrix[i][2]);
    if(inUnits[i] == outUnits[i])
      continue;
    else if (inUnits[i] == "a") {
      //inv angstroms to rlu
      for(size_t j = 0; j < numDims; ++j)
        ret[i][j] *= dStar;
    } else {
      //rlu to inv angstroms
      for(size_t j = 0; j < numDims; ++j)
        ret[i][j] /= dStar;
    }
  }

  return ret;
}

} // anonymous namespace

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CutMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CutMD::CutMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CutMD::~CutMD() {}

//----------------------------------------------------------------------------------------------

void CutMD::init() {
  declareProperty(new WorkspaceProperty<IMDWorkspace>("InputWorkspace", "",
                                                      Direction::Input),
                  "MDWorkspace to slice");

  declareProperty(
      new WorkspaceProperty<ITableWorkspace>("Projection", "", Direction::Input,
                                             PropertyMode::Optional),
      "Projection");

  declareProperty(new ArrayProperty<double>("P1Bin"), "Projection 1 binning.");
  declareProperty(new ArrayProperty<double>("P2Bin"), "Projection 2 binning.");
  declareProperty(new ArrayProperty<double>("P3Bin"), "Projection 3 binning.");
  declareProperty(new ArrayProperty<double>("P4Bin"), "Projection 4 binning.");
  declareProperty(new ArrayProperty<double>("P5Bin"), "Projection 5 binning.");

  declareProperty(new WorkspaceProperty<IMDWorkspace>("OutputWorkspace", "",
                                                      Direction::Output),
                  "Output cut workspace");
  declareProperty("NoPix", false, "If False creates a full MDEventWorkspaces "
                                  "as output. True to create an "
                                  "MDHistoWorkspace as output. This is DND "
                                  "only in Horace terminology.");
  declareProperty("CheckAxes", true,
                  "Check that the axis look to be correct, and abort if not.");
}
void CutMD::exec() {}

} // namespace Mantid
} // namespace MDAlgorithms
