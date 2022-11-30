// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//--------------------------------
// Includes
//--------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace API {
class ExperimentInfo;
}
namespace DataHandling {

/**
    This class allows the shape of the sample to be defined by using the allowed
   XML expressions

    @author Martyn Gigg, Tessella Support Services plc
    @date 13/03/2009
*/
class MANTID_DATAHANDLING_DLL CreateSampleShape final : public API::Algorithm {
public:
  static void setSampleShape(API::ExperimentInfo &expt, const std::string &shapeXML);

public:
  const std::string name() const override { return "CreateSampleShape"; }
  const std::string summary() const override { return "Create a shape object to model the sample."; }

  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SetSample", "AbsorptionCorrection", "SetSampleMaterial", "CopySample"};
  }
  const std::string category() const override { return "Sample;"; }
  const std::string alias() const override { return "SetSampleShape"; }

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
