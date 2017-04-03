#include "MantidQtCustomInterfaces/Tomography/ImggFormats.h"

#include <algorithm>
#include <map>

namespace MantidQt {
namespace CustomInterfaces {
namespace ImggFormats {

// Names are often acronyms, so writing them uppercase
const static std::vector<std::string> shortNames{"FITS", "TIFF", "PNG", "JPEG",
                                                 "NXTomo"};

// Map format name to vector of accepted/recognized extensions
const static std::map<std::string, std::vector<std::string>> extensions{
    {"FITS", {"fits", "fit"}},
    {"TIFF", {"tiff", "tif"}},
    {"PNG", {"png"}},
    {"JPEG", {"jpg", "jpeg"}},
    {"NXTomo", {"nxs"}}};

const static std::vector<std::string> descriptions{
    "FITS: Flexible Image Transport System.",
    "TIFF: - Tagged Image File Format", "PNG: Portable Network Graphics",
    "JPEG: Joint Photographic Experts Group",
    "NXTomo: NXTomo NeXus application definition"};

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
 * The first-choice file extension for a format (given by
 * name). Example: FITS => fit
 *
 * @param format name of the format (short string)
 *
 * @return extension as a string, empty if the format is unknown.
 */
std::string fileExtension(const std::string &format) {
  const auto &exts = extensions.at(format);

  return exts.front();
}

/**
 * Find out whether this extension is one of the accepted format
 * extensions. This is case insensitive
 *
 * @param extension a file name extension like .fits, or a filename
 * @param shortName name of a file format
 */
bool isFileExtension(const std::string &extension,
                     const std::string &shortName) {
  size_t pos = extension.find_last_of('.');
  std::string lowExt = extension.substr(pos + 1);
  std::transform(lowExt.begin(), lowExt.end(), lowExt.begin(), ::tolower);

  const auto &valid = extensions.at(shortName);
  return (valid.end() != std::find(valid.begin(), valid.end(), lowExt));
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
