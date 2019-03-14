// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DEPRECATEDALGORITHM_H_
#define DEPRECATEDALGORITHM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include <string>

namespace Mantid {
namespace API {

/**
 Class for marking algorithms as deprecated.

 @author Peter Peterson, NScD Oak Ridge National Laboratory
 @date 25/02/2011
*/
class MANTID_API_DLL DeprecatedAlgorithm {
public:
  DeprecatedAlgorithm();
  virtual ~DeprecatedAlgorithm();
  std::string deprecationMsg(const IAlgorithm *);

public:
  void useAlgorithm(const std::string &, const int version = -1);
  void deprecatedDate(const std::string &);

private:
  /// The algorithm to use instead of this one.
  std::string m_replacementAlgorithm;
  /// Replacement version, -1 indicates latest
  int m_replacementVersion;
  /// The date that the algorithm was first deprecated.
  std::string m_deprecatedDate;
};

} // namespace API
} // namespace Mantid

#endif /* DEPRECATEDALGORITHM_H_ */
