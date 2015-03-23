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

// Typedef to simplify function signatures
typedef std::pair<double, double> MinMax;

MinMax getDimensionExtents(IMDEventWorkspace_sptr ws, size_t index) {
  if (!ws)
    throw std::runtime_error(
        "Invalid workspace passed to getDimensionExtents.");
  auto dim = ws->getDimension(index);
  return std::make_pair(dim->getMinimum(), dim->getMaximum());
}

DblMatrix matrixFromProjection(ITableWorkspace_sptr projection) {
  if (!projection) {
    return DblMatrix(3, 3, true /* makeIdentity */);
  }

  const size_t numDims = projection->rowCount();
  DblMatrix ret(3, 3);
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

DblMatrix scaleProjection(const DblMatrix &inMatrix,
                          const std::vector<std::string> &inUnits,
                          const std::vector<std::string> &outUnits,
                          IMDEventWorkspace_sptr inWS) {
  DblMatrix ret(inMatrix);
  // Check if we actually need to do anything
  if (std::equal(inUnits.begin(), inUnits.end(), outUnits.begin()))
    return ret;

  if (inUnits.size() != outUnits.size())
    throw std::runtime_error(
        "scaleProjection given different quantity of input and output units");

  const OrientedLattice &orientedLattice =
      inWS->getExperimentInfo(0)->sample().getOrientedLattice();

  const size_t numDims = inUnits.size();
  for (size_t i = 0; i < numDims; ++i) {
    const double dStar =
        2 * M_PI *
        orientedLattice.dstar(inMatrix[i][0], inMatrix[i][1], inMatrix[i][2]);
    if (inUnits[i] == outUnits[i])
      continue;
    else if (inUnits[i] == "a") {
      // inv angstroms to rlu
      for (size_t j = 0; j < numDims; ++j)
        ret[i][j] *= dStar;
    } else {
      // rlu to inv angstroms
      for (size_t j = 0; j < numDims; ++j)
        ret[i][j] /= dStar;
    }
  }

  return ret;
}

std::vector<MinMax> calculateExtents(const DblMatrix &inMatrix,
                                     const std::vector<MinMax> &limits) {
  DblMatrix invMat(inMatrix);
  invMat.Invert();

  // iterate through min/max of each dimension, calculate dot(vert, inv_mat)
  // and store min/max value for each dimension
  std::vector<double> hRange(2), kRange(2), lRange(2);

  hRange[0] = limits[0].first;
  hRange[1] = limits[0].second;
  kRange[0] = limits[1].first;
  kRange[1] = limits[1].second;
  lRange[0] = limits[2].first;
  lRange[1] = limits[2].second;

  // Calculate the minimums and maximums of transformed coordinates
  DblMatrix extMat(3, 8);
  size_t counter = 0;
  for (auto hIt = hRange.begin(); hIt != hRange.end(); ++hIt)
    for (auto kIt = kRange.begin(); kIt != kRange.end(); ++kIt)
      for (auto lIt = lRange.begin(); lIt != lRange.end(); ++lIt) {
        V3D origPos(*hIt, *kIt, *lIt);
        for (size_t i = 0; i < 3; ++i) {
          const V3D other(invMat[i][0], invMat[i][1], invMat[i][2]);
          extMat[i][counter++] = origPos.scalar_prod(other);
        }
      }

  // Reduce down to the minimum and maximum vertices
  V3D min(extMat[0][0], extMat[1][0], extMat[2][0]);
  V3D max(extMat[0][0], extMat[1][0], extMat[2][0]);

  for (size_t i = 1; i < 8; ++i) {
    for (size_t j = 0; j < 3; ++j) {
      if (extMat[j][i] < min[j])
        min[j] = extMat[j][i];
      if (extMat[j][i] > max[j])
        max[j] = extMat[j][i];
    }
  }

  std::vector<MinMax> extents(3);
  for (size_t i = 0; i < 3; ++i) {
    extents[i].first = min[i];
    extents[i].second = max[i];
  }

  return extents;
}

std::pair<std::vector<MinMax>, std::vector<size_t>>
calculateSteps(std::vector<MinMax> inExtents,
               std::vector<std::vector<double>> binning) {
  std::vector<MinMax> outExtents(inExtents);
  std::vector<size_t> outBins;

  const size_t numBins = binning.size();

  for (size_t i = 0; i < numBins; ++i) {
    const size_t nArgs = binning[i].size();
    if (nArgs == 0) {
      throw std::runtime_error("Binning parameter cannot be empty");
    } else if (nArgs == 1) {
      const double dimRange = inExtents[i].second - inExtents[i].first;
      double stepSize = binning[i][0];
      if (stepSize > dimRange)
        stepSize = dimRange;
      outExtents[i].second = inExtents[i].first + static_cast<double>(numBins) * stepSize;
      outBins.push_back(static_cast<size_t>(dimRange / stepSize));
    } else if (nArgs == 2) {
      outExtents[i].first = binning[i][0];
      outExtents[i].second = binning[i][1];
      outBins.push_back(1);
    } else if (nArgs == 3) {
      const double dimMin = binning[i][0];
      const double dimMax = binning[i][2];
      const double dimRange = dimMax - dimMin;
      double stepSize = binning[i][i];
      if (stepSize > dimRange)
        stepSize = dimRange;
      outExtents[i].second = dimMin + static_cast<double>(numBins) * stepSize;
      outExtents[i].first = dimMin;
      outBins.push_back(static_cast<size_t>(dimRange/stepSize));
    } else {
      throw std::runtime_error("Cannot handle " +
                                 boost::lexical_cast<std::string>(nArgs) +
                                 " bins.");
    }
  }
  return std::make_pair(outExtents, outBins);
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
