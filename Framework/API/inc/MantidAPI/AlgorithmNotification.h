#ifndef MANTID_API_ALGORITHM_NOTIFICATION_H_
#define MANTID_API_ALGORITHM_NOTIFICATION_H_

#include "MantidAPI/DllConfig.h"

// forward declarations
namespace Poco {
class Notification;
}

namespace Mantid {
namespace API {

/**
 Classes for algorithm notification and specialized exception.

 Further text from Gaudi file.......
 The base class provides utility methods for accessing
 standard services (event data service etc.); for declaring
 properties which may be configured by the job options
 service; and for creating Child Algorithms.
 The only base class functionality which may be used in the
 constructor of a concrete algorithm is the declaration of
 member variables as properties. All other functionality,
 i.e. the use of services and the creation of Child Algorithms,
 may be used only in initialise() and afterwards (see the
 Gaudi user guide).

 Copyright &copy; 2007-2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://github.com/mantidproject/mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

// forward declarations
class Algorithm;
class IAlgorithm;

/// Base class for algorithm notifications
class MANTID_API_DLL Algorithm::AlgorithmNotification
    : public Poco::Notification {
public:
  AlgorithmNotification(const Algorithm *const alg);
  const IAlgorithm *algorithm() const;

private:
  const IAlgorithm *const m_algorithm; ///< The algorithm
};

/// StartedNotification is sent when the algorithm begins execution.
class MANTID_API_DLL Algorithm::StartedNotification
    : public AlgorithmNotification {
public:
  StartedNotification(const Algorithm *const alg);
  std::string name() const override;
};

/// FinishedNotification is sent after the algorithm finishes its execution
class MANTID_API_DLL Algorithm::FinishedNotification
    : public AlgorithmNotification {
public:
  FinishedNotification(const Algorithm *const alg, bool res);
  std::string name() const override;
  bool success; ///< true if the finished algorithm was successful or false if
                /// it failed.
};

/// An algorithm can report its progress by sending ProgressNotification. Use
/// Algorithm::progress(double) function to send a progress notification.
class MANTID_API_DLL Algorithm::ProgressNotification
    : public AlgorithmNotification {
public:
  /// Constructor
  ProgressNotification(const Algorithm *const alg, double p,
                       const std::string &msg, double estimatedTime,
                       int progressPrecision);
  std::string name() const override;
  double progress;       ///< Current progress. Value must be between 0 and 1.
  std::string message;   ///< Message sent with notification
  double estimatedTime;  ///<Estimated time to completion
  int progressPrecision; ///<Digits of precision to the progress (after the
                         /// decimal).
};

/// ErrorNotification is sent when an exception is caught during execution of
/// the algorithm.
class MANTID_API_DLL Algorithm::ErrorNotification
    : public AlgorithmNotification {
public:
  /// Constructor
  ErrorNotification(const Algorithm *const alg, const std::string &str);
  std::string name() const override;
  std::string what; ///< message string
};

/// CancelException is thrown to cancel execution of the algorithm. Use
/// Algorithm::cancel() to
/// terminate an algorithm. The execution will only be stopped if
/// Algorithm::exec() method calls
/// periodically Algorithm::interuption_point() which checks if
/// Algorithm::cancel() has been called
/// and throws CancelException if needed.
class MANTID_API_DLL Algorithm::CancelException : public std::exception {
public:
  /// Returns the message string.
  const char *what() const noexcept override;
};
} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ALGORITHM_NOTIFICATION_H_*/
