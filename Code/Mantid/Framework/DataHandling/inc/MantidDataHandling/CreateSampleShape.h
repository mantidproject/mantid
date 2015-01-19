#ifndef MANTID_DATAHANDLING_CREATESAMPLESHAPE_H_
#define MANTID_DATAHANDLING_CREATESAMPLESHAPE_H_

//--------------------------------
// Includes
//--------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

/**
    This class allows the shape of the sample to be defined by using the allowed
   XML
    expressions

    @author Martyn Gigg, Tessella Support Services plc
    @date 13/03/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport CreateSampleShape : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  CreateSampleShape() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~CreateSampleShape() {}
  /// Algorithm's name
  virtual const std::string name() const { return "CreateSampleShape"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Create a shape object to model the sample.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Sample;DataHandling"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};
}
}

#endif /* MANTID_DATAHANDLING_CREATESAMPLESHAPE_H_*/
