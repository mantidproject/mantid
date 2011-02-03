#ifndef MANTID_MDALGORITHMS_PERPENDICULARPARAMETER
#define MANTID_MDALGORITHMS_PERPENDICULARPARAMETER

#include "MantidAPI/ImplicitFunctionParameter.h"

namespace Mantid
{
namespace MDAlgorithms
{

/**
 PerpendicularParameter defines a spatial vector as the cross product of a normal vector and the up vector.

 @author Owen Arnold, Tessella plc
 @date 01/02/2011

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

class DLLExport PerpendicularParameter :public Mantid::API::ImplicitFunctionParameter
{
private:

   std::vector<double> m_perpendicular;

public:

    PerpendicularParameter(double o1, double o2, double o3);

    PerpendicularParameter();

    PerpendicularParameter(const PerpendicularParameter & other);

    PerpendicularParameter& operator=(const PerpendicularParameter& other);

    bool operator==(const PerpendicularParameter &other) const;

    bool operator!=(const PerpendicularParameter &other) const;

    bool isValid() const;

    std::string getName() const;

    PerpendicularParameter* clone() const;

    double getX() const;

    double getY() const;

    double getZ() const;

    std::string toXMLString() const;

    ~PerpendicularParameter();

    static std::string parameterName()
    {
        return "PerpendicularParameter";
    }
};

}
}
#endif
