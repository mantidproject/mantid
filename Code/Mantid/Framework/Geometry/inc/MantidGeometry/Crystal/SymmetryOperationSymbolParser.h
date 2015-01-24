#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONSYMBOLPARSER_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONSYMBOLPARSER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include <boost/regex.hpp>

namespace Mantid {
namespace Geometry {

/**
  @class SymmetryOperationSymbolParser

  This is a parser for symmetry operation symbols in the Jones
  faithful representation. It creates matrix and a vector component
  from the given symbol. First an example with no translational component,
  the inversion:

      -x,-y,-z

  Parsing this symbol returns the following matrix/vector pair:

       Matrix    Vector
      -1  0  0     0
       0 -1  0     0
       0  0 -1     0

  Translational components, as required for screw axes and glide planes
  are given as rational numbers, such as in this 2_1 screw axis around z:

      -x,-y,z+1/2

  Which returns the following matrix/vector pair:

       Matrix    Vector
      -1  0  0     0
       0 -1  0     0
       0  0  1    1/2

  From these components, a SymmetryOperation object can be constructed.
  See the documentation for SymmetryOperation and SymmetryOperationFactory.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 30/09/2014

    Copyright © 2014 PSI-MSS

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class MANTID_GEOMETRY_DLL SymmetryOperationSymbolParser {
public:
  ~SymmetryOperationSymbolParser() {}

  static std::pair<Kernel::IntMatrix, V3R>
  parseIdentifier(const std::string &identifier);
  static std::string
  getNormalizedIdentifier(const std::pair<Kernel::IntMatrix, V3R> &data);
  static std::string getNormalizedIdentifier(const Kernel::IntMatrix &matrix,
                                             const V3R &vector);

protected:
  SymmetryOperationSymbolParser();

  static std::pair<Kernel::IntMatrix, V3R>
  parseComponents(const std::vector<std::string> &components);
  static std::string
  getCleanComponentString(const std::string &componentString);
  static std::pair<std::vector<int>, RationalNumber>
  parseComponent(const std::string &component);

  static void processMatrixRowToken(const std::string &matrixToken,
                                    std::vector<int> &matrixRow);
  static void addToVector(std::vector<int> &vector,
                          const std::vector<int> &add);
  static std::vector<int> getVectorForSymbol(const char symbol,
                                             const char sign = '+');
  static int getFactorForSign(const char sign);

  static void
  processVectorComponentToken(const std::string &rationalNumberToken,
                              RationalNumber &vectorComponent);

  static bool isValidMatrixRow(const std::vector<int> &matrixRow);

  static bool regexMembersInitialized();
  static void initializeRegexMembers();
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONSYMBOLPARSER_H_ */
