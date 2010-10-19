#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MDDataObjects/point3D.h"
#include <cmath>
#include <vector>
namespace Mantid
{
	namespace MDAlgorithms
	{
		
		PlaneImplicitFunction::PlaneImplicitFunction(std::vector<double> normal, std::vector<double> origin)
		{
			this->normalX = normal.at(0);
			this->normalY = normal.at(1);
			this->normalZ = normal.at(2);
			this->originX = origin.at(0);
			this->originY = origin.at(1);
			this->originZ = origin.at(2);
		}
		

		bool PlaneImplicitFunction::Evaluate(MDDataObjects::point3D const * const pPoint) const
		{
			std::vector<double> num; 
			DotProduct<double>(pPoint->GetX() - originX, pPoint->GetY() - originY, pPoint->GetZ() - originZ, normalX, normalY, normalZ, num);
			//return num.at(0) + num.at(1) + num.at(2) / Absolute(normalX, normalY, normalZ) <= 0; //Calculates distance, but magnituted of normal not important in this algorithm
			return num.at(0) + num.at(1) + num.at(2)  <= 0;
		}

		double PlaneImplicitFunction::GetOriginX() const
		{
			return this->originX;
		}

		double PlaneImplicitFunction::GetOriginY() const
		{
			return this->originY;
		}

		double PlaneImplicitFunction::GetOriginZ() const
		{
			return this->originZ;
		}

		double PlaneImplicitFunction::GetNormalX() const
		{
			return this->normalX;
		}

		double PlaneImplicitFunction::GetNormalY() const
		{
			return this->normalY;
		}

		double PlaneImplicitFunction::GetNormalZ() const
		{
			return this->normalZ;
		}

		PlaneImplicitFunction::~PlaneImplicitFunction()
		{
		}
	}


}