// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_EXCEPTION_H_
#define MANTID_LIVEDATA_EXCEPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdexcept>

namespace Mantid {
namespace LiveData {
namespace Exception {

/** An exception that can be thrown by an ILiveListener implementation to
    notify LoadLiveData that it is not yet ready to return data. This could
    be, for example, because it has not yet completed its initialisation step
    or if the instrument from which data is being read is not in a run.
    LoadLiveData will ask for data again after a short delay.  Other exceptions
    thrown by the listener will have the effect of stopping the algorithm.
*/
class NotYet : public std::runtime_error {
public:
  /** Constructor
   *  @param message A description of the exceptional condition
   */
  explicit NotYet(const std::string &message) : std::runtime_error(message) {}
};

} // namespace Exception
} // namespace LiveData
} // namespace Mantid

#endif // MANTID_LIVEDATA_EXCEPTION_H_
