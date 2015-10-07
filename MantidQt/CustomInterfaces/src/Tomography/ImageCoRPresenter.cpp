#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h"
#include "MantidQtCustomInterfaces/Tomography/ImageCoRPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImageCoRView.h"

using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

ImageCoRPresenter::ImageCoRPresenter(IImageCoRView *view)
    : m_view(view), m_model(new ImageStackPreParams()) {
  if (!m_view) {
    throw std::runtime_error("Severe inconsistency found. Presenter created "
                             "with an empty/null view (tomography interface). "
                             "Cannot continue.");
  }
}

ImageCoRPresenter::~ImageCoRPresenter() { cleanup(); }

void ImageCoRPresenter::cleanup() {}

void ImageCoRPresenter::notify(Notification notif) {

  switch (notif) {

  case IImageCoRPresenter::Init:
    processInit();
    break;

  case IImageCoRPresenter::BrowseImgOrStack:
    processBrowseImg();
    break;

  case IImageCoRPresenter::NewImgOrStack:
    processNewStack();
    break;

  case IImageCoRPresenter::UpdateImgIndex:
    processUpdateImgIndex();
    break;

  case IImageCoRPresenter::SelectCoR:
    processSelectCoR();
    break;

  case IImageCoRPresenter::SelectROI:
    processSelectROI();
    break;

  case IImageCoRPresenter::SelectNormalization:
    processSelectNormalization();
    break;

  case IImageCoRPresenter::FinishedCoR:
    processFinishedCoR();
    break;

  case IImageCoRPresenter::FinishedROI:
    processFinishedROI();
    break;

  case IImageCoRPresenter::FinishedNormalization:
    processFinishedNormalization();
    break;

  case IImageCoRPresenter::ResetCoR:
    processResetCoR();
    break;

  case IImageCoRPresenter::ResetROI:
    processResetROI();
    break;

  case IImageCoRPresenter::ResetNormalization:
    processResetNormalization();
    break;

  case IImageCoRPresenter::ShutDown:
    processShutDown();
    break;
  }
}

void ImageCoRPresenter::processInit() {
  ImageStackPreParams p;
  m_view->setParams(p);
}

void ImageCoRPresenter::processBrowseImg() {
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
StackOfImagesDirs ImageCoRPresenter::checkInputStack(const std::string &path) {
  StackOfImagesDirs soid(path);

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

void ImageCoRPresenter::processNewStack() {

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
        "Error trying to find image/projection files in the stack directories",
        "Could not find any image file in the samples subdirectory: " +
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

void ImageCoRPresenter::processUpdateImgIndex() {
  m_view->updateImgWithIndex(m_view->currentImgIndex());
}

void ImageCoRPresenter::processSelectCoR() {
  m_view->changeSelectionState(IImageCoRView::SelectCoR);
}

void ImageCoRPresenter::processSelectROI() {
  m_view->changeSelectionState(IImageCoRView::SelectROIFirst);
}

void ImageCoRPresenter::processSelectNormalization() {
  m_view->changeSelectionState(IImageCoRView::SelectNormAreaFirst);
}

void ImageCoRPresenter::processFinishedCoR() {
  m_view->changeSelectionState(IImageCoRView::SelectNone);
}

void ImageCoRPresenter::processFinishedROI() {
  m_view->changeSelectionState(IImageCoRView::SelectNone);
}

void ImageCoRPresenter::processFinishedNormalization() {
  m_view->changeSelectionState(IImageCoRView::SelectNone);
}

void ImageCoRPresenter::processResetCoR() {
  m_view->resetCoR();
  m_view->changeSelectionState(IImageCoRView::SelectNone);
}

void ImageCoRPresenter::processResetROI() {
  m_view->resetROI();
  m_view->changeSelectionState(IImageCoRView::SelectNone);
}

void ImageCoRPresenter::processResetNormalization() {
  m_view->resetNormArea();
  m_view->changeSelectionState(IImageCoRView::SelectNone);
}

void ImageCoRPresenter::processShutDown() { m_view->saveSettings(); }

Mantid::API::WorkspaceGroup_sptr
ImageCoRPresenter::loadFITSStack(const std::vector<std::string> &imgs) {
  const std::string wsName = "__stack_fits_viewer_tomography_gui";
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  if (ads.doesExist(wsName)) {
    ads.remove(wsName);
  }
  for (size_t i = 0; i < imgs.size(); ++i) {
    loadFITSImage(imgs[i], wsName);
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
      imgs.size() == wsg->size()) {
    return wsg;
  } else {
    return Mantid::API::WorkspaceGroup_sptr();
  }
}

void ImageCoRPresenter::loadFITSImage(const std::string &path,
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
        "FITS image: " +
        std::string(e.what()));
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
