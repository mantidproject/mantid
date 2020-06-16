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

std::vector<QString> ImageInfoPresenter::getInfoList(const double x,
                                                     const double y,
                                                     const double z) {
  bool getValues(true);
  if (x == DBL_MAX || y == DBL_MAX || z == DBL_MAX)
    getValues = false;
  return getModel()->getInfoList(x, y, z, getValues);
}

void ImageInfoPresenter::createImageInfoModel(
    const std::string &workspace_type) {
  if (workspace_type == "MATRIX")
    m_model = std::make_unique<ImageInfoModelMatrixWS>();
  else {
    m_model = std::make_unique<ImageInfoModelMD>();
  }
}

void ImageInfoPresenter::setWorkspace(const Mantid::API::Workspace_sptr &ws) {
  m_model->setWorkspace(ws);
}

} // namespace MantidWidgets
} // namespace MantidQt
