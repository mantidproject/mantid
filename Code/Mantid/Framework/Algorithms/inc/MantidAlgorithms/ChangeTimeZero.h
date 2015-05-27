#ifndef MANTID_ALGORITHMS_CHANGETIMEZERO_H_
#define MANTID_ALGORITHMS_CHANGETIMEZERO_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/DateTimeValidator.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Algorithms {

class DLLExport ChangeTimeZero : public API::Algorithm {
public:
  ChangeTimeZero();
  ~ChangeTimeZero();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "ChangeTimeZero"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "The algorithm adjusts the zero time of a workspace.";
  }
  /// Check the inputs
  virtual std::map<std::string, std::string> validateInputs();

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Utility"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
  /// Create the output workspace
  Mantid::API::MatrixWorkspace_sptr
  createOutputWS(Mantid::API::MatrixWorkspace_sptr input, double startProgress, double stopProgress);
  /// Get the time shift
  double getTimeShift(API::MatrixWorkspace_sptr ws) const;
  /// Shift the time of the logs
  void shiftTimeOfLogs(Mantid::API::MatrixWorkspace_sptr ws, double timeShift, double startProgress,
                                         double stopProgress);
  /// Get the date and time of the first good frame of a workspace
  Mantid::Kernel::DateAndTime
  getStartTimeFromWorkspace(Mantid::API::MatrixWorkspace_sptr ws) const;
  /// Can the string be transformed to double
  bool checkForDouble(std::string val) const;
  /// Can the string be transformed to a DateTime
  bool checkForDateTime(const std::string& val) const;

  /// Time shift the log of a double series property
  void shiftTimeInLogForTimeSeries(Mantid::API::MatrixWorkspace_sptr ws,
                                   Mantid::Kernel::Property *logEntry,
                                   double timeShift) const;
  /// Time shift the log of a string property
  void shiftTimeOfLogForStringProperty(
      Mantid::Kernel::PropertyWithValue<std::string> *logEntry,
      double timeShift) const;
  // Shift the time of the neutrons
  void shiftTimeOfNeutrons(Mantid::API::MatrixWorkspace_sptr ws,
                           double timeShift, double startProgress, double stopProgress);

  bool isRelativeTimeShift(double offset) const;
  bool isAbsoluteTimeShift(const std::string &offset) const;

  const double m_defaultTimeShift;
  const std::string m_defaultAbsoluteTimeShift;
};

} // namespace Mantid
} // namespace Algorithms

#endif /* MANTID_ALGORITHMS_CHANGEPULSETIME_H_ */
