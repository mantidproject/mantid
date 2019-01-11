// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_MATERIALXMLPARSER_H_
#define MANTID_KERNEL_MATERIALXMLPARSER_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Material.h"
#include <istream>

namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace Kernel {

/**
  Read an XML definition of a Material and produce a new Material object
*/
class MANTID_KERNEL_DLL MaterialXMLParser {
public:
  static constexpr const char *MATERIAL_TAG = "material";

  Material parse(std::istream &istr) const;
  Material parse(Poco::XML::Element *node) const;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_MATERIALXMLPARSER_H_ */
