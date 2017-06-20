#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmNotification.h"

/**
 Implementation of various algorithm inner classes.

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
namespace Mantid {
namespace API {

Algorithm::AlgorithmNotification::AlgorithmNotification(
    const Algorithm *const alg)
    : Poco::Notification(), m_algorithm(alg) {}

const IAlgorithm *Algorithm::AlgorithmNotification::algorithm() const {
  return m_algorithm;
}

Algorithm::StartedNotification::StartedNotification(const Algorithm *const alg)
    : AlgorithmNotification(alg) {}

/// class name
std::string Algorithm::StartedNotification::name() const {
  return "StartedNotification";
}

Algorithm::FinishedNotification::FinishedNotification(
    const Algorithm *const alg, bool res)
    : AlgorithmNotification(alg), success(res) {}

/// class name
std::string Algorithm::FinishedNotification::name() const {
  return "FinishedNotification";
}

Algorithm::ProgressNotification::ProgressNotification(
    const Algorithm *const alg, double p, const std::string &msg,
    double estimatedTime, int progressPrecision)
    : AlgorithmNotification(alg), progress(p), message(msg),
      estimatedTime(estimatedTime), progressPrecision(progressPrecision) {}

/// class name
std::string Algorithm::ProgressNotification::name() const {
  return "ProgressNotification";
}

Algorithm::ErrorNotification::ErrorNotification(const Algorithm *const alg,
                                                const std::string &str)
    : AlgorithmNotification(alg), what(str) {}

/// class name
std::string Algorithm::ErrorNotification::name() const {
  return "ErrorNotification";
}

/// why the exception was raise
const char *Algorithm::CancelException::what() const noexcept {
  return "Algorithm terminated";
}

} // namespace API
} // namespace Mantid
