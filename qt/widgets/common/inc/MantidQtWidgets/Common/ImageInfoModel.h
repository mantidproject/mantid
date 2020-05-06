// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Common/DisplayType.h"

#include <string>
#include <vector>

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

class EXPORT_OPT_MANTIDQT_COMMON ImageInfoModel {

public:
  ImageInfoModel(Mantid::API::Workspace_sptr &ws, DisplayType *type = nullptr);
  ~ImageInfoModel();

  std::vector<std::string> getInfoList(const double x, const double y,
                                       const double z);

private:
  std::vector<std::string>
  getMatrixWorkspaceInfo(const double x, const double y, const double z,
                         Mantid::API::MatrixWorkspace_sptr ws);
  std::vector<std::string> getMDWorkspaceInfo(const double x, const double y,
                                              const double z);

  void addNameAndValue(const std::string &label, const double value,
                       const int precision, std::vector<std::string> &list);

  Mantid::API::Workspace_sptr m_workspace;
  std::shared_ptr<Mantid::API::SpectrumInfo> m_spectrumInfo;
  std::shared_ptr<const Mantid::Geometry::Instrument> m_instrument;
  std::shared_ptr<const Mantid::Geometry::IComponent> m_source;
  std::shared_ptr<const Mantid::Geometry::IComponent> m_sample;
  DisplayType *m_displayType;
  double m_xMin;
  double m_xMax;
  double m_yMax;
};

} // namespace MantidWidgets
} // namespace MantidQt
