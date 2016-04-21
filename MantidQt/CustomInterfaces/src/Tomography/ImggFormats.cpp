#include "MantidQtCustomInterfaces/Tomography/ImggFormats.h"

#include <algorithm>

namespace MantidQt {
namespace CustomInterfaces {
namespace ImggFormats {

const static std::vector<std::vector<std::string>> extensions{
    {"fits", "fit"}, {"tiff", "tif"}, {"png"}, {"nxs"}};

const static std::vector<std::string> descriptions{
    "FITS: Flexible Image Transport System.",
    "TIFF: - Tagged Image File Format", "PNG: Portable Network Graphics",
    "NXTomo NeXus application definition"};

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
