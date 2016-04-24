#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtAPI/BatchAlgorithmRunner.h"
#include "MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h"
#include "MantidQtCustomInterfaces/Tomography/ImageROIPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImageROIView.h"

using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("ImageROI");
}

const std::string ImageROIPresenter::g_wsgName =
    "__tomography_gui_stack_fits_viewer_sample_images";
const std::string ImageROIPresenter::g_wsgFlatsName =
    "__tomography_gui_stack_fits_viewer_flat_images";
const std::string ImageROIPresenter::g_wsgDarksName =
    "__tomography_gui_stack_fits_viewer_dark_images";

bool ImageROIPresenter::g_warnIfUnexpectedFileExtensions = false;

ImageROIPresenter::ImageROIPresenter(IImageROIView *view)
    : m_playStatus(false), m_stackPath(), m_view(view),
      m_model(new ImageStackPreParams()) {
  if (!m_view) {
    throw std::runtime_error("Severe inconsistency found. Presenter created "
                             "with an empty/null view (tomography interface). "
                             "Cannot continue.");
  }
  m_algRunner =
      Mantid::Kernel::make_unique<MantidQt::API::BatchAlgorithmRunner>();
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

  case IImageROIPresenter::ChangeImageType:
    processChangeImageType();
    break;

  case IImageROIPresenter::ChangeRotation:
    processChangeRotation();
    break;

  case IImageROIPresenter::UpdateImgIndex:
    processUpdateImgIndex();
    break;

  case IImageROIPresenter::PlayStartStop:
    processPlayStartStop();
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

  loadFITSStack(soid, g_wsgName, g_wsgFlatsName, g_wsgDarksName);

  connect(m_algRunner.get(), SIGNAL(batchComplete(bool)), this,
          SLOT(finishedLoadStack(bool)), Qt::QueuedConnection);

  m_view->enableActions(false);
  m_algRunner->executeBatchAsync();
}

void ImageROIPresenter::finishedLoadStack(bool error) {
  if (error) {
    m_view->userWarning("Could not load the stack of images",

                        "There was a failure while running the Mantid "
                        "algorithms that tried to load the stack of images. "
                        "Please check the error logs for details.");
    m_view->enableActions(true);
    return;
  }

  const auto &ads = Mantid::API::AnalysisDataService::Instance();
  try {
    m_stackSamples = ads.retrieveWS<Mantid::API::WorkspaceGroup>(g_wsgName);
  } catch (std::exception &e) {
    m_view->userWarning("Could not load the stack of sample images",

                        "Could not produce a workspace group for the "
                        "stack of sample images. Cannot "
                        "display this stack. Please check the error log "
                        "for further details. Error when trying to "
                        "retrieve the sample images workspace: " +
                            std::string(e.what()));
    m_view->enableActions(true);
    return;
  }

  // TODO: could be useful to do a check like this on wsg->size()?
  // if (wsg &&
  //     Mantid::API::AnalysisDataService::Instance().doesExist(wsg->name()) &&
  //     wsg->size() > 0 && imgs.size() >= wsg->size()) {
  //   return wsg;
  // } else {
  //   return Mantid::API::WorkspaceGroup_sptr();
  // }

  try {
    m_stackSamples = ads.retrieveWS<Mantid::API::WorkspaceGroup>(g_wsgName);
    Mantid::API::MatrixWorkspace_sptr ws =
        ads.retrieveWS<Mantid::API::MatrixWorkspace>(
            m_stackSamples->getNames()[0]);
  } catch (std::exception &exc) {
    m_view->userWarning(
        "Failed to load contents for at least the first sample image",
        "Could not load image contents for the first image file. "
        "An unrecoverable error happened when trying to load the "
        "image contents. Cannot display it. Error details: " +
            std::string(exc.what()));
    m_view->enableActions(true);
    return;
  }

  size_t imgCount = m_stackSamples->size();
  if (0 == imgCount) {
    m_view->userWarning(
        "Failed to load any FITS images - directory structure issue",
        "Even though a directory apparently holding a stack of images was "
        "found, "
        "it was not possible to load any image file correctly from: " +
            m_stackPath);
    m_view->enableActions(true);
    return;
  }

  // check flats and darks
  try {
    if (ads.doesExist(g_wsgFlatsName)) {
      m_stackFlats =
          ads.retrieveWS<Mantid::API::WorkspaceGroup>(g_wsgFlatsName);
    }
  } catch (std::runtime_error &exc) {
    m_view->userWarning("Failed to load the stack of flat (open beam) images",
                        "Could not produce a workspace group for the "
                        "stack of flat images. Cannot "
                        "display the flat images of this stack. "
                        "Please check the error log "
                        "for further details. Error when trying to "
                        "retrieve the flat images workspace:" +
                            std::string(exc.what()));
  }

  try {
    if (ads.doesExist(g_wsgDarksName)) {
      m_stackDarks =
          ads.retrieveWS<Mantid::API::WorkspaceGroup>(g_wsgDarksName);
    }
  } catch (std::runtime_error &exc) {
    m_view->userWarning(
        "Failed to load the stack of dark images",
        "Could not produce a workspace group for the "
        "stack of dark images. Cannot "
        "display the dark images of this stack. Please check the error log "
        "for further details. Error when trying to "
        "retrieve the dark images workspace:" +
            std::string(exc.what()));
  }

  m_view->showStack(m_stackSamples, m_stackFlats, m_stackDarks);
  m_view->enableActions(true);
}

void ImageROIPresenter::processChangeImageType() {
  m_view->updateImageType(m_view->currentImageTypeStack());
}

void ImageROIPresenter::processChangeRotation() {
  m_view->updateRotationAngle(m_view->currentRotationAngle());
}

void ImageROIPresenter::processUpdateImgIndex() {
  m_view->updateImgWithIndex(m_view->currentImgIndex());
}

void ImageROIPresenter::processPlayStartStop() {
  auto wsg = m_view->currentImageTypeStack();
  if (!wsg)
    return;

  if (wsg->size() <= 1) {
    m_view->userWarning(
        "Cannot \"play\" a single image",
        "The stack currently loaded has a single image. Cannot play it.");
  }

  if (m_playStatus) {
    m_view->playStop();
    m_playStatus = false;
    m_view->enableActions(true);
  } else {
    m_view->enableActions(false);
    m_playStatus = true;
    m_view->playStart();
  }
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

void ImageROIPresenter::loadFITSStack(const StackOfImagesDirs &soid,
                                      const std::string &wsgName,
                                      const std::string &wsgFlatsName,
                                      const std::string &wsgDarksName) {
  const std::vector<std::string> &imgs = soid.sampleFiles();
  if (imgs.empty())
    return;

  loadFITSList(imgs, wsgName);

  auto flats = soid.flatFiles();
  m_stackFlats = nullptr;
  loadFITSList(flats, wsgFlatsName);

  auto darks = soid.darkFiles();
  m_stackDarks = nullptr;
  loadFITSList(darks, wsgDarksName);
}

void ImageROIPresenter::loadFITSList(const std::vector<std::string> &imgs,
                                     const std::string &wsName) {

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
  }

  // This would be the alternative that loads images one by one (one
  // algorithm run per image file)
  // for (size_t i = 0; i < imgs.size(); ++i) {
  //  loadFITSImage(imgs[i], wsName);
  // }

  // Load all requested/supported image files using a list with their names
  try {
    const std::string allPaths = filterImagePathsForFITSStack(imgs);
    if (allPaths.empty()) {
      return;
    }
    loadFITSImage(allPaths, wsName);
  } catch (std::runtime_error &exc) {
    m_view->userWarning("Error trying to start the loading of FITS file(s)",
                        "There was an error which prevented the file(s) from "
                        "being loaded. Details: " +
                            std::string(exc.what()));
  }
}

/**
 * Produces a string with paths separated by commas. Takes the patsh from the
 * input paths string but selects only the ones that look consistent with the
 * supported format / extension.
 *
 * @param paths of the supposedly image files
 *
 * @return string with comma separated value (paths) ready to be passed as
 *input
 * to LoadFITS or similar algorithms
 */
std::string ImageROIPresenter::filterImagePathsForFITSStack(
    const std::vector<std::string> &paths) {
  std::string allPaths = "";

  // Let's take only the ones that we can effectively load
  const std::string expectedShort = "fit";
  const std::string expectedLong = "fits";
  const std::string summedSkipStr = "_SummedImg.";
  std::vector<std::string> unexpectedFiles, summedFiles;
  for (const auto &pathStr : paths) {
    const std::string extShort = pathStr.substr(pathStr.size() - 3);
    const std::string extLong = pathStr.substr(pathStr.size() - 4);
    // exception / sum images generated by some detectors
    if (std::string::npos != pathStr.find(summedSkipStr)) {
      summedFiles.push_back(pathStr);
    } else if (extShort != expectedShort && extLong != expectedLong) {
      unexpectedFiles.push_back(pathStr);
    } else {
      if (allPaths.empty()) {
        allPaths = pathStr;
      } else {
        allPaths.append(", " + pathStr);
      }
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
      m_view->userWarning("Files with invalid/unrecognized extension found in "
                          "the stack of images",
                          msg);
    }
    g_log.warning(msg);
  }
  if (!summedFiles.empty()) {
    std::string filesStrMsg = "";
    for (auto path : summedFiles) {
      filesStrMsg += path + "\n";
    }

    const std::string msg =
        "Found file(s) that look like summed images (have '" + summedSkipStr +
        "' in their name) in this "
        "stack ( " +
        m_stackPath + "). Ignoring them under the assumption that these are "
                      "note original images. Please make sure that this is "
                      "correct. The files ignored are: " +
        filesStrMsg;

    if (g_warnIfUnexpectedFileExtensions) {
      m_view->userWarning("Files that presumably are summed images have been "
                          "found in the stack of images",
                          msg);
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

  m_algRunner->addAlgorithm(alg);
}

} // namespace CustomInterfaces
} // namespace MantidQt
