#ifndef ORIGINPARAMETER_H_
#define ORIGINPARAMETER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "MantidAPI/ImplicitFunctionParameter.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        /**

        OriginParameter. Wraps a vector expressing origin location.

        @author Owen Arnold, Tessella plc
        @date 01/10/2010

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

        class DLLExport OriginParameter :public Mantid::API::ImplicitFunctionParameter
        {
        private:

           std::vector<double> m_origin;

        protected:

            OriginParameter* cloneImp() const;

        public:

            explicit OriginParameter(double o1, double o2, double o3);
            
            explicit OriginParameter();
            
            explicit OriginParameter(OriginParameter & other);
            
            explicit OriginParameter(OriginParameter const * const other);
            
            OriginParameter& operator=(const OriginParameter& other);

            bool operator==(const OriginParameter &other) const;

            bool operator!=(const OriginParameter &other) const;

            bool isValid() const;

            std::string getName() const;

            void asVector(std::vector<double>& origin) const;

            std::auto_ptr<OriginParameter> clone() const;

            double getX() const;

            double getY() const;

            double getZ() const;

            std::string toXMLString() const;

            ~OriginParameter();

            static std::string parameterName()
            {
                return "OriginParameter";
            }
        };
    }
}

#endif
