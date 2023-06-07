// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFData.h"
#include "DetectorTube.h"
#include "DllConfig.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDetector.h"

#include <map>
#include <memory>
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

  virtual std::string loadedWsName() const = 0;

  virtual void setData(ALFData const &dataType, Mantid::API::MatrixWorkspace_sptr const &workspace) = 0;
  virtual bool hasData(ALFData const &dataType) const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr data(ALFData const &dataType) const = 0;

  virtual void replaceSampleWorkspaceInADS(Mantid::API::MatrixWorkspace_sptr const &workspace) const = 0;

  virtual std::size_t run(ALFData const &dataType) const = 0;

  virtual bool isALFData(Mantid::API::MatrixWorkspace_const_sptr const &workspace) const = 0;
  virtual bool binningMismatch() const = 0;
  virtual bool axisIsDSpacing() const = 0;

  virtual bool setSelectedTubes(std::vector<DetectorTube> tubes) = 0;
  virtual bool addSelectedTube(DetectorTube const &tube) = 0;
  virtual bool hasSelectedTubes() const = 0;
  virtual std::vector<DetectorTube> selectedTubes() const = 0;

  virtual std::vector<double> twoThetasClosestToZero() const = 0;

  // The algorithms used to load and normalise the Sample
  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> loadProperties(std::string const &filename) const = 0;
  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  normaliseByCurrentProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const = 0;
  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> rebinToWorkspaceProperties() const = 0;
  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> divideProperties() const = 0;
  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  replaceSpecialValuesProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const = 0;
  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  convertUnitsProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const = 0;

  // The algorithms used to produce an Out of plane angle workspace
  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  createWorkspaceAlgorithmProperties(MantidQt::MantidWidgets::IInstrumentActor const &actor) = 0;
  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  scaleXProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const = 0;
  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  rebunchProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentModel final : public IALFInstrumentModel {

public:
  ALFInstrumentModel();

  inline std::string loadedWsName() const noexcept override { return "ALFData"; };

  void setData(ALFData const &dataType, Mantid::API::MatrixWorkspace_sptr const &workspace) override;
  bool hasData(ALFData const &dataType) const override;
  Mantid::API::MatrixWorkspace_sptr data(ALFData const &dataType) const override;

  void replaceSampleWorkspaceInADS(Mantid::API::MatrixWorkspace_sptr const &workspace) const override;

  std::size_t run(ALFData const &dataType) const override;

  bool isALFData(Mantid::API::MatrixWorkspace_const_sptr const &workspace) const override;
  bool binningMismatch() const override;
  bool axisIsDSpacing() const override;

  bool setSelectedTubes(std::vector<DetectorTube> tubes) override;
  bool addSelectedTube(DetectorTube const &tube) override;
  bool hasSelectedTubes() const override;
  inline std::vector<DetectorTube> selectedTubes() const noexcept override { return m_tubes; };

  inline std::vector<double> twoThetasClosestToZero() const noexcept override { return m_twoThetasClosestToZero; };

  // The algorithms used to load and normalise the Sample
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> loadProperties(std::string const &filename) const override;
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  normaliseByCurrentProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const override;
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> rebinToWorkspaceProperties() const override;
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> divideProperties() const override;
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  replaceSpecialValuesProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const override;
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  convertUnitsProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const override;

  // The algorithms used to produce an Out of plane angle workspace
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  createWorkspaceAlgorithmProperties(MantidQt::MantidWidgets::IInstrumentActor const &actor) override;
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  scaleXProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const override;
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  rebunchProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const override;

private:
  void setSample(Mantid::API::MatrixWorkspace_sptr const &sample);
  void setVanadium(Mantid::API::MatrixWorkspace_sptr const &vanadium);

  std::size_t runNumber(Mantid::API::MatrixWorkspace_sptr const &workspace) const;

  bool tubeExists(DetectorTube const &tube) const;

  void collectXAndYData(MantidQt::MantidWidgets::IInstrumentActor const &actor, std::vector<double> &x,
                        std::vector<double> &y, std::vector<double> &e);
  void collectAndSortYByX(std::map<double, double> &xy, std::map<double, double> &xe,
                          MantidQt::MantidWidgets::IInstrumentActor const &actor,
                          Mantid::API::MatrixWorkspace_const_sptr const &workspace,
                          Mantid::Geometry::ComponentInfo const &componentInfo,
                          Mantid::Geometry::DetectorInfo const &detectorInfo);

  inline std::size_t numberOfTubes() const noexcept { return m_tubes.size(); }

  Mantid::API::MatrixWorkspace_sptr m_emptyInstrument;
  Mantid::API::MatrixWorkspace_sptr m_sample;
  Mantid::API::MatrixWorkspace_sptr m_vanadium;
  std::vector<DetectorTube> m_tubes;
  std::vector<double> m_twoThetasClosestToZero;
};

} // namespace CustomInterfaces
} // namespace MantidQt
