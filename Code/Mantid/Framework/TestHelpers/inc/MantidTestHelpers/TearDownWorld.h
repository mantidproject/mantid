#ifndef TEARDOWNWORLD_H_
#define TEARDOWNWORLD_H_
/**
    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
#include <cxxtest/GlobalFixture.h>

// This file defines a set of CxxTest::GlobalFixture classes that 
// are used to control various aspects of the global test setUp and tearDown
// process

/**
 * Defines a CxxTest::GlobalFixture that clears the AlgorithmManager
 * when its tearDownWorld() method is called.
 */
class ClearAlgorithmManager : public CxxTest::GlobalFixture
{
  bool tearDownWorld();
};

//-----------------------------------------------------------------------------

/**
 * Defines a CxxTest::GlobalFixture that clears the AnalysisDataService
 * when its tearDownWorld() method is called.
 */
class ClearADS : public CxxTest::GlobalFixture
{
  bool tearDownWorld();
};

//-----------------------------------------------------------------------------

/**
 * Defines a CxxTest::GlobalFixture that clears the PropertyManagerDataService
 * when its tearDownWorld() method is called.
 */
class ClearPropertyManagerDataService : public CxxTest::GlobalFixture
{
  bool tearDownWorld();
};


#endif // TEARDOWNWORLD_H_
