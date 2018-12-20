#ifndef MANTID_ALGORITHMS_ELASTIC_WINDOW_H_
#define MANTID_ALGORITHMS_ELASTIC_WINDOW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** @author Michael Whitty, STFC ISIS
    @date 25/10/2010

    This algorithm uses the Integration, ConvertSpectrumAxis and Transpose
   algorithms
    to provide an integrated value over q and q^2..

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ElasticWindow : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ElasticWindow"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm performs an integration over an energy range, with "
           "the option to subtract a background over a second range, then "
           "transposes the result into a single-spectrum workspace with units "
           "in Q and Q^2.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"Integration"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Inelastic\\Indirect"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_ELASTIC_WINDOW_H_*/
