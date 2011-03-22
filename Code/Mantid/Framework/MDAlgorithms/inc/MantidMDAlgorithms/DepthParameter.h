#ifndef DEPTH_PARAMETER_H_
#define DEPTH_PARAMETER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/ImplicitFunctionParameter.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        /**

        DepthParameter. Wraps a vector expressing origin location.

        @author Owen Arnold, Tessella plc
        @date 09/12/2010

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

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
        */

        
        class DLLExport DepthParameter :public Mantid::API::ImplicitFunctionParameter
        {
        private:

           double m_depth;

        public:

            DepthParameter(double depth);
            
            DepthParameter();
            
            DepthParameter(const DepthParameter & other);
            
            DepthParameter& operator=(const DepthParameter& other);

            bool operator==(const DepthParameter &other) const;

            bool operator!=(const DepthParameter &other) const;

            bool isValid() const;

            std::string getName() const;

            DepthParameter* clone() const;

            double getValue() const;

            std::string toXMLString() const;

            ~DepthParameter();

            static std::string parameterName()
            {
                return "DepthParameter";
            }
        };
    }
}

#endif