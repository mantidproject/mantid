// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidKernel/PropertyManager_fwd.h"

namespace Mantid {
namespace Geometry {
class ReferenceFrame;
class SampleEnvironment;
} // namespace Geometry
namespace API {
class ExperimentInfo;
} // namespace API
namespace DataHandling {

/**
  High-level interface for setting sample metadata on a workspace.
*/
class MANTID_DATAHANDLING_DLL SetSample final : public API::Algorithm {
public:
  const std::string name() const override final;
  int version() const override final;
  const std::vector<std::string> seeAlso() const override {
    return {"SetSampleMaterial", "CreateSampleShape", "CopySample", "SetBeam"};
  }
  const std::string category() const override final;
  const std::string summary() const override final;

private:
  std::map<std::string, std::string> validateInputs() override final;
  void init() override final;
  void exec() override final;

  const Geometry::SampleEnvironment *setSampleEnvironmentFromFile(API::ExperimentInfo &experiment,
                                                                  const Kernel::PropertyManager_const_sptr &args);
  const Geometry::SampleEnvironment *
  setSampleEnvironmentFromXML(API::ExperimentInfo &experiment,
                              const Kernel::PropertyManager_const_sptr &canGeometryArgs,
                              const Kernel::PropertyManager_const_sptr &canMaterialArgs);
  void setSampleShape(API::ExperimentInfo &experiment, const Kernel::PropertyManager_const_sptr &args,
                      const Geometry::SampleEnvironment *sampleEnv);
  std::string tryCreateXMLFromArgsOnly(const Kernel::PropertyManager &args, const Geometry::ReferenceFrame &refFrame);
  std::string createFlatPlateXML(const Kernel::PropertyManager &args, const Geometry::ReferenceFrame &refFrame,
                                 const std::string &id = "sample-shape") const;
  std::string createFlatPlateHolderXML(const Kernel::PropertyManager &args,
                                       const Geometry::ReferenceFrame &refFrame) const;
  std::string createHollowCylinderHolderXML(const Kernel::PropertyManager &args,
                                            const Geometry::ReferenceFrame &refFrame) const;
  std::string createCylinderLikeXML(const Kernel::PropertyManager &args, const Geometry::ReferenceFrame &refFrame,
                                    bool hollow, const std::string &id = "sample-shape") const;
  std::string createSphereXML(const Kernel::PropertyManager &args) const;
  void validateGeometry(std::map<std::string, std::string> &errors, const Kernel::PropertyManager &args,
                        const std::string &flavour);
  void validateMaterial(std::map<std::string, std::string> &errors, const Kernel::PropertyManager &inputArgs,
                        const std::string &flavour);
  void assertNonNegative(std::map<std::string, std::string> &errors, const Kernel::PropertyManager &args,
                         const std::string &flavour, const std::vector<const std::string *> &keys);
  void setMaterial(ReadMaterial::MaterialParameters &materialParams, const Kernel::PropertyManager &materialArgs);
  Kernel::PropertyManager materialSettingsEnsureLegacyCompatibility(const Kernel::PropertyManager &materialArgs);
  bool isDictionaryPopulated(const Kernel::PropertyManager_const_sptr &dict) const;
};

} // namespace DataHandling
} // namespace Mantid
