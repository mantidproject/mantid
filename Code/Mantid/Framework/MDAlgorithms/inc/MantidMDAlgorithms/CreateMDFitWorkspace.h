#ifndef CREATEMDFITWORKSPACE_H_
#define CREATEMDFITWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include "MantidAPI/Algorithm.h"

namespace Mantid
{

    namespace MDAlgorithms
    {
        /** NOTE: CURRENTLY DISABLED in CMakeLists.txt
         * TODO: Refactor to use MDHistoWorkspace
         *
         * Algorithm to generate fake MDWorkspace from a function.

        @author Roman Tolchenov
        @date 01/10/2010

        Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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


        class DLLExport CreateMDFitWorkspace : public Mantid::API::Algorithm
        {
        public:

          /// Default constructor
          CreateMDFitWorkspace() : API::Algorithm() {};
          /// Destructor
          virtual ~CreateMDFitWorkspace(){}
          /// Algorithm's name for identification overriding a virtual method
          virtual const std::string name() const { return "CreateMDFitWorkspace";}
          /// Algorithm's version for identification overriding a virtual method
          virtual int version() const { return (1);}
          /// Algorithm's category for identification overriding a virtual method
          virtual const std::string category() const { return "CurveFitting";}

        protected:

          // Overridden Algorithm methods
          void init();
          void exec();
        };

    }
}

#endif // CREATEMDFITWORKSPACE_H_
