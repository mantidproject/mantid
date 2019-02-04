// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SETSAMPLE_H_
#define MANTID_DATAHANDLING_SETSAMPLE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/PropertyManager_fwd.h"

namespace Mantid {
namespace Geometry {
class ReferenceFrame;
class SampleEnvironment;
} // namespace Geometry
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

  const Geometry::SampleEnvironment *
  setSampleEnvironment(API::MatrixWorkspace_sptr &workspace,
                       const Kernel::PropertyManager_const_sptr &args);
  void setSampleShape(API::MatrixWorkspace_sptr &workspace,
                      const Kernel::PropertyManager_const_sptr &args,
                      const Geometry::SampleEnvironment *sampleEnv);
  std::string
  tryCreateXMLFromArgsOnly(const Kernel::PropertyManager &args,
                           const Geometry::ReferenceFrame &refFrame);
  std::string
  createFlatPlateXML(const Kernel::PropertyManager &args,
                     const Geometry::ReferenceFrame &refFrame) const;
  std::string createCylinderLikeXML(const Kernel::PropertyManager &args,
                                    const Geometry::ReferenceFrame &refFrame,
                                    bool hollow) const;

  void runSetSampleShape(API::MatrixWorkspace_sptr &workspace,
                         const std::string &xml);
  void runChildAlgorithm(const std::string &name,
                         API::MatrixWorkspace_sptr &workspace,
                         const Kernel::PropertyManager &args);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SETSAMPLE_H_ */
