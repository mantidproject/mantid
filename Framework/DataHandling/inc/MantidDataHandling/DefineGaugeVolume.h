// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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

    @author Russell Taylor, Tessella
    @date 04/10/2010
*/
class DLLExport DefineGaugeVolume : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "DefineGaugeVolume"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Defines a geometrical shape object to be used as the gauge volume "
           "in the AbsorptionCorrection algorithm.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"AbsorptionCorrection"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Sample"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};
} // namespace DataHandling
} // namespace Mantid
