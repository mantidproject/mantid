// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AsciiPointBase.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidReflectometry/DllConfig.h"

namespace Mantid {
namespace Reflectometry {
/**
Saves a file in three column format  from a 2D workspace
(Workspace2D class). SaveReflThreeColumnAscii is an algorithm but inherits from
the
AsciiPointBase class which provides the main implementation for the init() &
exec() methods.
Output is tab delimited Ascii point data without dq/q and with extra header
information.
*/
class MANTID_REFLECTOMETRY_DLL SaveReflThreeColumnAscii
    : public Mantid::API::AsciiPointBase,
      public Mantid::API::DeprecatedAlgorithm {

public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveReflThreeColumnAscii"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 2D workspace to a ascii file.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveReflCustomAscii", "SaveAscii"};
  }
  /// Algorithm's version for data output overriding a virtual method
  void data(std::ofstream &file, bool exportDeltaQ = false) override;
  SaveReflThreeColumnAscii() {
    this->useAlgorithm("SaveReflectometryAscii");
    this->deprecatedDate("2018-06-29");
  }

private:
  /// Return the file extension this algorthm should output.
  std::string ext() override { return ".dat"; }
  /// extra properties specifically for this
  void extraProps() override;
  /// write any extra information required
  void extraHeaders(std::ofstream &file) override;
};
} // namespace Reflectometry
} // namespace Mantid
