#ifndef MANTID_ALGORITHMS_CALCULATERESOLUTION_H_
#define MANTID_ALGORITHMS_CALCULATERESOLUTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include <boost/optional.hpp>

namespace Mantid
{
  namespace Algorithms
  {

    /** CalculateResolution

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    class DLLExport CalculateResolution  : public API::DataProcessorAlgorithm
    {
    public:
      CalculateResolution();
      virtual ~CalculateResolution();

      virtual const std::string name() const;
      virtual int version() const;
      virtual const std::string category() const;
      virtual const std::string summary() const;

    private:
      void init();
      void exec();
    };

  } // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_CALCULATERESOLUTION_H_ */
