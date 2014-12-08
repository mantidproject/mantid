#ifndef MANTID_KERNEL_CLASSMACROS_H_
#define MANTID_KERNEL_CLASSMACROS_H_
/**
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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/**
 * This file defines macros for altering class behaviour, i.e. disabling copying etc
 */

/// Disable default construction
#define DISABLE_DEFAULT_CONSTRUCT(ClassType) \
  ClassType();

/// Disable copy & assign
#define DISABLE_COPY_AND_ASSIGN(ClassType) \
  ClassType(const ClassType&);\
  ClassType& operator=(const ClassType&);


#endif /* MANTID_KERNEL_CLASSMACROS_H_ */
