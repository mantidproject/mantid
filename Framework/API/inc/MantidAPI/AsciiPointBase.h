// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <fstream>
#include <string>

namespace Mantid {
namespace API {
/**
Abstract base class for some ascii format save algorithms that print point data
and dq/q.
AsciiPointBase is a framework for some algorithms. It overrides exec and init
and provides full
implementation for any subclasses and as such any subclasses should only provide
implementations
for the additional abstract and virtual methods provided by this class.
*/
class DLLExport AsciiPointBase : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override = 0;
  /// Algorithm's version for identification overriding a virtual method
  int version() const override = 0;
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  /// Return the file extension this algorthm should output.
  virtual std::string ext() = 0;
  /// Return if the line should start with a separator
  virtual bool leadingSep() { return true; }
  /// Add extra properties
  virtual void extraProps() = 0;
  /// write any extra information required
  virtual void extraHeaders(std::ofstream &file) = 0;

  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
  /// print the appropriate value to file
  void outputval(double val, std::ofstream &file, bool leadingSep = true);

protected:
  /// write the main content of the data
  virtual void data(std::ofstream &file, bool exportDeltaQ = true);
  /// Retrieves the separator property
  virtual void appendSeparatorProperty();
  /// The separator character
  char m_sep{'\t'};
  size_t m_length{0};
  API::MatrixWorkspace_const_sptr m_ws;
};

} // namespace API
} // namespace Mantid
