// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/Component.h"

namespace Mantid {

namespace DataHandling {

class MANTID_DATAHANDLING_DLL ScaleInstrumentComponent : public API::Algorithm {
public:
  ScaleInstrumentComponent();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "ScaleInstrumentComponent"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Scales all detectors in a component around the component position.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"RotateInstrumentComponent", "SetInstrumentParameter", "MoveInstrumentComponent"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Instrument"; }

  std::map<std::string, std::string> validateInputs() override;

private:
  /// Implement abstract Algorithm methods
  void init() override;

  /// Implement abstract Algorithm methods
  void exec() override;

  Mantid::Geometry::ComponentInfo *m_componentInfo = nullptr;
  Mantid::Geometry::IComponent_const_sptr m_comp;
};

} // namespace DataHandling
} // namespace Mantid
