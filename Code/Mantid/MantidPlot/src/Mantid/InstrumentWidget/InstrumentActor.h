#ifndef INSTRUMENTACTOR_H_
#define INSTRUMENTACTOR_H_
#include "CompAssemblyActor.h"
/**
  \class  InstrumentActor
  \brief  InstrumentActor class is wrapper actor for the instrument.
  \author Srikanth Nagella
  \date   March 2009
  \version 1.0

   This class has the implementation for rendering Instrument. it provides the interface for picked ObjComponent and other
   operation for selective rendering of the instrument

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class InstrumentActor : public CompAssemblyActor
{
public:
	InstrumentActor(boost::shared_ptr<Mantid::Geometry::IInstrument> ins, bool withDisplayList); ///< Constructor
	~InstrumentActor();								   ///< Destructor
	virtual std::string type()const {return "InstrumentActor";} ///< Type of the GL object
	void getDetectorIDList(std::vector<int>&);
	void setDetectorColors(std::vector<boost::shared_ptr<GLColor> >& list);
	void refresh();
	int  getDetectorIDFromColor(int rgb);
	void setObjectResolutionToLow();
	void setObjectResolutionToHigh();
};

#endif /*GLTRIANGLE_H_*/

