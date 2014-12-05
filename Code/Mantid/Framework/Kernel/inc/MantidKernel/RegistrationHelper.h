#ifndef MANTID_KERNEL_REGISTRATIONHELPER_H_
#define MANTID_KERNEL_REGISTRATIONHELPER_H_
/*
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

#include "MantidKernel/System.h"

namespace Mantid
{
  namespace Kernel
  {

    /**
     * This class is simply used in the subscription of classes into the various
     * factories in Mantid. The fact that the constructor takes an int means that
     * the comma operator can be used to make a call to the factories' subscribe
     * method in the first part.
     */
    class DLLExport RegistrationHelper
    {
    public:
      /** Constructor. Does nothing.
       * @param i :: Takes an int and does nothing with it
       */
      inline RegistrationHelper(int i) { UNUSED_ARG(i); }
    };

  }
}

#endif /* MANTID_KERNEL_REGISTRATIONHELPER_H_ */
