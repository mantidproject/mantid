#ifndef PROCESS_H_
#define PROCESS_H_
/*
Copyright &copy; 2008-18 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include <cstdint>
#include <vector>

/*
 * A minimal wrapper around Python's psutil package to gather information
 * about processes
 */

namespace Process {

bool isAnotherInstanceRunning();

std::vector<int64_t> otherInstancePIDs();
}
#endif // PROCESS_H_
