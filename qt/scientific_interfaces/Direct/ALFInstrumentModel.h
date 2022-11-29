// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDetector.h"

#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace Mantid::Geometry {
class ComponentInfo;
class DetectorInfo;
} // namespace Mantid::Geometry

namespace MantidQt {

namespace MantidWidgets {
class IInstrumentActor;
}

namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL IALFInstrumentModel {

public:
  virtual ~IALFInstrumentModel() = default;

  virtual std::optional<std::string> loadAndTransform(std::string const &filename) = 0;

  virtual std::string loadedWsName() const = 0;
  virtual std::size_t runNumber() const = 0;

  virtual void setSelectedDetectors(Mantid::Geometry::ComponentInfo const &componentInfo,
                                    std::vector<std::size_t> const &detectorIndices) = 0;
  virtual std::vector<std::size_t> selectedDetectors() const = 0;

  virtual std::tuple<Mantid::API::MatrixWorkspace_sptr, std::vector<double>>
  generateOutOfPlaneAngleWorkspace(MantidQt::MantidWidgets::IInstrumentActor const &actor) const = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentModel final : public IALFInstrumentModel {

public:
  ALFInstrumentModel();

  std::optional<std::string> loadAndTransform(const std::string &filename) override;

  inline std::string loadedWsName() const noexcept override { return "ALFData"; };
  std::size_t runNumber() const override;

  void setSelectedDetectors(Mantid::Geometry::ComponentInfo const &componentInfo,
                            std::vector<std::size_t> const &detectorIndices) override;
  inline std::vector<std::size_t> selectedDetectors() const noexcept override { return m_detectorIndices; };

  std::tuple<Mantid::API::MatrixWorkspace_sptr, std::vector<double>>
  generateOutOfPlaneAngleWorkspace(MantidQt::MantidWidgets::IInstrumentActor const &actor) const override;

private:
  void collectXAndYData(MantidQt::MantidWidgets::IInstrumentActor const &actor, std::vector<double> &x,
                        std::vector<double> &y, std::vector<double> &e, std::vector<double> &twoThetas) const;
  void collectAndSortYByX(std::map<double, double> &xy, std::map<double, double> &xe, std::vector<double> &twoThetas,
                          MantidQt::MantidWidgets::IInstrumentActor const &actor,
                          Mantid::API::MatrixWorkspace_const_sptr const &workspace,
                          Mantid::Geometry::ComponentInfo const &componentInfo,
                          Mantid::Geometry::DetectorInfo const &detectorInfo) const;
  std::size_t numberOfDetectorsPerTube(Mantid::Geometry::ComponentInfo const &componentInfo) const;
  std::size_t numberOfTubes(MantidQt::MantidWidgets::IInstrumentActor const &actor) const;

  std::vector<std::size_t> m_detectorIndices;
};

} // namespace CustomInterfaces
} // namespace MantidQt
