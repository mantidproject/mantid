#include "MantidQtCustomInterfaces/Tomography/StackOfImagesDirs.h"

#include <boost/algorithm/string.hpp>

#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>

namespace MantidQt {
namespace CustomInterfaces {

const std::string StackOfImagesDirs::g_descrComplex =
    "A directory (folder) that contains subdirectories with names "
    "starting with:\n- 'Data' (for sample images),\n- 'Flat' (for white "
    "bean images),\n- 'Dark' (for dark images)\nThe first one is "
    "mandatory whereas the other two are optional.";

const std::string StackOfImagesDirs::g_descrSimple =
    "A directory containing (readable) image files.";

const std::string StackOfImagesDirs::g_descrBoth = "Two alternatives: \n\n"
                                                   "Simple: " +
                                                   g_descrSimple +
                                                   "\n\n"
                                                   "Or Full:\n\n" +
                                                   g_descrComplex + "\n";

const std::string StackOfImagesDirs::g_sampleNamePrefix = "data";
const std::string StackOfImagesDirs::g_flatNamePrefix = "flat";
const std::string StackOfImagesDirs::g_darkNamePrefix = "dark";
const std::string StackOfImagesDirs::g_processedNamePrefix = "processed";
const std::string StackOfImagesDirs::g_prefilteredNamePrefix = "pre_filtered";

StackOfImagesDirs::StackOfImagesDirs(const std::string &path,
                                     bool allowSimpleLayout)
    : m_valid(false), m_statusDescStr("Constructed, no checks done yet.") {
  findStackDirs(path, allowSimpleLayout);
}

std::string StackOfImagesDirs::description() const { return m_descr; }

std::string StackOfImagesDirs::status() const {
  if (m_valid)
    return "Stack of images is correct";
  else
    return "There are errors in the directories and/or files. " +
           m_statusDescStr;
}

std::vector<std::string> StackOfImagesDirs::sampleFiles() const {
  return findImgFiles(m_sampleDir);
}
std::vector<std::string> StackOfImagesDirs::flatFiles() const {
  return findImgFiles(m_flatDir);
}
std::vector<std::string> StackOfImagesDirs::darkFiles() const {
  return findImgFiles(m_darkDir);
}

void StackOfImagesDirs::findStackDirs(const std::string &path,
                                      bool allowSimpleLayout) {
  if (allowSimpleLayout) {
    m_descr = g_descrBoth;
  } else {
    m_descr = g_descrComplex;
  }

  if (path.empty())
    return;

  Poco::File dir(path);
  if (!dir.isDirectory() || !dir.exists())
    return;

  bool found = false;
  // go for the simple layout first (just image files in a flat tree).
  if (allowSimpleLayout) {
    found = findStackFilesSimple(dir);
  }

  if (!found) {
    findStackDirsComplex(dir);
  }
}

/**
 * Tries to find a stack of images with the simple layout: all sample
 * files are in the directory given.
 *
 * @param dir directory which is the base path for the stack of images, that is,
 * which contains image files.
 *
 * @return whether the stack has been found succesfully
 */
bool StackOfImagesDirs::findStackFilesSimple(Poco::File &dir) {
  Poco::DirectoryIterator end;
  for (Poco::DirectoryIterator it(dir); it != end; ++it) {
    // presence of one regular file is enough
    if (it->isFile()) {
      m_sampleDir = dir.path();
      m_flatDir = "";
      m_darkDir = "";
      m_valid = true;
    }
  }

  return m_valid;
}

/**
 * Tries to find a stack of images with the full or complex
 * layout. Assumes that the path given has been checked. In a full
 * layout the files are arranged in up to three subdirectories:
 *
 * - path/data*: sample images
 * - path/flat*: flat images
 * - path/dark*: dark images
 *
 * @param dir directory to use as base path for the stack of images
 *
 * @return whether the stack has been found succesfully
 */
bool StackOfImagesDirs::findStackDirsComplex(Poco::File &dir) {
  Poco::DirectoryIterator end;
  for (Poco::DirectoryIterator it(dir); it != end; ++it) {
    if (!it->isDirectory()) {
      continue;
    }

    const std::string name = it.name();

    // case insensitive comparison against expected pattersn: data_*, flat_*,
    // dark_*, etc.
    if (boost::iequals(name.substr(0, g_sampleNamePrefix.length()),
                       g_sampleNamePrefix)) {
      m_sampleDir = it.path().toString();
    } else if (boost::iequals(name.substr(0, g_flatNamePrefix.length()),
                              g_flatNamePrefix)) {
      m_flatDir = it.path().toString();
    } else if (boost::iequals(name.substr(0, g_darkNamePrefix.length()),
                              g_darkNamePrefix)) {
      m_darkDir = it.path().toString();
    }
  }

  if (m_sampleDir.empty()) {
    m_statusDescStr = "The the sample images directory (" + g_sampleNamePrefix +
                      "...) has not been found.";
    return false;
  }

  // can be valid only if we get here. There must be at least one entry that is
  // a file
  Poco::Path samplesPath(m_sampleDir);
  for (Poco::DirectoryIterator it(samplesPath); it != end; ++it) {
    if (it->isFile()) {
      m_valid = true;
      break;
    }
  }

  if (m_valid) {
    m_statusDescStr = "all checks passed";
  } else {
    m_statusDescStr = "No files were found in the sample images directory (" +
                      g_sampleNamePrefix + "...).";
  }

  return m_valid;
}

std::vector<std::string>
StackOfImagesDirs::findImgFiles(const std::string &path) const {
  std::vector<std::string> fnames;
  if (path.empty()) {
    return fnames;
  }

  try {
    Poco::File dir(path);
    if (!dir.isDirectory() || !dir.exists())
      return fnames;

    // as an alternative could also use Poco::Glob to find the files
    Poco::DirectoryIterator it(dir);
    Poco::DirectoryIterator end;
    while (it != end) {
      // TODO: filter names by extension?
      // const std::string name = it.name();
      if (it->isFile()) {
        fnames.push_back(it.path().toString());
      }

      ++it;
    }

    // this assumes the usual sorting of images of a stack (directory): a
    // prefix,
    // and a sequence number (with a fixed number of digits).
    std::sort(fnames.begin(), fnames.end());
  } catch (std::runtime_error &) {
    // it's fine if the files are not found, not readable, etc.
  }

  return fnames;
}

} // namespace CustomInterfaces
} // namespace MantidQt
