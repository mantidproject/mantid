#include "MantidAlgorithms/ImggTomographicReconstruction.h"
#include "MantidAlgorithms/Tomography/FBPTomopy.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/make_unique.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ImggTomographicReconstruction)

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
      Kernel::make_unique<API::WorkspaceProperty<>>(
          PROP_INPUT_WS, "", Kernel::Direction::Input,
          boost::make_shared<API::WorkspaceUnitValidator>("Label")),
      "Workspace holding an image (with one spectrum per pixel row).");

  std::vector<std::string> methods{"FBP (tomopy)"};
  declareProperty(
      PROP_METHOD, methods.front(),
      boost::make_shared<Kernel::ListValidator<std::string>>(methods),
      "Reconstruction method", Kernel::Direction::Input);

  declareProperty(Kernel::make_unique<API::WorkspaceProperty<API::Workspace>>(
      PROP_OUTPUT_WS, "", Kernel::Direction::Output));

  auto zeroOrPosInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  zeroOrPosInt->setLower(-1);
  declareProperty(PROP_COR, -1, zeroOrPosInt,
                  "Center of rotation for the reconstruction (in pixels).");

  auto zeroOrPosDbl = boost::make_shared<Kernel::BoundedValidator<double>>();
  zeroOrPosDbl->setLower(0.0);
  declareProperty(PROP_RELAXATION_PARAM, 0.5, zeroOrPosDbl,
                  "Relaxation parameter for the reconstruction method.");

  zeroOrPosDbl->setLower(0);
  declareProperty(PROP_MAX_CORES, 0, zeroOrPosInt,
                  "Maximum number of cores to use for parallel runs. Leave "
                  "empty to use all available cores.");

  declareProperty(PROP_MIN_PROJ_ANGLE, 0, zeroOrPosDbl,
                  "Minimum projection angle.");

  declareProperty(PROP_MAX_PROJ_ANGLE, 180, zeroOrPosDbl,
                  "Maximum projection angle (assuming a uniform angle increase "
                  "from first to last projection.");
}

std::map<std::string, std::string>
ImggTomographicReconstruction::validateInputs() {
  std::map<std::string, std::string> result;

  API::Workspace_sptr inWks = getProperty(PROP_INPUT_WS);
  API::WorkspaceGroup_sptr inGrp =
      boost::dynamic_pointer_cast<API::WorkspaceGroup>(inWks);
  if (!inGrp) {
    result[PROP_INPUT_WS] = "The input workspace must be a group";
  } else {
    if (inGrp->size() < 2) {
      result[PROP_INPUT_WS] = "The input workspace must have at least two "
                              "workspaces (projection images)";
    }
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
  API::WorkspaceGroup_sptr wks = getProperty(PROP_INPUT_WS);

  double minAngle = getProperty(PROP_MIN_PROJ_ANGLE);
  double maxAngle = getProperty(PROP_MAX_PROJ_ANGLE);

  auto angles = prepareProjectionAngles(wks, minAngle, maxAngle);

  int ysize = static_cast<int>(ySizeProjections(wks));
  int projSize = static_cast<int>(angles->size());
  int xsize = static_cast<int>(xSizeProjections(wks));
  // total size of input data in voxels
  size_t totalSize = ysize * projSize * xsize;

  auto inVol = prepareInputData(totalSize, wks);
  auto reconVol = prepareDataVol(totalSize);
  auto centers = prepareCenters(getProperty(PROP_COR), ysize);

  // TODO: check consistency of center of rotation with input image size
  Mantid::Algorithms::Tomography::FBPTomopy(
      &inVol->front(), ysize, projSize, xsize, &centers->front(),
      &angles->front(), &reconVol->front(), xsize, ysize);

  size_t expectedVox = ysize * ysize * xsize;
  if (reconVol->size() != expectedVox) {
    std::stringstream stream;
    stream << std::string("The reconstructed volume data block does not "
                          "have the expected dimensions. It has ")
           << reconVol->size() << "voxels, but was expecting: " << ysize
           << " x " << ysize << " x " << xsize << " = " << expectedVox
           << " voxels in total";
    throw std::runtime_error(stream.str());
  }

  auto outputGrp = buildOutputWks(*reconVol, xsize, ysize);
  setProperty(PROP_OUTPUT_WS, outputGrp);

  g_log.notice() << "Reconstructe volume from workspace " << wks->getTitle()
                 << " with " << projSize << " input projections "
                 << ", " << ysize << " rows by " << xsize << "columns."
                 << std::endl;
}

std::unique_ptr<std::vector<float>>
ImggTomographicReconstruction::prepareProjectionAngles(
    API::WorkspaceGroup_const_sptr wks, double minAngle,
    double maxAngle) const {
  auto angles = Kernel::make_unique<std::vector<float>>(wks->size());

  auto vec = *angles;
  double factor = (maxAngle - minAngle);
  for (size_t idx = 0; idx < wks->size(); ++idx) {
    vec[idx] = static_cast<float>(minAngle +
                                  factor * static_cast<double>(idx) /
                                      static_cast<double>(idx - 1));
  }

  return angles;
}

std::unique_ptr<std::vector<float>>
ImggTomographicReconstruction::prepareInputData(size_t totalSize,
                                                API::WorkspaceGroup_sptr wsg) {
  auto data = prepareDataVol(totalSize);

  // TODO: fill in
  UNUSED_ARG(wsg);

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
 *
 * Output will produce ysize slices (images) of dimensions ysize rows by xsize
 *columns
 *
 */
API::WorkspaceGroup_sptr
ImggTomographicReconstruction::buildOutputWks(const std::vector<float> &dataVol,
                                              size_t xsize, size_t ysize) {

  auto wsGroup = boost::make_shared<API::WorkspaceGroup>();
  wsGroup->setTitle("Reconstructed volume from imaging projection data");

  // TODO: fill in
  UNUSED_ARG(dataVol);
  UNUSED_ARG(xsize);
  UNUSED_ARG(ysize);

  return wsGroup;
}

} // namespace Algorithms
} // namespace Mantid
