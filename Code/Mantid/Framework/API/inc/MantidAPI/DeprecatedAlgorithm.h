/*
 * DeprecatedAlgorithm.h
 *
 *  Created on: Feb 25, 2011
 *      Author: pf9
 */

#ifndef DEPRECATEDALGORITHM_H_
#define DEPRECATEDALGORITHM_H_

#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"
#include <string>

namespace Mantid
{
namespace API
{

class DLLExport DeprecatedAlgorithm
{
public:
  DeprecatedAlgorithm();
  virtual ~DeprecatedAlgorithm();
  const std::string deprecationMsg(const IAlgorithm *);
protected:
  void useAlgorithm(const std::string &);
  void deprecatedDate(const std::string &);
private:
  /// The algorithm to use instead of this one.
  std::string m_replacementAlgorithm;
  /// The date that the algorithm was first deprecated.
  std::string m_deprecatdDate;
};

} // namespace API
} // namespace Mantid

#endif /* DEPRECATEDALGORITHM_H_ */
