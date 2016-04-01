#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h"
#include "MantidQtCustomInterfaces/Tomography/ImageROIPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImageROIView.h"

using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("ImageROI");
}

bool ImageROIPresenter::g_warnIfUnexpectedFileExtensions = false;

ImageROIPresenter::ImageROIPresenter(IImageROIView *view)
    : m_stackPath(), m_view(view), m_model(new ImageStackPreParams()) {
  if (!m_view) {
    throw std::runtime_error("Severe inconsistency found. Presenter created "
                             "with an empty/null view (tomography interface). "
                             "Cannot continue.");
  }
}

ImageROIPresenter::~ImageROIPresenter() { cleanup(); }

void ImageROIPresenter::cleanup() {}

void ImageROIPresenter::notify(Notification notif) {

  switch (notif) {

  case IImageROIPresenter::Init:
    processInit();
    break;

  case IImageROIPresenter::BrowseImgOrStack:
    processBrowseImg();
    break;

  case IImageROIPresenter::NewImgOrStack:
    processNewStack();
    break;

  case IImageROIPresenter::UpdateImgIndex:
    processUpdateImgIndex();
    break;

  case IImageROIPresenter::SelectCoR:
    processSelectCoR();
    break;

  case IImageROIPresenter::SelectROI:
    processSelectROI();
    break;

  case IImageROIPresenter::SelectNormalization:
    processSelectNormalization();
    break;

  case IImageROIPresenter::FinishedCoR:
    processFinishedCoR();
    break;

  case IImageROIPresenter::FinishedROI:
    processFinishedROI();
    break;

  case IImageROIPresenter::FinishedNormalization:
    processFinishedNormalization();
    break;

  case IImageROIPresenter::ResetCoR:
    processResetCoR();
    break;

  case IImageROIPresenter::ResetROI:
    processResetROI();
    break;

  case IImageROIPresenter::ResetNormalization:
    processResetNormalization();
    break;

  case IImageROIPresenter::ShutDown:
    processShutDown();
    break;
  }
}

void ImageROIPresenter::processInit() {
  ImageStackPreParams p;
  m_view->setParams(p);
}

void ImageROIPresenter::processBrowseImg() {
  const std::string path = m_view->askImgOrStackPath();

  if (path.empty())
    return;

  m_stackPath = path;
  processNewStack();
}

/**
 * Validates the input stack of images (directories and files), and
 * shows warning/error messages as needed. The outocome of the
 * validation can be checkec via isValid() on the returned stack of
 * images object.
 *
 * @param path user provided path to the stack of images
 *
 * @return a stack of images built from the path passed, not
 * necessarily correct (check with isValid())
 */
StackOfImagesDirs ImageROIPresenter::checkInputStack(const std::string &path) {
  StackOfImagesDirs soid(path, true);

  const std::string soiPath = soid.sampleImagesDir();
  if (soiPath.empty()) {
    m_view->userWarning("Error trying to find a stack of images",
                        "Could not find the sample images directory. The stack "
                        "of images is expected as: \n\n" +
                            soid.description());
  } else if (!soid.isValid()) {
    m_view->userWarning("Error while checking/validating the stack of images",
                        "The stack of images could not be loaded correctly. " +
                            soid.status());
  }

  return soid;
}

void ImageROIPresenter::processNewStack() {

  StackOfImagesDirs soid("");
  try {
    soid = checkInputStack(m_stackPath);
  } catch (std::exception &e) {
    // Poco::FileNotFoundException: this should never happen, unless
    // the open dir dialog misbehaves unexpectedly, or in tests
    m_view->userWarning("Error trying to open directories/files",
                        "The path selected via the dialog cannot be openend or "
                        "there was a problem while trying to access it. This "
                        "is an unexpected inconsistency. Error details: " +
                            std::string(e.what()));
  }

  if (!soid.isValid())
    return;

  std::vector<std::string> imgs = soid.sampleFiles();
  if (0 >= imgs.size()) {
    m_view->userWarning(
        "Error while trying to find image/projection files in the stack "
        "directories",
        "Could not find any (image) file in the samples subdirectory: " +
            soid.sampleImagesDir());
    return;
  }

  Mantid::API::WorkspaceGroup_sptr wsg = loadFITSStack(imgs);
  if (!wsg)
    return;

  size_t imgCount = wsg->size();
  if (0 == imgCount) {
    m_view->userWarning(
        "Failed to load any FITS images - directory structure issue",
        "Even though a directory apparently holding a stack of images was "
        "found, "
        "it was not possible to load any image file correctly from: " +
            m_stackPath);
    return;
  }

  m_view->showStack(wsg);

  // clean-up container group workspace? Not for now
  if (false && wsg)
    Mantid::API::AnalysisDataService::Instance().remove(wsg->getName());
}

void ImageROIPresenter::processUpdateImgIndex() {
  m_view->updateImgWithIndex(m_view->currentImgIndex());
}

void ImageROIPresenter::processSelectCoR() {
  m_view->changeSelectionState(IImageROIView::SelectCoR);
}

void ImageROIPresenter::processSelectROI() {
  m_view->changeSelectionState(IImageROIView::SelectROIFirst);
}

void ImageROIPresenter::processSelectNormalization() {
  m_view->changeSelectionState(IImageROIView::SelectNormAreaFirst);
}

void ImageROIPresenter::processFinishedCoR() {
  m_view->changeSelectionState(IImageROIView::SelectNone);
}

void ImageROIPresenter::processFinishedROI() {
  m_view->changeSelectionState(IImageROIView::SelectNone);
}

void ImageROIPresenter::processFinishedNormalization() {
  m_view->changeSelectionState(IImageROIView::SelectNone);
}

void ImageROIPresenter::processResetCoR() {
  m_view->resetCoR();
  m_view->changeSelectionState(IImageROIView::SelectNone);
}

void ImageROIPresenter::processResetROI() {
  m_view->resetROI();
  m_view->changeSelectionState(IImageROIView::SelectNone);
}

void ImageROIPresenter::processResetNormalization() {
  m_view->resetNormArea();
  m_view->changeSelectionState(IImageROIView::SelectNone);
}

void ImageROIPresenter::processShutDown() { m_view->saveSettings(); }

Mantid::API::WorkspaceGroup_sptr
ImageROIPresenter::loadFITSStack(const std::vector<std::string> &imgs) {
  if (imgs.empty())
    return Mantid::API::WorkspaceGroup_sptr();

  const std::string wsName = "__stack_fits_viewer_tomography_gui";
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  try {
    if (ads.doesExist(wsName)) {
      ads.remove(wsName);
    }
  } catch (std::runtime_error &exc) {
    m_view->userError(
        "Error accessing the analysis data service",
        "There was an error while accessing the Mantid analysis data service "
        "to check for the presence of (and remove if present) workspace '" +
            wsName + "'. This is a severe inconsistency . Error details:: " +
            std::string(exc.what()));
    return Mantid::API::WorkspaceGroup_sptr();
  }

  // This would be the alternative that loads images one by one (one
  // algorithm run per image file)
  // for (size_t i = 0; i < imgs.size(); ++i) {
  //  loadFITSImage(imgs[i], wsName);
  // }

  // Load all requested/supported image files using a list with their names
  try {
    const std::string allPaths = filterImagePathsForFITSStack(imgs);
    loadFITSImage(allPaths, wsName);
  } catch (std::runtime_error &exc) {
    m_view->userWarning("Error trying to open FITS file(s)",
                        "There was an error which prevented the file(s) from "
                        "being loaded. Details: " +
                            std::string(exc.what()));
    return Mantid::API::WorkspaceGroup_sptr();
  }

  Mantid::API::WorkspaceGroup_sptr wsg;
  try {
    wsg = ads.retrieveWS<Mantid::API::WorkspaceGroup>(wsName);
  } catch (std::exception &e) {
    throw std::runtime_error(
        "Could not produce a workspace group for the stack images. Cannot "
        "display it. Error details: " +
        std::string(e.what()));
  }

  if (wsg &&
      Mantid::API::AnalysisDataService::Instance().doesExist(wsg->name()) &&
      wsg->size() > 0 && imgs.size() >= wsg->size()) {
    return wsg;
  } else {
    return Mantid::API::WorkspaceGroup_sptr();
  }
}

/**
 * Produces a string with paths separated by commas. Takes the patsh from the
 * input paths string but selects only the ones that look consistent with the
 * supported format / extension.
 *
 * @param paths of the supposedly image files
 *
 * @return string with comma separated value (paths) ready to be passed as input
 * to LoadFITS or similar algorithms
 */
std::string ImageROIPresenter::filterImagePathsForFITSStack(
    const std::vector<std::string> &paths) {
  std::string allPaths = "";

  // Let's take only the ones that we can effectively load
  const std::string expectedShort = "fit";
  const std::string expectedLong = "fits";
  std::vector<std::string> unexpectedFiles;
  for (size_t i = 0; i < paths.size(); i++) {
    const std::string extShort = paths[i].substr(paths[i].size() - 3);
    const std::string extLong = paths[i].substr(paths[i].size() - 4);
    if (extShort == expectedShort || extLong == expectedLong) {
      if (allPaths.empty()) {
        allPaths = paths[i];
      } else {
        allPaths.append(", " + paths[i]);
      }
    } else {
      unexpectedFiles.push_back(paths[i]);
    }
  }

  // If needed, give a warning once, at the end
  if (!unexpectedFiles.empty()) {
    std::string filesStrMsg = "";
    for (auto path : unexpectedFiles) {
      filesStrMsg += path + "\n";
    }

    const std::string msg =
        "Found files with unrecognized or unsupported extension in this "
        "stack ( " +
        m_stackPath + "). Expected files with extension '" + expectedShort +
        "' or '" + expectedLong +
        "' the following file(s) were found (and not loaded):" + filesStrMsg;

    if (g_warnIfUnexpectedFileExtensions) {
      m_view->userWarning("Invalid files found in the stack of images", msg);
    }
    g_log.warning(msg);
  }

  return allPaths;
}

void ImageROIPresenter::loadFITSImage(const std::string &path,
                                      const std::string &wsName) {
  // get fits file into workspace and retrieve it from the ADS
  auto alg = Mantid::API::AlgorithmManager::Instance().create("LoadFITS");
  try {
    alg->initialize();
    alg->setPropertyValue("Filename", path);
    alg->setProperty("OutputWorkspace", wsName);
    // this is way faster when loading into a MatrixWorkspace
    alg->setProperty("LoadAsRectImg", true);
  } catch (std::exception &e) {
    throw std::runtime_error("Failed to initialize the mantid algorithm to "
                             "load images. Error description: " +
                             std::string(e.what()));
  }

  try {
    alg->execute();
  } catch (std::exception &e) {
    throw std::runtime_error(
        "Failed to load image. Could not load this file as a "
        "FITS image. Trying to load from this path: " +
        path + ". Error details: " + std::string(e.what()));
  }

  if (!alg->isExecuted()) {
    throw std::runtime_error(
        "Failed to load image correctly. Note that even though "
        "the image file has been loaded it seems to contain errors.");
  }

  try {
    Mantid::API::WorkspaceGroup_sptr wsg;
    Mantid::API::MatrixWorkspace_sptr ws;
    const auto &ads = Mantid::API::AnalysisDataService::Instance();
    wsg = ads.retrieveWS<Mantid::API::WorkspaceGroup>(wsName);
    ws = ads.retrieveWS<Mantid::API::MatrixWorkspace>(wsg->getNames()[0]);
  } catch (std::exception &e) {
    throw std::runtime_error(
        "Could not load image contents for file '" + path +
        "'. An unrecoverable error "
        "happened when trying to load the image contents. Cannot "
        "display it. Error details: " +
        std::string(e.what()));
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
