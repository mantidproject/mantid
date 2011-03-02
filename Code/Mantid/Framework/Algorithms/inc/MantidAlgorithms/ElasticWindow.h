#ifndef MANTID_ALGORITHMS_ELASTIC_WINDOW_H_
#define MANTID_ALGORITHMS_ELASTIC_WINDOW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** @author Michael Whitty, STFC ISIS
    @date 25/10/2010

    This algorithm uses the Integration, ConvertSpectrumAxis and Transpose algorithms
    to provide an integrated value over q and q^2..

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ElasticWindow : public API::Algorithm
{
public:
  /// (Empty) Constructor
  ElasticWindow() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~ElasticWindow() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ElasticWindow"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_ELASTIC_WINDOW_H_*/
