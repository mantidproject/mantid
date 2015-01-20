/*


namespace Crystal * PeakHKLOffsets.h
*
*  Created on: May 13, 2013
*      Author: ruth
*/
/**
Shows integer offsets  for each peak of their h,k and l values, along with max
offset and run number and detector number

@author Ruth Mikkelson, SNS, ORNL
@date 05/13/2013

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
#ifndef SHOWPEAKHKLOFFSETS_H_
#define SHOWPEAKHKLOFFSETS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {
class DLLExport ShowPeakHKLOffsets : public API::Algorithm {
public:
  ShowPeakHKLOffsets();
  virtual ~ShowPeakHKLOffsets();

  virtual const std::string name() const { return "ShowPeakHKLOffsets"; };

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return " Histograms, scatter plots, etc. of this data could be useful to "
           "detect calibration problems";
  }

  virtual int version() const { return 1; };

  const std::string category() const { return "Crystal"; };

private:
  void init();

  void exec();
};
}
}

#endif /* ShowPeakHKLOffsets_H_ */
