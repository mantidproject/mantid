#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>
#include <vector>
#include <map>
#include <list>
#include <stack>
#include <string>
#include <algorithm>

#include "MantidKernel/Logger.h"
#include "AuxException.h"

#include "MantidGeometry/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Support.h"
#include "MantidGeometry/BaseVisit.h"
#include "MantidGeometry/Surface.h"
#include "MantidGeometry/Quadratic.h"
#include "MantidGeometry/Plane.h"
#include "MantidGeometry/Cylinder.h"
#include "MantidGeometry/Cone.h"
#include "MantidGeometry/General.h"
#include "MantidGeometry/Sphere.h"
#include "MantidGeometry/Torus.h"
#include "MantidGeometry/SurfaceFactory.h"

namespace Mantid
{

namespace Geometry 
{

Kernel::Logger& SurfaceFactory::PLog(Kernel::Logger::get("SurfaceFactory"));
SurfaceFactory* SurfaceFactory::FOBJ(0);


SurfaceFactory*
SurfaceFactory::Instance() 
  /*!
    Effective new command / this command 
    \returns Single instance of SurfaceFactory
  */
{
  if (!FOBJ)
    {
      FOBJ=new SurfaceFactory();
    }
  return FOBJ;
}

SurfaceFactory::SurfaceFactory() 
  /*!
    Constructor
  */
{
  registerSurface();
}

SurfaceFactory::SurfaceFactory(const SurfaceFactory& A)  :
  ID(A.ID)
  /*! 
    Copy constructor 
    \param A :: Object to copy
  */
{
  MapType::const_iterator vc;
  for(vc=A.SGrid.begin();vc!=A.SGrid.end();vc++)
    SGrid.insert(MapType::value_type
		 (vc->first,vc->second->clone()));

}
  
SurfaceFactory::~SurfaceFactory()
  /*!
    Destructor removes memory for atom/cluster list
  */
{
  MapType::iterator vc;
  for(vc=SGrid.begin();vc!=SGrid.end();vc++)
    delete vc->second;
}

void
SurfaceFactory::registerSurface()
  /*!
    Register tallies to be used
  */
{
  SGrid["Plane"]=new Plane;  
  SGrid["Cylinder"]=new Cylinder;  
  SGrid["Cone"]=new Cone;  
  SGrid["Torus"]=new Torus;  
  SGrid["General"]=new General;  
  SGrid["Sphere"]=new Sphere;  
  
  ID['c']="Cylinder";
  ID['k']="Cone";
  ID['g']="General";
  ID['p']="Plane";
  ID['s']="Sphere";
  ID['t']="torus";
  return;
}
  
Surface*
SurfaceFactory::createSurface(const std::string& Key) const
  /*!
    Creates an instance of tally
    given a valid key. 
    
    \param Key :: Item to get 
    \throw InContainerError for the key if not found
    \return new tally object.
  */    
{
  MapType::const_iterator vc;
  vc=SGrid.find(Key);
  if (vc==SGrid.end())
    {
      throw ColErr::InContainerError<std::string>
	(Key,"SurfaceFactory::createSurface");
    }
  Surface *X= vc->second->clone();
  return X;
}

Surface*
SurfaceFactory::createSurfaceID(const std::string& Key) const
  /*!
    Creates an instance of tally
    given a valid key. 
    
    \param Key :: Form of first ID
    \throw InContainerError for the key if not found
    \return new tally object.
  */    
{
  std::map<char,std::string>::const_iterator mc;
  
  mc=(Key.empty()) ? ID.end() : ID.find(tolower(Key[0]));
  if (mc==ID.end())
    {
      throw ColErr::InContainerError<std::string>
	(Key,"SurfaceFactory::createSurfaceID");
    }
  
  return createSurface(mc->second);
}

}  // NAMESPACE Geometry
 
}  // NAMESPACE Mantid 

