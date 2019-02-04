// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ICAT_ERRORHANDLING_H_
#define MANTID_ICAT_ERRORHANDLING_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdexcept>
#include <string>

//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
namespace ICat3 {
class ICATPortBindingProxy;
}

namespace Mantid {
namespace ICat {

/** CErrorHandling class responsible for handling errors in Mantid-ICat
   Algorithms.
    This algorithm  gives the datsets for a given investigations record

    @author Sofia Antony, ISIS Rutherford Appleton Laboratory
    @date 07/07/2010
 */
class CErrorHandling {
public:
  /** This method throws the error string returned by gsoap to mantid upper
   * layer
   *  @param icat ICat proxy object
   */
  static void throwErrorMessages(ICat3::ICATPortBindingProxy &icat);
};

/** a class for Throwing Session exception in Catalog module
 */
class SessionException : public std::runtime_error {
private:
  /// error string
  std::string m_error;

public:
  /// constructor
  SessionException(const std::string &error);
  /// return the error message
  const char *what() const noexcept override;
};
} // namespace ICat
} // namespace Mantid
#endif
