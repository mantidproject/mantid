#ifndef MANTID_ALGORITHMS_GETALLEI_H_
#define MANTID_ALGORITHMS_GETALLEI_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace Algorithms {

/** 
    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport AlignDetectors : public API::Algorithm {
public:
  AlignDetectors();
  virtual ~AlignDetectors();

  /// Algorithms name for identification. @see Algorithm::name
  virtual const std::string name() const;
  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  virtual const std::string summary() const;

  /// Algorithm's version for identification. @see Algorithm::version
  virtual int version() const ;
  /// Algorithm's category for identification. @see Algorithm::category
  virtual const std::string category() const;
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  virtual std::map<std::string, std::string> validateInputs();

private:
  // Implement abstract Algorithm methods
  void init();
  void exec();

};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_GETALLEI_H_ */
