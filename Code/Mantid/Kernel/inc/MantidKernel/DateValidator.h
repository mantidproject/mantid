#ifndef MANTID_KERNEL_DATEVALIDATOR_H_
#define MANTID_KERNEL_DATEVALIDATOR_H_

#include "IValidator.h"
#include <time.h>

namespace Mantid
{
namespace Kernel
{
/** DateValidator is a validator that validates date, format of valid date is  "DD/MM/YYYY"
    At present, this validator is only available for properties of type std::string
    This class has written for validating  start and end dates of  ICat interface.

    @author Sofia Antony, STFC Rutherford Appleton Laboratory
    @date 03/09/2010
 
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport DateValidator : public IValidator<std::string>
{
public:
  DateValidator();
  virtual ~DateValidator();
  IValidator<std::string> * clone();

private:
  struct tm getTimevalue(const std::string& sDate, std::string & error) const;
  std::string checkValidity(const std::string& value) const;
};

}
}

#endif
