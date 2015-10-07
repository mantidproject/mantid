#ifndef DEPRECATEDALGORITHM_H_
#define DEPRECATEDALGORITHM_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"
#include <string>

namespace Mantid {
namespace API {

/**
 Class for marking algorithms as deprecated.

 @author Peter Peterson, NScD Oak Ridge National Laboratory
 @date 25/02/2011

 Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_API_DLL DeprecatedAlgorithm {
public:
  DeprecatedAlgorithm();
  virtual ~DeprecatedAlgorithm();
  const std::string deprecationMsg(const IAlgorithm *);

public:
  void useAlgorithm(const std::string &, const int version = -1);
  void deprecatedDate(const std::string &);

private:
  /// The algorithm to use instead of this one.
  std::string m_replacementAlgorithm;
  /// Replacement version, -1 indicates latest
  int m_replacementVersion;
  /// The date that the algorithm was first deprecated.
  std::string m_deprecatdDate;
};

} // namespace API
} // namespace Mantid

#endif /* DEPRECATEDALGORITHM_H_ */
