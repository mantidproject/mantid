// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoPresenter.h"
#include "MantidQtWidgets/Common/ImageInfoModelMD.h"
#include "MantidQtWidgets/Common/ImageInfoModelMatrixWS.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 */
ImageInfoPresenter::ImageInfoPresenter(IImageInfoWidget *view)
    : m_view(std::move(view)) {}

std::vector<std::string> ImageInfoPresenter::getInfoList(const double x,
                                                         const double y,
                                                         const double z,
                                                         bool includeValues) {
  return getModel()->getInfoList(x, y, z, includeValues);
}

void ImageInfoPresenter::createImageInfoModel(
    const Mantid::API::Workspace_sptr &ws) {
  if (auto matWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws))
    m_model = std::make_unique<ImageInfoModelMatrixWS>(matWS);
  else if (auto MDWS =
               std::dynamic_pointer_cast<Mantid::API::IMDWorkspace>(ws)) {
    m_model = std::make_unique<ImageInfoModelMD>(MDWS);
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
