// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/DateTimeValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <memory>

namespace Mantid {
namespace Algorithms {

class MANTID_ALGORITHMS_DLL ChangeTimeZero final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "ChangeTimeZero"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "The algorithm adjusts the zero time of a workspace."; }
  /// Check the inputs
  std::map<std::string, std::string> validateInputs() override;

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Create the output workspace
  Mantid::API::MatrixWorkspace_sptr createOutputWS(const Mantid::API::MatrixWorkspace_sptr &input, double startProgress,
                                                   double stopProgress);
  /// Get the time shift
  double getTimeShift(const API::MatrixWorkspace_sptr &ws) const;
  /// Shift the time of the logs
  void shiftTimeOfLogs(const Mantid::API::MatrixWorkspace_sptr &ws, double timeShift, double startProgress,
                       double stopProgress);
  /// Get the date and time of the first good frame of a workspace
  Mantid::Types::Core::DateAndTime getStartTimeFromWorkspace(const Mantid::API::MatrixWorkspace_sptr &ws) const;
  /// Can the string be transformed to double
  bool checkForDouble(const std::string &val) const;
  /// Can the string be transformed to a DateTime
  bool checkForDateTime(const std::string &val) const;

  /// Time shift the log of a double series property
  void shiftTimeInLogForTimeSeries(const Mantid::API::MatrixWorkspace_sptr &ws, Mantid::Kernel::Property const *prop,
                                   double timeShift) const;
  /// Time shift the log of a string property
  void shiftTimeOfLogForStringProperty(Mantid::Kernel::PropertyWithValue<std::string> *logEntry,
                                       double timeShift) const;
  // Shift the time of the neutrons
  void shiftTimeOfNeutrons(const Mantid::API::MatrixWorkspace_sptr &ws, double timeShift, double startProgress,
                           double stopProgress);

  bool isRelativeTimeShift(double offset) const;
  bool isAbsoluteTimeShift(const std::string &offset) const;

  const double m_defaultTimeShift = 0.0;
  const std::string m_defaultAbsoluteTimeShift;
};

} // namespace Algorithms
} // namespace Mantid
