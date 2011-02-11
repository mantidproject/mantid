#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h> 
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

namespace Mantid
{
namespace Geometry
{
// get reference to logger for
Kernel::Logger& MDDimension::g_log=Kernel::Logger::get("MDDimension");

//
std::string MDDimension::getDimensionId() const
{
  return this->dimTag; //TODO, dimension id string member required for this type.
}


bool MDDimension::getIsIntegrated() const
{
  return this->isIntegrated;
}

void MDDimension::getAxisPoints(std::vector<double>  &rez)const
{
  rez.resize(this->nBins);
  for(unsigned int i=0;i<nBins;i++){
    rez[i]=0.5*(this->Axis[i]+this->Axis[i+1]);
  }
}

// default dimension is always integrated (it has one point and limits) 
MDDimension::MDDimension(const std::string &ID):
    dimTag(ID),
    isIntegrated(true),
    nBins(1),
    nStride(0),
    data_shift(0)
{
  // default name coinside with the tag but can be overwritten later
  this->setRange();
  this->setName(dimTag);

}
void 
MDDimension::initialize(const DimensionDescription &descr)
{
    if(descr.Tag!=this->dimTag){
        g_log.error()<<"Attempt to initialize the dimension, ID"<<this->dimTag<<" with wrong dimension description: "<<descr.Tag<<std::endl;
    }
    //TODO:  the dimensin direction to be introduced and set-up here soon;
    this->setRange(descr.cut_min,descr.cut_max,descr.nBins);
    // stride now reset which makes this dimension invalid in group of dimensions
    this->nStride = 0;
	// the meaning of the lattice parameter for dimension is unclear
//    this->latticeParam= descr.data_scale;
    this->data_shift  = descr.data_shift;

	if(!descr.AxisName.empty()){
			this->setName(descr.AxisName);
    }

}
/// this function sets Linear range. 
void  MDDimension::setRange(double rxMin,double rxMax,unsigned int nxBins)
{
  if(rxMin>rxMax)
  {
    g_log.error()<< "Attempting to set minimal integration limit higer then maximal for Dimension tag: "<<this->dimTag<<std::endl;
    g_log.error()<< "setting MinVal: "<<rxMin<<" MaxVal: "<<rxMax<<std::endl;

    throw(std::invalid_argument("setRange: wrong argument"));
  }
  this->rMin = rxMin;
  this->rMax = rxMax;

  this->setExpanded(nxBins);

}
/// set dimension expanded;
void MDDimension::setExpanded(double rxMin, double rxMax,unsigned int nBins)
{
  this->check_ranges(rxMin,rxMax);
  this->rMin=rxMin;
  this->rMax=rxMax;

  this->setExpanded(nBins);
}

//
void MDDimension::check_ranges(double rxMin,double rxMax)
{
  if(rxMin>rxMax)
  {
    g_log.error()<< "Attempting to set minimal integration limit higer then maximal for Dimension tag: "<<this->dimTag<<std::endl;
    g_log.error()<< "setting MinVal: "<<rxMin<<" MaxVal: "<<rxMax<<std::endl;

    throw(std::invalid_argument("checkRanges: rMin>rMax"));
  }
  //if(rxMin>this->rMax||rxMax<this->rMin){
  //    g_log.error()<< "Attempting to set integration limits outside the data range in Dimension ID N: "<<this->dimTag<<std::endl;
  //    g_log.error()<< "existing MinVal: "<<this->rMin<<" MaxVal: "<<this->rMax<<" Setting: minVal: "<<rxMin<<" maxVal: "<<rxMax<<std::endl;
  //    throw(std::invalid_argument("checkRanges: wrong rMin or rMax"));

  //}

}

void MDDimension::setExpanded(unsigned int nxBins)
{
  if(nxBins<1||nxBins>MAX_REASONABLE_BIN_NUMBER)
  {
    g_log.error()<< "Setting number of bins="<<nxBins<<" our of range  for Dimension tag: "<<this->dimTag<<std::endl;
    throw(std::invalid_argument("setExpanded: wrong number of bin"));
  }
  if(nxBins> 1)
  {
    this->isIntegrated=false;
  }
  else
  {
    this->isIntegrated=true;
  }
  this->nBins= nxBins;
  double Delta=this->getRange()/(nBins);

  double r;
  this->Axis.clear();
  this->Axis.reserve(nBins+1);
  for(unsigned int i=0;i<nBins+1;i++){
    r=this->rMin+i*Delta;
    this->Axis.push_back(r);
  }
  // but all this is not enough -- > stride has to be set extrenaly, on the basis of other dimensions which were or were not integrated;
  // stide is undefined here
}


/// clear dimension and sets integrated sign;
void MDDimension::setIntegrated(void)
{
  this->isIntegrated=true;
  this->nBins  =1;
  this->nStride=0;  // the stride of neighboring dimensions has to change accordingly
  this->Axis.clear();
  this->Axis.assign(2,0);
  this->Axis[0] = this->rMin;
  this->Axis[1] = this->rMax;
}

void MDDimension::setIntegrated(double rxMin)
{
  if(rxMin>this->rMax)
  {
    g_log.error()<< "Attempting to set minimal integration limit higer then maximal for Dimension tag: "<<this->dimTag<<std::endl;
    g_log.error()<< "existing MaxVal: "<<this->rMax<<" setting minVal: "<<rxMin<<std::endl;
    throw(std::invalid_argument("setIntegrated: new min integration limit is higer than existing max integration limit"));
  }
  this->rMin=rxMin;
  this->setIntegrated();
}

void MDDimension::setIntegrated(double rxMin, double rxMax)
{
  this->check_ranges(rxMin,rxMax);

  this->rMin=rxMin;
  this->rMax=rxMax;
  this->setIntegrated();
}

bool MDDimension::operator==(const MDDimension& other) const
{
  return this->dimTag == other.dimTag;
}

///* Assigment operator */
//MDDimension & MDDimension::operator=(const MDDimension &rhs)
//{
//  this->Axis = rhs.Axis;
//  AxisName = rhs.AxisName;
//  dimTag = rhs.dimTag;
//  isIntegrated = rhs.isIntegrated;
//  nBins = rhs.nBins;
//  nStride = rhs.nStride;
//  rMin = rhs.rMin;
//  rMax = rhs.rMax;
//  latticeParam = rhs.latticeParam;
//  coord = rhs.coord;
//  return *this;
//}

bool MDDimension::operator!=(const MDDimension& other) const
{
  return this->dimTag != other.dimTag;
}

void MDDimension::ApplySerialization(Poco::XML::Document* pDoc, Poco::XML::Element* pDimensionElement) const
{
  using namespace Poco::XML;

  //Set the id.
  AutoPtr<Attr> idAttribute = pDoc->createAttribute("ID");
  idAttribute->setNodeValue(this->getDimensionId());
  pDimensionElement->setAttributeNode(idAttribute);

  //Set the name.
  AutoPtr<Element> nameElement = pDoc->createElement("Name");
  AutoPtr<Text> nameText = pDoc->createTextNode(this->getName());
  nameElement->appendChild(nameText);
  pDimensionElement->appendChild(nameElement);

  //Set the upper bounds
  AutoPtr<Element> upperBoundsElement = pDoc->createElement("UpperBounds");
  AutoPtr<Text> upperBoundsText = pDoc->createTextNode(boost::str(boost::format("%.4d") % this->getMaximum()));
  upperBoundsElement->appendChild(upperBoundsText);
  pDimensionElement->appendChild(upperBoundsElement);

  //Set the lower bounds
  AutoPtr<Element> lowerBoundsElement = pDoc->createElement("LowerBounds");
  AutoPtr<Text> lowerBoundsText = pDoc->createTextNode(boost::str(boost::format("%.4d") % this->getMinimum()));
  lowerBoundsElement->appendChild(lowerBoundsText);
  pDimensionElement->appendChild(lowerBoundsElement);

  //Set the number of bins
  AutoPtr<Element> numberOfBinsElement = pDoc->createElement("NumberOfBins");
  AutoPtr<Text> numberOfBinsText = pDoc->createTextNode(boost::str(boost::format("%.4d") % this->getNBins()));
  numberOfBinsElement->appendChild(numberOfBinsText);
  pDimensionElement->appendChild(numberOfBinsElement);

  //Provide upper and lower limits for integrated dimensions.
  if(this->getIntegrated())
  {
    AutoPtr<Element> integratedElement = pDoc->createElement("Integrated");
    //Set the upper limit
    AutoPtr<Element> upperLimitElement = pDoc->createElement("UpperLimit");
    AutoPtr<Text> upperLimitText = pDoc->createTextNode(boost::str(boost::format("%.4d") % this->getMaximum())); // Dimension does not yet provide integration ranges.
    upperLimitElement->appendChild(upperLimitText);
    integratedElement->appendChild(upperLimitElement);

    //Set the lower limit
    AutoPtr<Element> lowerLimitElement = pDoc->createElement("LowerLimit");
    AutoPtr<Text> lowerLimitText = pDoc->createTextNode(boost::str(boost::format("%.4d") % this->getMinimum())); // Dimension does not yet provide integration ranges.
    lowerLimitElement->appendChild(lowerLimitText);
    integratedElement->appendChild(lowerLimitElement);

    pDimensionElement->appendChild(integratedElement);
  }
}

std::string MDDimension::toXMLString() const
{
   using namespace Poco::XML;

  //Create the root element for this fragment.
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> pDimensionElement= pDoc->createElement("Dimension");
  pDoc->appendChild(pDimensionElement);

  //Apply the serialization based on the current instance.
  ApplySerialization(pDoc.get(), pDimensionElement.get());

  //Create a string representation of the DOM tree.
  std::stringstream xmlstream;
  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  return xmlstream.str().c_str();
}

MDDimension::~MDDimension()
{
}

} //namespace
}
