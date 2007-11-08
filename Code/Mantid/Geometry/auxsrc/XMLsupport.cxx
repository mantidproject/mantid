#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <vector>
#include <stack>
#include <map>
#include <iterator>
#include <functional>
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>

#include "Exception.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "GTKreport.h"
#include "FileReport.h"
#include "OutputLog.h"
#include "RefControl.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "EventComp.h"
#include "CommandObj.h"
#include "XMLsupport.h"

namespace XML
{
  
mapXML::mapXML(XMLcollect& A) :
  AX(A)
  /*!
    Constructor takes XMLcollect to append to
   */
{}

void 
mapXML::operator()(const std::pair<std::string,Command::CommandObj*>& A)
  /*!
    Update an XML object
  */
{
  AX.addGrp(A.second->getName());
  for(int i=0;i<A.second->getArgs();i++)
    {
      Command::EventComp* EV=A.second->getEvent(i);
      if (EV)
	AX.addNumComp("Event",EV->getName());
      else
	AX.addNumComp("Event",XML::nullObj());
    }
  AX.closeGrp();
  return;
}

}
