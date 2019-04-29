// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_CREATESAMPLESHAPE_H_
#define MANTID_DATAHANDLING_CREATESAMPLESHAPE_H_

//--------------------------------
// Includes
//--------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

/**
    This class allows the shape of the sample to be defined by using the allowed
   XML
    expressions

    @author Martyn Gigg, Tessella Support Services plc
    @date 13/03/2009
*/
class DLLExport CreateSampleShape : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CreateSampleShape"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Create a shape object to model the sample.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SetSample", "AbsorptionCorrection", "SetSampleMaterial",
            "CopySample"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Sample;"; }
  /// Algorithm's aliases
  const std::string alias() const override { return "SetSampleShape"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_CREATESAMPLESHAPE_H_*/
