#ifndef MANTID_CURVEFITTING_FAKEMINIMIZER_H_
#define MANTID_CURVEFITTING_FAKEMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"

namespace Mantid
{
namespace CurveFitting
{
/** Fake minimizer for testing output workspace properties.
    Must be deleted before closing ticket #10008.

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport FakeMinimizer : public API::IFuncMinimizer
{
public:
  /// Constructor setting a value for the relative error acceptance (default=0.01)
  FakeMinimizer();
  /// Destructor
  ~FakeMinimizer();

  /// Overloading base class methods
  std::string name()const{return "Fake";}
  /// Do one iteration
  bool iterate(size_t);
  /// Return current value of the cost function
  double costFunctionVal();
  /// Initialize minimizer, i.e. pass a function to minimize.
  virtual void initialize(API::ICostFunction_sptr function, size_t maxIterations = 0);

private:
  size_t m_maxIters;
  std::vector<double> m_data;
  int m_someInt;
  double m_someDouble;
  std::string m_someString;
};


} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FAKEMINIMIZER_H_*/
