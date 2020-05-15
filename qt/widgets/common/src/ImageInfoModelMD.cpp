// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoModelMD.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 */
ImageInfoModelMD::ImageInfoModelMD(const Mantid::API::IMDWorkspace_sptr &ws)
    : m_workspace(ws) {}

// Creates a list containing pairs of strings with information about the
// coordinates in the workspace.
std::vector<std::string>
ImageInfoModelMD::getInfoList(const double x, const double y, const double z) {

  std::vector<std::string> list;
  addNameAndValue("x", x, 4, list);
  addNameAndValue("y", y, 4, list);
  addNameAndValue("Value", z, 4, list);

  return list;
}

} // namespace MantidWidgets
} // namespace MantidQt
