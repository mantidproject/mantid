#ifndef MANTID_ALGORITHMS_CHANGETIMEZERO_H_
#define MANTID_ALGORITHMS_CHANGETIMEZERO_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

// Forward declarations
class Mantid::Kernel::DateAndTime;

namespace Mantid {
namespace Algorithms {

/** This algorithm allows for shifting the absolute time of a workspace.
 * Both the logs and the time stamps of the data are changed.
 *
 * @author
 * @date 11/05/2015
 */
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
  virtual const std::string category() const {
    return "Utility";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
  /// Create the output workspace
  Mantid::API::MatrixWorkspace_sptr createOutputWS(Mantid::API::MatrixWorkspace_const_sptr input);
  /// Get the time shift
  double getTimeShift(API::MatrixWorkspace_const_sptr ws) const;
  /// Shift the time of the logs
  void shiftTimeOfLogs(Mantid::API::MatrixWorkspace_sptr ws, double timeShift);
  /// Get the date and time of the first good frame of a workspace
  Mantid::Kernel::DateAndTime getStartTimeFromWorkspace(Mantid::API::MatrixWorkspace_const_sptr ws) const;
};

} // namespace Mantid
} // namespace Algorithms

#endif /* MANTID_ALGORITHMS_CHANGEPULSETIME_H_ */
