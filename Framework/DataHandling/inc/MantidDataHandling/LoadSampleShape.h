// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidDataHandling/MeshFileIO.h"
#include "MantidGeometry/Objects/MeshObject.h"

namespace Mantid {
namespace DataHandling {
/**  Load Shape into an instrument of a workspace

     The following file types are supported

       STL file with suffix .stl
       OFF file with suffix .off


@author Karl Palmen ISIS;
@date 26/02/2018
 */

class MANTID_DATAHANDLING_DLL LoadSampleShape : public Mantid::API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadSampleShape"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The algorithm loads a shape into the instrument of a workspace "
           "at the sample.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  /// Related algorithms
  const std::vector<std::string> seeAlso() const override {
    return {"CreateSampleShape", "CopySample", "SetSampleMaterial", "LoadSampleEnvironment"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Instrument"; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
