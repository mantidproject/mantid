#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h"
#include "MantidQtCustomInterfaces/Tomography/ImageCoRPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImageCoRView.h"
#include "MantidQtCustomInterfaces/Tomography/StackOfImagesDirs.h"

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

void ImageCoRPresenter::processInit() {}

void ImageCoRPresenter::processBrowseImg() {
  const std::string path = m_view->askImgOrStackPath();

  if (!path.empty()) {
    processNewStack();
  }
}

void ImageCoRPresenter::processNewStack() {
  const std::string pstr = m_view->stackPath();

  StackOfImagesDirs soid(pstr);

  const std::string soiPath = soid.sampleImagesDir();
  if (soiPath.empty()) {
    m_view->userWarning("Error trying to find image stack",
                        "Could not find the sample images directory. The stack "
                        "of images is expected as: \n\n" + soid.description());
    return;
  }

  std::vector<std::string> imgs = soid.sampleFiles();
  if (0 >= imgs.size()) {
    m_view->userWarning("Error trying to find image/projection files in the stack directories",
                        "Could not find any image file in the samples subdirectory: " + soiPath);
    return;
  }

  const std::string imgPath = imgs.front();

  Mantid::API::WorkspaceGroup_sptr wsg = loadFITSStack(imgPath);
  if (!wsg)
    return;

  size_t imgCount = wsg->size();
  if (0 == imgCount) {
    m_view->userWarning(
        "Failed to load any FITS images - directory structure issue",
        "Even though a directory apprently holding a stack of images was found, "
        "it was not possible to load any image file correctly from: " +
                        soiPath);
    return;
  }

  m_view->showStack(wsg);

  // clean-up container group workspace
  if (wsg)
    Mantid::API::AnalysisDataService::Instance().remove(wsg->getName());
}

void ImageCoRPresenter::processSelectCoR() {}

void ImageCoRPresenter::processSelectROI() {}

void ImageCoRPresenter::processSelectNormalization() {}

void ImageCoRPresenter::processFinishedCoR() {}

void ImageCoRPresenter::processFinishedROI() {}

void ImageCoRPresenter::processFinishedNormalization() {}

void ImageCoRPresenter::processResetCoR() {}

void ImageCoRPresenter::processResetROI() {}

void ImageCoRPresenter::processResetNormalization() {}

void ImageCoRPresenter::processShutDown() { m_view->saveSettings(); }

Mantid::API::WorkspaceGroup_sptr
ImageCoRPresenter::loadFITSStack(const std::string &path) {
  // TODO: go through directory
  return loadFITSImage(path);
}

Mantid::API::WorkspaceGroup_sptr
ImageCoRPresenter::loadFITSImage(const std::string &path) {
  // get fits file into workspace and retrieve it from the ADS
  auto alg = Mantid::API::AlgorithmManager::Instance().create("LoadFITS");
  alg->initialize();
  alg->setPropertyValue("Filename", path);
  std::string wsName = "__fits_ws_tomography_gui";
  alg->setProperty("OutputWorkspace", wsName);
  // this is way faster when loading into a MatrixWorkspace
  alg->setProperty("LoadAsRectImg", true);
  try {
    alg->execute();
  } catch (std::exception &e) {
    throw std::runtime_error(
        "Failed to load image. Could not load this file as a "
        "FITS image: " +
        std::string(e.what()));
    return Mantid::API::WorkspaceGroup_sptr();
  }
  if (!alg->isExecuted()) {
    throw std::runtime_error(
        "Failed to load image correctly. Note that even though "
        "the image file has been loaded it seems to contain errors.");
  }
  Mantid::API::WorkspaceGroup_sptr wsg;
  Mantid::API::MatrixWorkspace_sptr ws;
  try {
    const auto &ads = Mantid::API::AnalysisDataService::Instance();
    wsg = ads.retrieveWS<Mantid::API::WorkspaceGroup>(wsName);
    ws = ads.retrieveWS<Mantid::API::MatrixWorkspace>(wsg->getNames()[0]);
  } catch (std::exception &e) {
    throw std::runtime_error(
        "Could not load image contents. An unrecoverable error "
        "happened when trying to load the image contents. Cannot "
        "display it. Error details: " +
        std::string(e.what()));
  }

  // draw image from workspace
  if (wsg && ws &&
      Mantid::API::AnalysisDataService::Instance().doesExist(ws->name())) {
    return wsg;
  } else {
    return Mantid::API::WorkspaceGroup_sptr();
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
