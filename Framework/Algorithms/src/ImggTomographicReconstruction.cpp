#include "MantidAlgorithms/ImggTomographicReconstruction.h"
#include "MantidAlgorithms/Tomography/FBPTomopy.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/make_unique.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ImggTomographicReconstruction)

namespace {
// Just to have explicit double => float casts in std::copy/transform
struct DoubleToFloatStd {
  float operator()(const double &dblValue) const {
    return static_cast<float>(dblValue);
  }
};
}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ImggTomographicReconstruction::name() const {
  return "ImggTomographicReconstruction";
}

/// Algorithm's version for identification. @see Algorithm::version
int ImggTomographicReconstruction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ImggTomographicReconstruction::category() const {
  return "Diffraction\\Imaging;Diffraction\\Tomography";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ImggTomographicReconstruction::summary() const {
  return "Reconstruct a 3D volume from 2D imaging projection data";
}

namespace {
const std::string PROP_INPUT_WS = "InputWorkspace";
const std::string PROP_METHOD = "Method";
const std::string PROP_OUTPUT_WS = "OutputWorkspace";
const std::string PROP_COR = "CenterOfRotation";
const std::string PROP_RELAXATION_PARAM = "RelaxationParameter";
const std::string PROP_MAX_CORES = "MaximumCores";
const std::string PROP_MIN_PROJ_ANGLE = "MinProjectionAngle";
const std::string PROP_MAX_PROJ_ANGLE = "MaxProjectionAngle";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ImggTomographicReconstruction::init() {

  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::Workspace>>(
          PROP_INPUT_WS, "", Kernel::Direction::Input),
      "Group of workspace holding images (with one spectrum per pixel row).");

  std::vector<std::string> methods{"FBP (tomopy)"};
  declareProperty(
      PROP_METHOD, methods.front(),
      boost::make_shared<Kernel::ListValidator<std::string>>(methods),
      "Reconstruction method", Kernel::Direction::Input);

  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::WorkspaceGroup>>(
          PROP_OUTPUT_WS, "", Kernel::Direction::Output),
      "Output reconstructed volume, as a group of workspaces where "
      "each workspace holds one slice of the volume.");

  auto zeroOrPosInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  zeroOrPosInt->setLower(-1);
  declareProperty(PROP_COR, -1, zeroOrPosInt,
                  "Center of rotation for the reconstruction (in pixels).");

  auto zeroOrPosDbl = boost::make_shared<Kernel::BoundedValidator<double>>();
  zeroOrPosDbl->setLower(0.0);
  declareProperty(PROP_RELAXATION_PARAM, 0.5, zeroOrPosDbl,
                  "Relaxation parameter for the reconstruction method.");

  zeroOrPosInt->setLower(0);
  declareProperty(PROP_MAX_CORES, 0, zeroOrPosInt,
                  "Maximum number of cores to use for parallel runs. Leave "
                  "empty to use all available cores.");

  declareProperty(PROP_MIN_PROJ_ANGLE, 0.0, zeroOrPosDbl,
                  "Minimum projection angle.");

  declareProperty(PROP_MAX_PROJ_ANGLE, 180.0, zeroOrPosDbl,
                  "Maximum projection angle (assuming a uniform angle increase "
                  "from first to last projection.");
}

std::map<std::string, std::string>
ImggTomographicReconstruction::validateInputs() {
  std::map<std::string, std::string> result;

  API::Workspace_const_sptr inWks = getProperty(PROP_INPUT_WS);
  API::WorkspaceGroup_const_sptr inGrp =
      boost::dynamic_pointer_cast<const API::WorkspaceGroup>(inWks);
  if (!inGrp) {
    result[PROP_INPUT_WS] = "The current version of this algorithm only "
                            "supports input workspaces of type WorkspaceGroup";
  } else {
    if (inGrp->size() < 2) {
      result[PROP_INPUT_WS] = "The input workspace must have at least two "
                              "workspaces (projection images)";
    }

    auto first = inGrp->getItem(0);
    auto fwks = boost::dynamic_pointer_cast<API::MatrixWorkspace>(first);
    if (!fwks) {
      result[PROP_INPUT_WS] =
          "Unable to get a matrix workspace from the first "
          "item of the input workspace group " +
          first->getTitle() +
          ". It must contain workspaces of type MatrixWorkspace";
    } else {
      int cor = getProperty(PROP_COR);
      size_t bsize = fwks->blocksize();
      if (cor < 0 || cor >= static_cast<int>(bsize)) {
        result[PROP_COR] =
            "The center of rotation must be between 0 and the "
            "number of columns in the input projection images (0 to " +
            std::to_string(bsize - 1) + ")";
      }
    }
    // Not validating requirements on all input workspaces here (there could be
    // many)
  }

  double minAngle = getProperty(PROP_MIN_PROJ_ANGLE);
  double maxAngle = getProperty(PROP_MAX_PROJ_ANGLE);

  if (minAngle >= maxAngle) {
    result[PROP_MIN_PROJ_ANGLE] = PROP_MIN_PROJ_ANGLE +
                                  " cannot be equal to or lower than " +
                                  PROP_MAX_PROJ_ANGLE;
  }
  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ImggTomographicReconstruction::exec() {
  throw std::runtime_error(
      "This algorithm cannot be executed with a single workspace as input");
}

bool ImggTomographicReconstruction::processGroups() {
  API::Workspace_const_sptr inWks = getProperty("InputWorkspace");

  API::WorkspaceGroup_const_sptr wks =
      boost::dynamic_pointer_cast<const API::WorkspaceGroup>(inWks);

  if (!wks) {
    g_log.error(
        "Could not retrieve the input workspace as a workspace group: ");
    return false;
  }

  // TODO: apply validators here on every input image/workspace
  for (size_t idx = 0; idx < wks->size(); idx++) {
    auto item = wks->getItem(0);
    auto mWS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(item);
    if (!mWS) {
      throw std::runtime_error("Unable to get a matrix workspace from the "
                               "element of te workspace group with title " +
                               item->getTitle());
    }

    auto wsValidator = boost::make_shared<Kernel::CompositeValidator>();
    wsValidator->add<API::CommonBinsValidator>();
    wsValidator->add<API::HistogramValidator>();
    // Probably we won't need this:
    // wsValidator->add<API::WorkspaceUnitValidator>("Label");
    const std::string validation = wsValidator->isValid(mWS);
    if (validation != "") {
      throw std::runtime_error(
          "Validation of input image / matrix workspace failed: " + validation);
    }
  }

  double minAngle = getProperty(PROP_MIN_PROJ_ANGLE);
  double maxAngle = getProperty(PROP_MAX_PROJ_ANGLE);
  auto angles = prepareProjectionAngles(wks, minAngle, maxAngle);

  // these values are expected as 'int' by the tompy routines
  const int ysize = static_cast<int>(ySizeProjections(wks));
  const int projSize = static_cast<int>(angles->size());
  const int xsize = static_cast<int>(xSizeProjections(wks));
  // total size of input data in voxels
  size_t totalInSize = ysize * projSize * xsize;
  // total size of the reconstructed volume
  size_t totalReconSize = ysize * ysize * xsize;

  auto inVol = prepareInputData(totalInSize, wks);
  auto reconVol = prepareDataVol(totalReconSize);

  int cor = getProperty(PROP_COR);
  auto centers = prepareCenters(cor, ysize);

  Mantid::Algorithms::Tomography::FBPTomopy(
      inVol->data(), ysize, projSize, xsize, centers->data(), angles->data(),
      reconVol->data(), xsize, ysize);

  size_t expectedVox = ysize * ysize * xsize;
  if (reconVol->size() != expectedVox) {
    std::stringstream stream;
    stream << std::string("The reconstructed volume data block does not "
                          "have the expected dimensions. It has ")
           << reconVol->size() << " voxels, whereas I was expecting: " << ysize
           << " slices by " << ysize << " rows by " << xsize
           << " columns = " << expectedVox << " voxels in total";
    throw std::runtime_error(stream.str());
  }

  const auto outputGrp = buildOutputWks(*reconVol, xsize, ysize, ysize);
  setProperty(PROP_OUTPUT_WS, outputGrp);

  g_log.notice() << "Finished reconstruction of volume from workspace "
                 << wks->getTitle() << " with " << projSize
                 << " input projections, " << ysize << " rows by " << xsize
                 << " columns.\n";

  return true;
}

std::unique_ptr<std::vector<float>>
ImggTomographicReconstruction::prepareProjectionAngles(
    API::WorkspaceGroup_const_sptr wks, double minAngle,
    double maxAngle) const {

  auto projCount = wks->size();
  auto angles = Kernel::make_unique<std::vector<float>>(projCount);

  auto vec = *angles;
  if (vec.empty())
    return angles;

  double factor = (maxAngle - minAngle);
  vec[0] = static_cast<float>(minAngle);
  for (size_t idx = 1; idx < projCount; ++idx) {
    vec[idx] = static_cast<float>(minAngle +
                                  factor * static_cast<double>(idx) /
                                      static_cast<double>(projCount - 1));
  }

  return angles;
}

std::unique_ptr<std::vector<float>>
ImggTomographicReconstruction::prepareInputData(
    size_t totalSize, API::WorkspaceGroup_const_sptr wksg) {
  auto data = prepareDataVol(totalSize);

  if (!wksg || 0 == wksg->size())
    return data;

  auto first = wksg->getItem(0);
  auto fwks = boost::dynamic_pointer_cast<API::MatrixWorkspace>(first);
  if (!fwks) {
    throw std::runtime_error(
        "Unable to get a matrix workspace from the first "
        "item of the input workspace group " +
        first->getTitle() +
        ". It must contain workspaces of type MatrixWorkspace");
  }

  size_t ysize = fwks->getNumberHistograms();
  size_t xsize = fwks->blocksize();
  const size_t oneSliceSize = xsize * ysize;

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int slice = 0; slice < static_cast<int>(wksg->size()); ++slice) {
    size_t startSlice = slice * oneSliceSize;
    for (size_t row = 0; row < ysize; ++row) {
      const auto &dataY = fwks->getSpectrum(row).readY();
      size_t startRow = startSlice + row * ysize;
      // MSVC will produce C4244 warnings in <xutility> (double=>float
      // converstion)
      // std::copy(dataY.begin(), dataY.end(), data->begin() + startRow);
      std::transform(dataY.begin(), dataY.end(), data->begin() + startRow,
                     DoubleToFloatStd());
    }
  }

  return data;
}

std::unique_ptr<std::vector<float>>
ImggTomographicReconstruction::prepareDataVol(size_t totalSize) {
  return Kernel::make_unique<std::vector<float>>(totalSize);
}

std::unique_ptr<std::vector<float>>
ImggTomographicReconstruction::prepareCenters(int cor, size_t totalSize) {
  auto centers = Kernel::make_unique<std::vector<float>>(totalSize);

  for (auto &cnt : *centers) {
    cnt = static_cast<float>(cor);
  }

  return centers;
}

size_t ImggTomographicReconstruction::xSizeProjections(
    API::WorkspaceGroup_const_sptr wks) const {
  auto first = wks->getItem(0);

  auto wksItem = boost::dynamic_pointer_cast<API::MatrixWorkspace>(first);
  if (!wksItem) {
    throw std::runtime_error("Unable to get a matrix workspace from the first "
                             "item of the workspace group " +
                             first->getTitle());
  }

  return wksItem->getNumberHistograms();
}

size_t ImggTomographicReconstruction::pSizeProjections(
    API::WorkspaceGroup_const_sptr wks) const {
  return wks->size();
}

size_t ImggTomographicReconstruction::ySizeProjections(
    API::WorkspaceGroup_const_sptr wks) const {
  auto first = wks->getItem(0);

  auto wksItem = boost::dynamic_pointer_cast<API::MatrixWorkspace>(first);
  if (!wksItem) {
    throw std::runtime_error("Unable to get a matrix workspace from the first "
                             "item of the workspace group " +
                             first->getTitle());
  }

  return wksItem->blocksize();
}

/**
 * Transfer data from a numpy/tomopy style data volume to a workspace
 * group of MatrixWorkspaces. This will typically have ysize slices
 * (images) of dimensions ysize rows by xsize columns.

 *
 * @param dataVol a 3D data volume that may have been generated as
 * output from a reconstruction
 * @param xsize image x dimension or number of columns (bins)
 * @param ysize image y dimension or number of rows (spectra)
 * @param sliceSize number of slices
 *
 * @return a workspace group with reconstruction slices
 */
API::WorkspaceGroup_sptr
ImggTomographicReconstruction::buildOutputWks(const std::vector<float> &dataVol,
                                              size_t xsize, size_t ysize,
                                              size_t sliceSize) {

  // auto wsGroup = boost::make_shared<API::WorkspaceGroup>();
  auto wsGroup = API::WorkspaceGroup_sptr(new API::WorkspaceGroup());
  wsGroup->setTitle("Reconstructed volume from imaging projection data");

  const size_t oneSliceSize = xsize * ysize;
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int slice = 0; slice < static_cast<int>(sliceSize); ++slice) {
    // individual slices as Workspace2D/MatrixWorkspace
    DataObjects::Workspace2D_sptr sliceWS =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            API::WorkspaceFactory::Instance().create("Workspace2D", ysize,
                                                     xsize + 1, xsize));
    size_t startSlice = slice * oneSliceSize;
    for (size_t row = 0; row < ysize; ++row) {
      auto &specRow = sliceWS->getSpectrum(row);
      auto &dataX = specRow.dataX();
      std::fill(dataX.begin(), dataX.end(), static_cast<double>(row));

      size_t startRow = startSlice + row * ysize;
      size_t endRow = startRow + xsize;
      auto &dataY = specRow.dataY();
      std::transform(dataVol.begin() + startRow, dataVol.begin() + endRow,
                     dataY.begin(), DoubleToFloatStd());
    }
    wsGroup->addWorkspace(sliceWS);
  }

  return wsGroup;
}

} // namespace Algorithms
} // namespace Mantid
