#ifndef MANTID_ALGORITHMS_COPYDETECTORMAPPING_H_
#define MANTID_ALGORITHMS_COPYDETECTORMAPPING_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/**
    Algorithm to copy spectra-detector mapping from one matrix workspace
    to another.

    @author Dan Nixon

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
class DLLExport CopyDetectorMapping : public API::Algorithm {
public:
  /// (Empty) Constructor
  CopyDetectorMapping() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~CopyDetectorMapping() {}
  /// Algorithm's name
  virtual const std::string name() const { return "CopyDetectorMapping"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "DataHandling";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_COPYDETECTORMAPPING_H_*/
