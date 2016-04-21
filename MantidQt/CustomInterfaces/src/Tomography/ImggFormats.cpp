#include "MantidQtCustomInterfaces/Tomography/ImggFormats.h"

#include <algorithm>

namespace MantidQt {
namespace CustomInterfaces {
namespace ImggFormats {

const static std::vector<std::string> shortNames{"FITS", "TIFF", "PNG", "JPEG",
                                                 "NXTomo"};

const static std::vector<std::vector<std::string>> extensions{
    {"fits", "fit"}, {"tiff", "tif"}, {"png"}, {"jpg", "jpeg"}, {"nxs"}};

const static std::vector<std::string> descriptions{
    "FITS: Flexible Image Transport System.",
    "TIFF: - Tagged Image File Format", "PNG: Portable Network Graphics",
    "JPEG: Joint Photographic Experts Group",
    "NXTomo NeXus application definition"};

/**
 * Possible common/accepted file extensions for a format
 *
 * @return extensions (without the dot, like "fits") in no particular
 * order, lowercase
 */
std::string shortName(Format fmt) { return shortNames.at(fmt); }

/**
 * Get sequence number or ID of the format
 *
 * @param shortName short name of the format (example: "FITS")
 *
 * @return the format id number, -1 if it is unknown
 */
size_t formatID(const std::string &shortName) {
  auto pos = std::find(shortNames.begin(), shortNames.end(), shortName);

  if (shortNames.end() == pos) {
    return 0;
  }

  return pos - shortNames.begin();
}

/**
 * Possible common/accepted file extensions for a format
 *
 * @return extensions (without the dot, like "fits") in no particular
 * order, lowercase
 */
std::vector<std::string> fileExtensions(Format fmt) {
  return extensions.at(fmt);
}

/**
 * The first-choice file extension for a format (given by
 * name). Example: FITS => fit
 *
 * @param name string of the format
 *
 * @return extension as a string, empty if the format is unknown.
 */
std::string fileExtension(const std::string &format) {
  auto pos = std::find(shortNames.begin(), shortNames.end(), format);

  if (shortNames.end() == pos) {
    return "";
  }

  return extensions[pos - shortNames.begin()].front();
}

/**
 * Find out whether this extension is one of the accepted format
 * extensions. This is case insensitive
 *
 * @param extension a file name extension like .fits
 * @param fmt a file format
 */
bool isFileExtension(std::string extension, Format fmt) {
  std::string lowExt = extension;
  std::transform(lowExt.begin(), lowExt.end(), lowExt.begin(), ::tolower);

  const auto &valid = extensions.at(fmt);
  return (valid.end() != std::find(valid.begin(), valid.end(), lowExt));
}

bool isFileExtension(std::string extension, const std::string &shortName) {
  size_t idx = formatID(shortName);

  return isFileExtension(extension, static_cast<Format>(idx));
}

/**
 * Human readable description / long name of the format
 *
 * @return description string to show in log messages, interfaces,
 * etc.
 */
std::string description(Format fmt) { return descriptions.at(fmt); }

} // namespace ImggFormats
} // namespace CustomInterfaces
} // namespace MantidQt
