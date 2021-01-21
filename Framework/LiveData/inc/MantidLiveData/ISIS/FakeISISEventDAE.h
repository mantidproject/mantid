// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace LiveData {
/**
    Simulates ISIS histogram DAE. It runs continuously until canceled and
    listens to port 6789 for ISIS DAE commands.
*/
class FakeISISEventDAE : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FakeISISEventDAE"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\DataAcquisition"; }
  const std::vector<std::string> seeAlso() const override { return {"FakeISISHistoDAE"}; }

  /// Algorithm's summary
  const std::string summary() const override { return "Simulates ISIS event DAE."; }

private:
  void init() override;
  void exec() override;
};

} // namespace LiveData
} // namespace Mantid
