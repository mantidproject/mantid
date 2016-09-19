#include "MantidQtCustomInterfaces/Tomography/ImggFormatsConvertPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtCustomInterfaces/Tomography/IImggFormatsConvertView.h"
#include "MantidQtCustomInterfaces/Tomography/ImggFormats.h"

#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("ImggFormatsConvert");
}

ImggFormatsConvertPresenter::ImggFormatsConvertPresenter(
    IImggFormatsConvertView *view)
    : m_view(view) {
  if (!m_view) {
    throw std::runtime_error(
        "Severe inconsistency found. Presenter created "
        "with an empty/null view (formats conversion interface). "
        "Cannot continue.");
  }
}

ImggFormatsConvertPresenter::~ImggFormatsConvertPresenter() { cleanup(); }

void ImggFormatsConvertPresenter::cleanup() {}

void ImggFormatsConvertPresenter::notify(Notification notif) {

  switch (notif) {

  case IImggFormatsConvertPresenter::Init:
    processInit();
    break;

  case IImggFormatsConvertPresenter::Convert:
    processConvert();
    break;

  case IImggFormatsConvertPresenter::ShutDown:
    processShutDown();
    break;
  }
}

void ImggFormatsConvertPresenter::processInit() {
  const std::vector<std::string> formats{
      shortName(ImggFormats::FITS), shortName(ImggFormats::TIFF),
      shortName(ImggFormats::PNG), shortName(ImggFormats::JPG),
      shortName(ImggFormats::NXTomo)};

  const std::vector<bool> enableLoads{true, true, true, true, false};

  m_view->setFormats(formats, enableLoads);
}

void ImggFormatsConvertPresenter::processConvert() {
  const std::string inPS = m_view->inputPath();
  const std::string outPS = m_view->outputPath();
  size_t depth = m_view->maxSearchDepth();

  g_log.information() << "Trying to convert images from path: " << inPS
                      << " into " << outPS << ", with depth " << depth << '\n';

  const std::string emptyMsg = "Please specify an input and and output path.";
  if (inPS.empty()) {
    m_view->userError("Empty imput path", emptyMsg);
    return;
  }
  if (outPS.empty()) {
    m_view->userError("Empty output path", emptyMsg);
    return;
  }

  Poco::File inFilePath(inPS);
  if (!inFilePath.exists() || !inFilePath.isDirectory() ||
      !inFilePath.canRead()) {
    m_view->userError(
        "Cannot read from input path",
        "Please check the input path given: " + inPS +
            ". It must be an existing directory and it must be readable.");
    return;
  }

  Poco::File outFilePath(outPS);
  if (!outFilePath.exists() || !outFilePath.isDirectory() ||
      !outFilePath.canWrite()) {
    m_view->userError(
        "Cannot write into the output path",
        "Please check the output path given: " + outPS +
            ". It must be an existing directory and it must be writeable.");
    return;
  }

  const std::string inFormat = m_view->inputFormatName();
  const std::string outFormat = m_view->outputFormatName();

  try {
    size_t count = goThroughDirRecur(inPS, inFormat, outPS, outFormat, depth);
    if (count > 0) {
      std::stringstream msg;
      msg << "Finished converstion of images from path: " << inPS << " into "
          << outPS << ", with depth " << depth << ". Converted " << count
          << " images from format " << inFormat << " to format " << outFormat
          << '\n';
      g_log.notice() << msg.str();
      m_view->userWarning("Conversion finished successfully", msg.str());
    } else {
      std::stringstream msg;
      msg << "No images could be found in input path: " << inPS
          << " with format " << inFormat << ". 0 images converted.\n";
      g_log.notice() << msg.str();
      m_view->userWarning("No images could be found", msg.str());
    }
  } catch (std::runtime_error &rexc) {
    m_view->userError("Error while converting files",
                      "There was an error in the conversion process: " +
                          std::string(rexc.what()));
  }
}

void ImggFormatsConvertPresenter::processShutDown() { m_view->saveSettings(); }

/**
 * Search for images in the input path and transfers them to the
 * output path in the output format. For subdirectories, it recurs
 * through subdirectories "depth" levels.
 *
 * @param inFilePath input path where to search for image/stack files
 * @param inFormat input format to consider
 * @param outFilePath output path to write converted files
 * @param outFormat format for the output images
 * @param depth search depth remaining (for recursive calls).
 *
 * @return count of images processed/converted
 */
size_t ImggFormatsConvertPresenter::goThroughDirRecur(
    const Poco::File &inFilePath, const std::string &inFormat,
    const Poco::File &outFilePath, const std::string &outFormat, size_t depth) {

  const std::string outExt = ImggFormats::fileExtension(outFormat);

  size_t count = 0;
  Poco::DirectoryIterator end;
  for (Poco::DirectoryIterator it(inFilePath); it != end; ++it) {
    if (it->isDirectory()) {

      if (1 == depth)
        continue;

      // skip the output directory if it is nested in the input path!
      if (it.path().toString() == outFilePath.path())
        continue;

      // append, to delve into subdirectory:
      Poco::Path outPath(outFilePath.path());
      outPath.append(it.name());
      // create subdirectory in output path
      Poco::File(outPath).createDirectory();

      count +=
          goThroughDirRecur(it.path(), inFormat, outPath, outFormat, depth - 1);
    } else if (it->isFile()) {

      const std::string fname = it.name();
      if (ImggFormats::isFileExtension(fname, inFormat)) {
        // intentionally not removing source/original extension
        Poco::Path path(outFilePath.path());
        path.append(it.name());
        const std::string outFilename = path.toString() + "." + outExt;
        convert(it.path().toString(), inFormat, outFilename, outFormat);
        ++count;
      }
    }
  }
  return count;
}

/**
 * Create an output image from an input image, converting
 * formats. This uses the view (Qt classes) to process images in
 * traditional formats like TIFF, PNG, JPG. That should be moved to
 * this presenter when we have the Load/SaveImage algorithm.
 *
 * @param inputName name of input image
 * @param inFormat format the input image is in
 * @param outputName name of the output image to produce
 * @param outFormat format for the output image
 */
void ImggFormatsConvertPresenter::convert(const std::string &inputName,
                                          const std::string &inFormat,
                                          const std::string &outputName,
                                          const std::string &outFormat) const {

  if ("NXTomo" == outFormat) {
    convertToNXTomo(inputName, inFormat, outputName);
  } else if ("FITS" == outFormat) {
    convertToFITS(inputName, inFormat, outputName);
  } else {
    if ("FITS" == inFormat) {
      auto inWks = loadFITS(inputName);
      m_view->writeImg(inWks, outputName, outFormat);
    } else {
      // other image formats
      try {
        m_view->convert(inputName, inFormat, outputName, outFormat);
      } catch (std::runtime_error &rexc) {
        m_view->userError("Error in conversion",
                          "There was an error when converting the image " +
                              inputName + " from format " + inFormat +
                              " to format " + outFormat +
                              " into the output file " + outputName +
                              ". Details: " + rexc.what());
      }
    }
  }
}

void ImggFormatsConvertPresenter::convertToFITS(
    const std::string &inputName, const std::string &inFormat,
    const std::string &outputName) const {

  auto inWks = loadImg(inputName, inFormat);
  try {
    saveFITS(inWks, outputName);
  } catch (std::runtime_error &rexc) {
    m_view->userError(
        "Error in conversion",
        "There was an error when converting the image " + inputName +
            " (format " + inFormat +
            "), trying to write it into FITS format in the output file " +
            outputName + ". Details: " + rexc.what());
  }
}

void ImggFormatsConvertPresenter::convertToNXTomo(
    const std::string &inputName, const std::string &inFormat,
    const std::string &outputName) const {

  // loadNXTomo is not enabled (and there is no LoadNXTomo algorithm for now)
  auto inWks = loadImg(inputName, inFormat);
  try {
    saveNXTomo(inWks, outputName);
  } catch (std::runtime_error &rexc) {
    m_view->userError(
        "Error in conversion",
        "There was an error when converting the image " + inputName +
            " (format " + inFormat +
            "), trying to write it into NXTomo format in the output file " +
            outputName + ". Details: " + rexc.what());
  }
}

Mantid::API::MatrixWorkspace_sptr
ImggFormatsConvertPresenter::loadImg(const std::string &inputName,
                                     const std::string inFormat) const {
  Mantid::API::MatrixWorkspace_sptr wks;
  // TODO: This should become the algorithm LoadImage:
  // https://github.com/mantidproject/mantid/issues/6843
  if ("FITS" == inFormat) {
    wks = loadFITS(inputName);
  } else {
    wks = m_view->loadImg(inputName, inFormat);
  }
  return wks;
}

Mantid::API::MatrixWorkspace_sptr
ImggFormatsConvertPresenter::loadFITS(const std::string &inputName) const {

  const std::string wksGrpName = "__fits_img_to_convert";
  if (Mantid::API::AnalysisDataService::Instance().doesExist(wksGrpName)) {
    auto algDel =
        Mantid::API::AlgorithmManager::Instance().create("DeleteWorkspace");
    algDel->setChild(true);
    algDel->setPropertyValue("Workspace", wksGrpName);
    algDel->execute();
  }
  // Just run LoadFITS
  auto alg = Mantid::API::AlgorithmManager::Instance().create("LoadFITS");
  alg->initialize();
  alg->setChild(true);
  alg->setProperty("Filename", inputName);
  alg->setProperty("OutputWorkspace", wksGrpName);
  alg->setProperty("LoadAsRectImg", true);
  alg->execute();

  if (!alg->isExecuted()) {
    throw std::runtime_error("Failed to execute the algorithm "
                             "LoadFITS to load the image file '" +
                             inputName + "' in FITS format.");
  }

  Mantid::API::Workspace_sptr wks = alg->getProperty("OutputWorkspace");
  auto grp = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(wks);
  if (!grp) {
    throw std::runtime_error(
        "When loading a FITS image: " + inputName +
        ", failed to retrieve the output workspace (group) after "
        "executing the algorithm LoadFITS.");
  }
  auto imgWorkspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      grp->getItem(grp->size() - 1));

  return imgWorkspace;
}

void ImggFormatsConvertPresenter::saveFITS(
    Mantid::API::MatrixWorkspace_sptr image,
    const std::string &outputName) const {
  // Just run LoadFITS
  auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveFITS");
  alg->initialize();
  alg->setProperty("InputWorkspace", image);
  alg->setProperty("Filename", outputName);
  alg->execute();

  if (!alg->isExecuted()) {
    throw std::runtime_error("Failed to execute the algorithm SaveFITS to save "
                             "an image in FITS format into the file '" +
                             outputName + ".");
  }
}

void ImggFormatsConvertPresenter::saveNXTomo(
    Mantid::API::MatrixWorkspace_sptr image,
    const std::string &outputName) const {

  // Run the algorithm SaveNXTomo
  auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveNXTomo");
  alg->initialize();
  alg->setProperty("InputWorkspaces", image);
  alg->setProperty("Filename", outputName);
  alg->setProperty("OverwriteFile", false);
  alg->execute();

  if (!alg->isExecuted()) {
    throw std::runtime_error(
        "Failed to execute the algorithm SaveNXTomo to save "
        "images in NXTomo format into the file '" +
        outputName + ".");
  }

  g_log.information()
      << "Save NXTomo file (overwriting if the file already existed: "
      << outputName << '\n';
}

} // namespace CustomInterfaces
} // namespace MantidQt
