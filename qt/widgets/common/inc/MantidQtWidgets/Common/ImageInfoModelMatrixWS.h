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
#include "MantidQtWidgets/Common/ImageInfoModel.h"

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

class EXPORT_OPT_MANTIDQT_COMMON ImageInfoModelMatrixWS
    : public ImageInfoModel {

public:
  ImageInfoModelMatrixWS(const Mantid::API::MatrixWorkspace_sptr &ws);

  /** Creates a list with information about the coordinates in the workspace.
  @param x: x data coordinate
  @param specNum: the spectrum number of the coordinate
  @param signal: the signal value at x, y
  @param includeValues: if false the list will contain "-" for each of the
                        numeric values
  @return a vector containing pairs of strings
  */
  std::vector<std::string> getInfoList(const double x, const double specNum,
                                       const double signal,
                                       bool includeValues = true) override;

private:
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  const Mantid::API::SpectrumInfo *m_spectrumInfo;
  std::shared_ptr<const Mantid::Geometry::Instrument> m_instrument;
  std::shared_ptr<const Mantid::Geometry::IComponent> m_source;
  std::shared_ptr<const Mantid::Geometry::IComponent> m_sample;
  double m_xMin;
  double m_xMax;
};

} // namespace MantidWidgets
} // namespace MantidQt
