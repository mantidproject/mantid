// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/IImageInfoWidget.h"
#include "MantidQtWidgets/Common/ImageInfoModel.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * A table widget containing information about the pixel the mouse is over in
 * an image
 */
class EXPORT_OPT_MANTIDQT_COMMON ImageInfoPresenter {

public:
  ImageInfoPresenter(IImageInfoWidget *view);

  inline const ImageInfoModel &model() { return *m_model; }

  void cursorAt(const double x, const double y, const double signal, const QMap<QString, QString> extraValues);
  void setWorkspace(const Mantid::API::Workspace_sptr &ws);
  void fillTableCells(const ImageInfoModel::ImageInfo &info);
  inline void setShowSignal(const bool showSignal) { m_showSignal = showSignal; }

private:
  std::unique_ptr<ImageInfoModel> m_model;
  IImageInfoWidget *m_view;
  bool m_showSignal;
};

} // namespace MantidWidgets
} // namespace MantidQt
