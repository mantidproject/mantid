#include "MantidGeometry/MDGeometry/MDDimensionRes.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h> 
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <sstream>
#include <float.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

namespace Mantid{
    namespace Geometry{






MDDimensionRes::MDDimensionRes(const std::string &ID,const rec_dim nRecDim0):
MDDimension(ID),
nRecDim(nRecDim0)
{
    this->direction[nRecDim] = 1;
}
void 
MDDimensionRes::setDirection(const V3D &theDirection)
{
	
    if(theDirection.norm2()<FLT_EPSILON){
        g_log.error()<<"MDDimensionRes::setDirection: Attempt to set reciprocal dimension in 0 direction";
        throw(std::invalid_argument("MDDimensionRes::setDirection: Attempt to set reciprocal dimension in 0 direction"));
    }
    direction=theDirection;
	direction.normalize();

}
//
V3D 
MDDimensionRes::getDirectionCryst(void)const
{
	unsigned int i;
	V3D dirCryst(this->direction);
	double minDir(FLT_MAX);
	double val;
	for(i=0;i<3;i++){
		val =std::abs(dirCryst[i]); 
		if(val>FLT_EPSILON){
			if(val<minDir)minDir=val;
		}
	}
	return dirCryst/minDir;
}
//
std::string MDDimensionRes::getQTypeAsString() const
{
  std::string qType;
  if(this->nRecDim == q1)
  {
    qType = "q1";
  }
  else if(this->nRecDim == q2)
  {
    qType = "q2";
  }
  else
  {
    qType = "q3";
  }
  return qType;
}


std::string MDDimensionRes::toXMLString() const
{
   using namespace Poco::XML;

  //Create the root element for this fragment.
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> pDimensionElement= pDoc->createElement("Dimension");
  //Create the body
  pDoc->appendChild(pDimensionElement);
  
  //Apply reciprocal dimension xml.
  ApplySerialization(pDoc.get(), pDimensionElement.get());

  //This is a reciprocal dimension
  AutoPtr<Element> reciprocalDimensionMappingElement = pDoc->createElement("ReciprocalDimensionMapping");
  //The type of reciprocal dimension
  std::string qTypeString = getQTypeAsString();
  AutoPtr<Text> reciprocalDimensionMappingText = pDoc->createTextNode(qTypeString);
  reciprocalDimensionMappingElement->appendChild(reciprocalDimensionMappingText);
  pDimensionElement->appendChild(reciprocalDimensionMappingElement);

  //Convert to string format.
  std::stringstream xmlstream;
  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  return xmlstream.str().c_str();
}


MDDimensionRes::~MDDimensionRes(void)
{
}
//********************************************************************************************************************************************
MDDimDummy::MDDimDummy(unsigned int nRecDim):
MDDimensionRes("DUMMY REC_DIM",(rec_dim)nRecDim)
{ 
  // set 1 bin and the dimension integrated;
  this->setRange(0,1,1);
  this->setName("DUMMY AXIS NAME");
}
  //Get coordinate for index; Throws  if ind is out of range 
double 
MDDimDummy::getX(unsigned int ind)const
{
      switch(ind){
      case(0): return 0;
      case(1): return 1;
      default: throw(std::out_of_range("Dummy dimension index is out of range (0,1) "));
      }
}

}
}
