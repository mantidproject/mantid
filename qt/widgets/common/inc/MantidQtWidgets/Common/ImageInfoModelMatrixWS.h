// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidQtWidgets/Common/ImageInfoModel.h"
#include <tuple>

namespace Mantid {
namespace Geometry {
class IComponent;
class Instrument;
} // namespace Geometry
namespace API {
class SpectrumInfo;
}
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {

/**
 * Model to support looking up information about a given point with a
 * MatrixWorkspace
 */
class EXPORT_OPT_MANTIDQT_COMMON ImageInfoModelMatrixWS : public ImageInfoModel {

public:
  ImageInfoModelMatrixWS(Mantid::API::MatrixWorkspace_sptr workspace);

  ImageInfoModel::ImageInfo info(const double x, const double y, const double signal) const override;

private:
  void setUnitsInfo(ImageInfoModel::ImageInfo *info, int infoIndex, const size_t wsIndex, const double x) const;
  std::tuple<Mantid::Kernel::DeltaEMode::Type, double> efixedAt(const size_t wsIndex) const;
  void cacheWorkspaceInfo();
  void createItemNames();

private:
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  const Mantid::API::SpectrumInfo *m_spectrumInfo;
  std::shared_ptr<const Mantid::Geometry::Instrument> m_instrument;
  std::shared_ptr<const Mantid::Geometry::IComponent> m_source;
  std::shared_ptr<const Mantid::Geometry::IComponent> m_sample;
  ImageInfoModel::ImageInfo::StringItems m_names;
  Mantid::Kernel::Unit_sptr m_xunit;
  bool m_xIsTOF;
};

} // namespace MantidWidgets
} // namespace MantidQt
