#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtCustomInterfaces/Tomography/IImagingFormatsConvertView.h"
#include "MantidQtCustomInterfaces/Tomography/ImggFormats.h"
#include "MantidQtCustomInterfaces/Tomography/ImagingFormatsConvertPresenter.h"

#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("ImagingFormatsConvert");
}

ImagingFormatsConvertPresenter::ImagingFormatsConvertPresenter(
    IImagingFormatsConvertView *view)
    : m_view(view) {
  if (!m_view) {
    throw std::runtime_error(
        "Severe inconsistency found. Presenter created "
        "with an empty/null view (formats conversion interface). "
        "Cannot continue.");
  }
}

ImagingFormatsConvertPresenter::~ImagingFormatsConvertPresenter() { cleanup(); }

void ImagingFormatsConvertPresenter::cleanup() {}

void ImagingFormatsConvertPresenter::notify(Notification notif) {

  switch (notif) {

  case IImagingFormatsConvertPresenter::Init:
    processInit();
    break;

  case IImagingFormatsConvertPresenter::Convert:
    processConvert();
    break;

  case IImagingFormatsConvertPresenter::ShutDown:
    processShutDown();
    break;
  }
}

void ImagingFormatsConvertPresenter::processInit() {
  const std::vector<std::string> formats = {
      shortName(ImggFormats::FITS), shortName(ImggFormats::TIFF),
      shortName(ImggFormats::PNG), shortName(ImggFormats::JPG),
      shortName(ImggFormats::NXTomo)};

  m_view->setFormats(formats);
}

void ImagingFormatsConvertPresenter::processConvert() {
  const std::string inPS = m_view->inputPath();
  const std::string outPS = m_view->outputPath();
  size_t depth = m_view->maxSearchDepth();

  g_log.information() << "Converting images from path: " << inPS << " into "
                      << outPS << ", with depth " << depth << std::endl;

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
  }

  Poco::File outFilePath(outPS);
  if (!inFilePath.exists() || !inFilePath.isDirectory() ||
      !inFilePath.canWrite()) {
    m_view->userError(
        "Cannot write into the output path",
        "Please check the output path give: " + outPS +
            ". It must be an existing directory and it must be writeable.");
  }

  const std::string inFormat = m_view->inputFormatName();
  const std::string outFormat = m_view->outputFormatName();

  try {
    goThroughDirRecur(inPS, inFormat, outPS, outFormat, depth);
  } catch (std::runtime_error &rexc) {
    m_view->userError("Error while converting files",
                      "There was an error in the conversion process: " +
                          std::string(rexc.what()));
  }
}

void ImagingFormatsConvertPresenter::processShutDown() {
  m_view->saveSettings();
}

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
 */
void ImagingFormatsConvertPresenter::goThroughDirRecur(
    const Poco::File &inFilePath, const std::string &inFormat,
    const Poco::File &outFilePath, const std::string &outFormat, size_t depth) {

  const std::string outExt = ImggFormats::fileExtension(outFormat);

  Poco::DirectoryIterator end;
  for (Poco::DirectoryIterator it(inFilePath); it != end; ++it) {
    if (it->isDirectory()) {

      if (1 == depth)
        continue;

      // append to go into subdirectory:
      Poco::Path outPath(outFilePath.path());
      outPath.append(it.name());
      goThroughDirRecur(it.path(), inFormat, outPath, outFormat, depth - 1);
    } else if (it->isFile()) {

      const std::string fname = it.name();
      if (ImggFormats::isFileExtension(fname, inFormat)) {
        // intentionally not removing source/original extension
        Poco::Path path(outFilePath.path());
        path.append(it.name());
        const std::string outFilename = path.toString() + "." + outExt;
        convert(it.path().toString(), inFormat, outFilename, outFormat);
      }
    }
  }
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
void ImagingFormatsConvertPresenter::convert(
    const std::string &inputName, const std::string &inFormat,
    const std::string &outputName, const std::string &outFormat) const {

  if ("FITS" == inFormat) {
    auto inWks = loadFITS(inputName);
    m_view->writeImg(inWks, outputName, outFormat);
  } else if ("FITS" == outFormat) {
    auto inWks = m_view->loadImg(inputName, inFormat);
    saveFITS(inWks, outputName);
  } else {
    // other image formats
    m_view->convert(inputName, inFormat, outputName, outFormat);
  }
}

Mantid::API::MatrixWorkspace_sptr
ImagingFormatsConvertPresenter::loadFITS(const std::string &inputName) const {
  Mantid::API::MatrixWorkspace_sptr img;
  // child LoadFITS 'LoadImgAsRect'
  return img;
}

void ImagingFormatsConvertPresenter::saveFITS(
    Mantid::API::MatrixWorkspace_sptr image,
    const std::string &outputName) const {
  // child SaveFITS
}

} // namespace CustomInterfaces
} // namespace MantidQt
