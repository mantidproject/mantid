#ifndef HeightParameter_H_
#define HeightParameter_H_

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

        HeightParameter. Wraps a vector expressing origin location.

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

        
        class DLLExport HeightParameter :public Mantid::API::ImplicitFunctionParameter
        {
        private:

           double m_height;

        public:

            HeightParameter(double width);
            
            HeightParameter();
            
            HeightParameter(const HeightParameter & other);
            
            HeightParameter& operator=(const HeightParameter& other);

            bool operator==(const HeightParameter &other) const;

            bool operator!=(const HeightParameter &other) const;

            bool isValid() const;

            std::string getName() const;

            HeightParameter* clone() const;

            double getValue() const;

            std::string toXMLString() const;

            ~HeightParameter();

            static std::string parameterName()
            {
                return "HeightParameter";
            }
        };
    }
}

#endif