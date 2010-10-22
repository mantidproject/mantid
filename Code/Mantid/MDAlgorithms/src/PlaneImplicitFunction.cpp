#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MDDataObjects/point3D.h"
#include <cmath>
#include <vector>

namespace Mantid
{
    namespace MDAlgorithms
    {

        PlaneImplicitFunction::PlaneImplicitFunction(NormalParameter normal, OriginParameter origin): m_normal(normal), m_origin(origin)
        {

        }


        bool PlaneImplicitFunction::evaluate(MDDataObjects::point3D const * const pPoint) const
        {
            std::vector<double> num; 
            dotProduct<double>(pPoint->GetX() - m_origin.getX(), pPoint->GetY() - m_origin.getY(), pPoint->GetZ() - m_origin.getZ(), m_normal.getX(), m_normal.getY(), m_normal.getZ(), num);
            //return num.at(0) + num.at(1) + num.at(2) / absolute(normalX, normalY, normalZ) <= 0; //Calculates distance, but magnituted of normal not important in this algorithm
            return num.at(0) + num.at(1) + num.at(2)  <= 0;
        }

        double PlaneImplicitFunction::getOriginX() const
        {
            return this->m_origin.getX();
        }

        double PlaneImplicitFunction::getOriginY() const
        {
            return this->m_origin.getY();
        }

        double PlaneImplicitFunction::getOriginZ() const
        {
            return this->m_origin.getZ();
        }

        double PlaneImplicitFunction::getNormalX() const
        {
            return this->m_normal.getX();
        }

        double PlaneImplicitFunction::getNormalY() const
        {
            return this->m_normal.getY();
        }

        double PlaneImplicitFunction::getNormalZ() const
        {
            return this->m_normal.getZ();
        }

        PlaneImplicitFunction::~PlaneImplicitFunction()
        {
        }
    }


}