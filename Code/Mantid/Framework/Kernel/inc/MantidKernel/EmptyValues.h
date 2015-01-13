#ifndef MANTID_KERNEL_EMPTYVALUES_H_
#define MANTID_KERNEL_EMPTYVALUES_H_

/**
    This file contains functions to define empty values, i.e EMPTY_INT();

    @author Martyn Gigg, Tessella plc

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidKernel/DllConfig.h"

namespace Mantid {

/// Returns what we consider an "empty" integer
DLLExport int EMPTY_INT();

/// Returns what we consider an "empty" long
DLLExport long EMPTY_LONG();

/// Return what we consider to be an empty double
DLLExport double EMPTY_DBL();
}

#endif // MANTID_KERNEL_EMPTYVALUES_H_
