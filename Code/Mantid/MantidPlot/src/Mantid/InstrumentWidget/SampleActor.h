#ifndef SMAPLE_ACTOR_H_
#define SMAPLE_ACTOR_H_
#include "GLActor.h"
#include "GLColor.h"
#include "ObjComponentActor.h"

#include <boost/shared_ptr.hpp>

/**
  \class  SampleActor
  \brief  SampleActor is an actor class for rendering Samples.
  \author Roman Tolchenov
  \date   04/07/2011
  \version 1.0

   This class has the implementation for rendering SampleActor in OpenGL and it inherits from the GLActor

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
*/
namespace Mantid
{
  namespace API
  {
    class Sample;
  }
  namespace Geometry
  {
    class IObjComponent;
  }
}

class InstrumentActor;

class SampleActor : public GLActor
{
public:
  SampleActor(const InstrumentActor& instrActor,const Mantid::API::Sample& sample,const ObjComponentActor* samplePosActor); ///< Constructor
  virtual std::string type()const {return "SampleActor";} ///< Type of the GL object
  void draw(bool picking)const;
  void getBoundingBox(Mantid::Kernel::V3D& minBound,Mantid::Kernel::V3D& maxBound)const;
  void setColor(const GLColor& c){m_color = c;}
  const ObjComponentActor* getSamplePosActor()const{return m_samplePosActor;}
protected:
  const InstrumentActor& m_instrActor;
  const Mantid::API::Sample& m_sample;
  const ObjComponentActor* m_samplePosActor;
  boost::shared_ptr<const Mantid::Geometry::IObjComponent> m_samplePos;
  GLColor m_color;
};

#endif /*SMAPLE_ACTOR_H_*/

