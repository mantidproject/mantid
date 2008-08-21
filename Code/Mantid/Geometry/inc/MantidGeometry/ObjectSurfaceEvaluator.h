#ifndef OBJECT_SURFACEEVALUATOR_H
#define OBJECT_SURFACEEVALUATOR_H

#include <iostream>
#include <vector>
#include "MantidGeometry/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quadratic.h"
#include "MantidGeometry/Object.h"

namespace Mantid
{

	namespace Geometry
	{
		/*!
		\class ObjectSurfaceEvaluator
		\brief Evaluate Object surface at a given point
		\author Srikanth Nagella
		\date July 2008
		\version 1.0

		This is an concrete class for evaluating a value of surface at a given point.

		Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

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
		*/
		class Rule;
		class Intersection;
		class Union;
		class SurfPoint;
		class CompObj;
		class CompGrp;
		class BoolValue;
		class ObjectSurfaceEvaluator:public SurfaceEvaluator
		{
		private:
			const Object *surf;  ///< Input Object for which R-Function value needs evaluating
			double evaluate(Rule* rule,V3D point);
			double evaluate(Intersection* rule,V3D point);
			double evaluate(Union* rule,V3D point);
			double evaluate(CompObj* rule,V3D point);
			double evaluate(CompGrp* rule,V3D point);
			double evaluate(BoolValue* rule,V3D point);			
			double evaluate(SurfPoint* rule,V3D point);
		public:
			ObjectSurfaceEvaluator(const Object* quad){surf=quad;} ///< Constructor
			double evaluate(V3D point);
		};
	}
}
#endif