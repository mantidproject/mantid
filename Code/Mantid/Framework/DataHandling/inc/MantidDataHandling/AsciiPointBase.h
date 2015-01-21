#ifndef MANTID_DATAHANDLING_ASCIIPOINTBASE_H_
#define MANTID_DATAHANDLING_ASCIIPOINTBASE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <fstream>

namespace Mantid {
namespace DataHandling {
/**
Abstract base class for some ascii format save algorithms that print point data
and dq/q.
AsciiPointBase is a framework for some algorithms. It overrides exec and init
and provides full
implementation for any subclasses and as such any subclasses should only provide
implementations
for the additional abstract and virtual methods provided by this class.

Copyright &copy; 2007-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport AsciiPointBase : public API::Algorithm {
public:
  /// Default constructor
  AsciiPointBase() : m_qres(0), m_xlength(0), m_ws() {}
  /// Destructor
  ~AsciiPointBase() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const = 0;
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const = 0;
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Text"; }

private:
  /// Return the file extension this algorthm should output.
  virtual std::string ext() = 0;
  /// return if the line should start with a separator
  virtual bool leadingSep() { return true; }
  /// Add extra properties
  virtual void extraProps() = 0;
  /// write any extra information required
  virtual void extraHeaders(std::ofstream &file) = 0;

  /// Overwrites Algorithm method.
  void init();
  /// Overwrites Algorithm method
  void exec();
  /// returns true if the value is NaN
  bool checkIfNan(const double &value) const;
  /// returns true if the value if + or - infinity
  bool checkIfInfinite(const double &value) const;
  /// print the appropriate value to file
  void outputval(double val, std::ofstream &file, bool leadingSep = true);
  /// write the top of the file
  virtual std::vector<double> header(std::ofstream &file);

protected:
  /// Return the separator character
  virtual char sep() { return '\t'; }
  /// write the main content of the data
  virtual void data(std::ofstream &file, const std::vector<double> &XData,
                    bool exportDeltaQ = true);
  double m_qres;
  size_t m_xlength;

  API::MatrixWorkspace_const_sptr m_ws;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SAVEANSTO_H_  */
