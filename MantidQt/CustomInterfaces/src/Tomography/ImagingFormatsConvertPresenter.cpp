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
  const std::string oext = ImggFormats::fileExtension(outFormat);
  try {
    goThroughDirRecur(inPS, inFormat, outPS, oext, depth);
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
 * @param outExt extension for output files
 * @param depth search depth remaining (for recursive calls).
 */
void ImagingFormatsConvertPresenter::goThroughDirRecur(
    const Poco::File &inFilePath, const std::string &inFormat,
    const Poco::File &outFilePath, const std::string &outExt, size_t depth) {

  Poco::DirectoryIterator end;

  for (Poco::DirectoryIterator it(inFilePath); it != end; ++it) {
    if (it->isDirectory()) {

      if (1 == depth)
        continue;

      // append to go into subdirectory:
      Poco::Path outPath(outFilePath.path());
      outPath.append(it.name());
      goThroughDirRecur(it.path(), inFormat, outPath, outExt, depth - 1);
    } else if (it->isFile()) {

      const std::string fname = it.name();
      if (ImggFormats::isFileExtension(fname, inFormat)) {
        // not removing source/original extension
        const std::string outFilename = outFilePath.path() + "." + outExt;
        m_view->convert(it.path().toString(), outFilename);
      }
    }
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
