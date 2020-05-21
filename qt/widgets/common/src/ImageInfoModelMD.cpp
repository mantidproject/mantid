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

std::vector<std::string> ImageInfoModelMD::getInfoList(const double x,
                                                       const double y,
                                                       const double signal,
                                                       bool includeValues) {

  std::vector<std::string> list;
  addNameAndValue("x", x, 4, list, includeValues);
  addNameAndValue("y", y, 4, list, includeValues);
  addNameAndValue("Signal", signal, 4, list, includeValues);

  return list;
}

} // namespace MantidWidgets
} // namespace MantidQt
