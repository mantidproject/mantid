/*WIKI*

This algorithm loads a Sassena output file into a group workspace.
It will create a workspace for each scattering intensity and one workspace for the Q-values

*WIKI*/

#include<fstream>
#include <utility>
#include <bits/stl_pair.h>

#include "MantidDataHandling/LoadSassenaParams.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/Exception.h"

// special library headers
#include <boost/filesystem.hpp>
#include <boost/math/special_functions/factorials.hpp>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_on_sphere.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/thread.hpp>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadSassenaParams)
//register the algorithm into LoadAlgorithmFactory
DECLARE_LOADALGORITHM(LoadSassenaParams)

// Initialize the loggers
Kernel::Logger& XMLInterface::g_log = Kernel::Logger::get("XMLInterface");
Kernel::Logger& ScatteringAverageOrientationVectorsParameters::g_log = Kernel::Logger::get("ScatteringAverageOrientationVectorsParameters");
Kernel::Logger& ScatteringAverageOrientationMultipoleMomentsParameters::g_log = Kernel::Logger::get("ScatteringAverageOrientationMultipoleMomentsParameters");
Kernel::Logger& ScatteringVectorsParameters::g_log = Kernel::Logger::get("ScatteringVectorsParameters");
Kernel::Logger& Params::g_log = Kernel::Logger::get("Params");

/**Constructor
 * @param ptr xml node, possibly containing children
 */
XMLElement::XMLElement(xmlNodePtr ptr) {
  node_ptr = ptr;
  if (node_ptr->children!=NULL) {
    xmlNodePtr cur = node_ptr->children;
    while(cur!=NULL) {
      children.push_back(cur);
      cur=cur->next;
    }
  }
}

void XMLElement::print(const std::string prepend, bool showchildren)
{
  std::cout << prepend << "type   = " << this->type() << std::endl;
  std::cout << prepend << "name   = " << this->name() << std::endl;
  std::cout << prepend << "content= " << this->content() << std::endl;
  if (!showchildren) return;
  if (this->children.size()==0) return;
  std::cout << prepend << "children: [" << this->children.size() << "]" << std::endl;
  for(size_t i = 0; i < this->children.size(); ++i){
    std::cout << prepend << "-----" << std::endl;
    this->children[i].print(std::string(prepend)+"  ", showchildren);
    std::cout << prepend << "-----" << std::endl;
  }
}

XMLInterface::XMLInterface(const std::string filename)
{
  xmlInitParser();
  try
  {
    p_doc = xmlParseFile(filename.c_str());
  }
  catch(xmlError const& p_excp)
  {
    throw Kernel::Exception::FileError(std::string(p_excp.message)+". Unable to parse File:", filename);
  }

  /* Create xpath evaluation context */
  p_xpathCtx = xmlXPathNewContext(p_doc);
  if(p_xpathCtx == NULL)
  {
    xmlFreeDoc(p_doc);
    throw Kernel::Exception::NullPointerException("p_xpathCtx","XMLInterface::XMLInterface");
  }

  maxrecursion = 20;
  size_t recursion = 1;
  while( xmlXIncludeProcess(p_doc)!=0 )
  {
    if (recursion >maxrecursion)
      throw Kernel::Exception::IndexError(recursion,maxrecursion,"XMLInterface::XMLInterface");
    recursion++;
  }
} // end of XMLInterface::XMLInterface

void XMLInterface::dump(std::vector<char>& c)
{
  int chars;
  xmlChar * data;
  xmlDocDumpMemory(p_doc,&data,&chars);
  std::stringstream ss;
  for(int i = 0; i < chars; ++i) c.push_back(data[i]);
  xmlFree(data);
}

XMLInterface::~XMLInterface()
{
  xmlXPathFreeContext(p_xpathCtx);
  xmlFreeDoc(p_doc);
  xmlCleanupParser();
}

std::vector<XMLElement> XMLInterface::get(const char* xpathexp)
{
  return get(std::string(xpathexp));
}

std::vector<XMLElement> XMLInterface::get(std::string xpathexp)
{
  // PREPARE XPATH expressions first...
  const xmlChar* p_xpath_exp = xmlCharStrdup(xpathexp.c_str());
  xmlXPathObjectPtr p_xpathObj = xmlXPathEvalExpression(p_xpath_exp, p_xpathCtx);
  if (p_xpathObj==NULL)
  {
    g_log.error("XPath expression not found: "+xpathexp);
    throw;
  }
  xmlNodeSetPtr nodes = p_xpathObj->nodesetval; // nodes
  size_t size = (nodes) ? nodes->nodeNr : 0;
  std::vector<XMLElement> result;
  for(size_t i = 0; i < size; ++i)
  {
    result.push_back(nodes->nodeTab[i]);
  }
  xmlXPathFreeObject(p_xpathObj);
  return result;
}

bool XMLInterface::exists(const char* xpathexp)
{
  const xmlChar* p_xpath_exp = xmlCharStrdup(xpathexp);
  xmlXPathObjectPtr p_xpathObj = xmlXPathEvalExpression(p_xpath_exp, p_xpathCtx);
  if (p_xpathObj->nodesetval->nodeNr==0)
  {
  return false;
  }
  else
  {
    return true;
  }
}

template<> bool XMLInterface::get_value<bool>(const char* xpathexp)
{
  std::vector<XMLElement> elements = get(xpathexp);
  // if elements has more than one entry, then xpathexp is ambigious
  if (elements.size()==0)
  {
    return boost::lexical_cast<bool>("");
  }
  else if (elements.size()>1)
  {
    g_log.error("Xpathexp is ambigious, multiple fields matched: "+std::string(xpathexp));
    throw;
  }
  XMLElement& thisel = elements[0];
  if (thisel.type()!=XML_ELEMENT_NODE)
  {
    g_log.error("Xpathexp doesn't resolve to XML_ELEMENT_NODE: "+std::string(xpathexp));
  throw;
  }
  size_t textelements = 0;
  std::string result;
  for(size_t i = 0; i < thisel.children.size(); ++i)
  {
    if (thisel.children[0].type()==XML_TEXT_NODE)
    {
      textelements++;
      result = thisel.children[0].content();
    }
  }
  // textelements == 0 corresponds to an empty text field. That is legal!
  if (textelements>1)
  {
    g_log.error("Xpathexp resolves to more than one text field: "+std::string(xpathexp));
    throw;
  }
  // trim whitespaces!
  boost::trim(result);
  std::string result_upper = result;
  boost::to_upper(result_upper);
  // circumvent differences b/w lexical_cast and xml specification
  if (result_upper=="FALSE") result = "0";
  if (result_upper=="TRUE") result = "1";
  return boost::lexical_cast<bool>(result);
}

/// Provides type safe access to content element in XML files through XPATH.
template<class convT> convT XMLInterface::get_value(const char* xpathexp)
{
  std::vector<XMLElement> elements = get(xpathexp);
  // if elements has more than one entry, then xpathexp is ambigious
  if (elements.size()==0)
  {
    return boost::lexical_cast<convT>("");
  }
  else if (elements.size()>1)
  {
    g_log.error("Xpathexp is ambigious, multiple fields matched: "+std::string(xpathexp));
    throw;
  }
  XMLElement& thisel = elements[0];
  if (thisel.type()!=XML_ELEMENT_NODE)
  {
    g_log.error("Xpathexp doesn't resolve to XML_ELEMENT_NODE: "+std::string(xpathexp));
    throw;
  }
  size_t textelements = 0;
  std::string result;
  for(size_t i = 0; i < thisel.children.size(); ++i)
  {
    if (thisel.children[0].type()==XML_TEXT_NODE)
    {
      textelements++;
      result = thisel.children[0].content();
    }
  }
  // textelements == 0 corresponds to an empty text field. That is legal!
  if (textelements>1)
  {
    g_log.error("Xpathexp resolves to more than one text field: "+std::string(xpathexp));
    throw;
   }
  // trim whitespaces!
  boost::trim(result);
  // circumvent differences b/w lexical_cast and xml specification
  return boost::lexical_cast<convT>(result);
}

/**
 *  Overloading operator '<' to compare two CartesianCoor3D vectors by their components
 * @param that right hand side
 */
bool CartesianCoor3D::operator<(const CartesianCoor3D& that) const
{
  if (this->x < that.x)
    return true;

  if (this->x > that.x)
    return false;

  if (this->y < that.y)
    return true;

  if (this->y > that.y)
    return false;

  if (this->z < that.z)
    return true;

  if (this->z > that.z)
    return false;

  return false;
}

/**
 * Conversion constructor cylinder -> cartesian
 * @param cc a CylinderCoor3D vector
 */
CartesianCoor3D::CartesianCoor3D(CylinderCoor3D cc)
{
  x = cc.r*cos(cc.phi);
  y = cc.r*sin(cc.phi);
  z = cc.z;
}

/**
 * Conversion constructor spherical -> cartesian
 * @param cc a SphericalCoor3D vector
 */
CartesianCoor3D::CartesianCoor3D(SphericalCoor3D cc)
{
  x = cc.r*sin(cc.theta)*cos(cc.phi);
  y = cc.r*sin(cc.theta)*sin(cc.phi);
  z = cc.r*cos(cc.theta);
}

/// return the size of the vector
double CartesianCoor3D::length()
{
  return sqrt(pow(x,2)+pow(y,2)+pow(z,2));
}


std::ostream& operator<<(std::ostream& os, const CartesianCoor3D& cc)
{
  return os << "(x=" << cc.x << ",y=" << cc.y << ",z=" << cc.z << ")";
}


CartesianCoor3D& CartesianCoor3D::operator=(const CartesianCoor3D& that)
{
  if (this != &that) { x=that.x; y=that.y; z=that.z; }
  return *this;
}

CartesianCoor3D CartesianCoor3D::operator-(const CartesianCoor3D& that)
{
  return CartesianCoor3D(x-that.x,y-that.y,z-that.z);
}

CartesianCoor3D CartesianCoor3D::operator+(const CartesianCoor3D& that)
{
  return CartesianCoor3D(x+that.x,y+that.y,z+that.z);
}

double CartesianCoor3D::operator*(const CartesianCoor3D& that)
{
  return (x*that.x+y*that.y+z*that.z);
}

CartesianCoor3D CartesianCoor3D::cross_product(const CartesianCoor3D& that)
{
  return CartesianCoor3D(y*that.z-z*that.y,z*that.x-x*that.z,x*that.y-y*that.x);
}

CartesianCoor3D operator*(const double lambda, const CartesianCoor3D& that)
{
  return CartesianCoor3D(lambda*that.x,lambda*that.y,lambda*that.z);
}

CartesianCoor3D operator*(const CartesianCoor3D& that,const double lambda)
{
  return CartesianCoor3D(lambda*that.x,lambda*that.y,lambda*that.z);
}

CartesianCoor3D operator/(const CartesianCoor3D& that, const double lambda)
{
  return CartesianCoor3D(that.x/lambda,that.y/lambda,that.z/lambda);
}

/**
 * Conversion constructor cartesian -> cylinder
 */
CylinderCoor3D::CylinderCoor3D(double v1,double v2,double v3)
{
  if (v1<0){
    throw "Negative values not allowed for cylinder radius\n";
  }
  r   = v1;
  phi = v2;
  z   = v3;
}

/**
 * Conversion constructor cartesian -> cylinder
 */
CylinderCoor3D::CylinderCoor3D(CartesianCoor3D cc)
{
  float M_PIf = static_cast<float>(M_PI);
  float M_PI_2f = static_cast<float>(M_PI_2);
  float ccyf = static_cast<float>(cc.y);
  r = sqrt(pow(cc.x,2)+pow(cc.y,2));

  if (cc.x!=0.0)
  {
    phi = atan(cc.y/cc.x);
    if (cc.x<0.0)
    {
      phi = static_cast<double>( sign(M_PIf,ccyf) )+phi;
    }
  }
  else if (cc.y!=0.0)
  {
    phi = static_cast<double>( sign(M_PI_2f,ccyf) );
  }
  else
  {
    phi = 0.0;
  }

  phi = phi<0 ? 2*M_PI + phi : phi;

  if ( (phi<0) || (phi>2*M_PI))
  {
    std::cerr << "PHI OUT OF BOUNDS: " << r << ", " << phi << std::endl;
    std::cerr << cc << std::endl;
    throw;
  }
  z = cc.z;
}

/**
 * Conversion constructor spherical -> cylinder
 */
CylinderCoor3D::CylinderCoor3D(SphericalCoor3D cc)
{
  CartesianCoor3D c1(cc);
  CylinderCoor3D c2(c1);
  r = c2.r; phi = c2.phi; z = c2.z;
}

std::ostream& operator<<(std::ostream& os, const CylinderCoor3D& cc)
{
  return os << "(r=" << cc.r << ",phi=" << cc.phi << ",z=" << cc.z << ")";
}

CylinderCoor3D& CylinderCoor3D::operator=(const CylinderCoor3D& that)
{
  if (this != &that) { r=that.r; phi=that.phi; z=that.z; }
  return *this;
}

CylinderCoor3D CylinderCoor3D::operator-(const CylinderCoor3D& that)
{
  return CylinderCoor3D(r-that.r,phi-that.phi,z-that.z);
}

/**
 * Conversion constructor cartesian -> spherical
 */
SphericalCoor3D::SphericalCoor3D(CartesianCoor3D cc)
{
  r = sqrt(pow(cc.x,2)+pow(cc.y,2)+pow(cc.z,2));
  if (r==0)
  {
    theta = 0;
    phi=0;
    return;
  }
  theta = acos(cc.z/r);
  if (theta<0) throw;
  if (theta>M_PI) throw;
  if (cc.x!=0.0)
  {
    phi = atan(cc.y/cc.x);
    if (cc.x<0.0) phi += M_PI;
    else if (cc.y<0.0) phi += 2*M_PI;
  }
  else if (cc.y!=0.0)
  {
    if (cc.y>0) phi=M_PI_2;
    if (cc.y<0) phi=3*M_PI_2;
  }
  else
  {
    phi = 0.0;
  }
  if (phi<0) throw;
  if (phi>2*M_PI) throw;
}

/**
 * Conversion constructor cylinder -> spherical
 */
SphericalCoor3D::SphericalCoor3D(CylinderCoor3D cc)
{
  CartesianCoor3D c1(cc);
  SphericalCoor3D c2(c1);
  r = c2.r; phi = c2.phi; theta = c2.theta;
}

std::ostream& operator<<(std::ostream& os, const SphericalCoor3D& cc)
{
  return os << "(r=" << cc.r << ",phi=" << cc.phi << ",theta=" << cc.theta << ")";
}

SphericalCoor3D& SphericalCoor3D::operator=(const SphericalCoor3D& that)
{
  if (this != &that) { r=that.r; phi=that.phi; theta=that.theta;}
  return *this;
}

SphericalCoor3D SphericalCoor3D::operator-(const SphericalCoor3D& that)
{
  return SphericalCoor3D(r-that.r,phi-that.phi,theta-that.theta);
}

//transformations

CartesianCoor3D rotate(CartesianCoor3D c,std::string axis,double rad)
{
  CartesianCoor3D r;
  if (axis=="x") {
    double m[3][3];
    m[0][0] = 1; m[0][1] = 0; m[0][2] = 0;
    m[1][0] = 0; m[1][1] = cos(rad); m[1][2] = -sin(rad);
    m[2][0] = 0; m[2][1] = sin(rad); m[2][2] = cos(rad);
    r.x = m[0][0]*c.x + m[0][1]*c.y + m[0][2]*c.z;
    r.y = m[1][0]*c.x + m[1][1]*c.y + m[1][2]*c.z;
    r.z = m[2][0]*c.x + m[2][1]*c.y + m[2][2]*c.z;
  }
  if (axis=="y") {
    double m[3][3];
    m[0][0] = cos(rad); m[0][1] = 0; m[0][2] = sin(rad);
    m[1][0] = 0; m[1][1] = 1; m[1][2] = 0;
    m[2][0] = -sin(rad); m[2][1] = 0; m[2][2] = cos(rad);
    r.x = m[0][0]*c.x + m[0][1]*c.y + m[0][2]*c.z;
    r.y = m[1][0]*c.x + m[1][1]*c.y + m[1][2]*c.z;
    r.z = m[2][0]*c.x + m[2][1]*c.y + m[2][2]*c.z;
  }
  if (axis=="z") {
    double m[3][3];
    m[0][0] = cos(rad); m[0][1] = -sin(rad); m[0][2] = 0;
    m[1][0] = sin(rad); m[1][1] = cos(rad); m[1][2] = 0;
    m[2][0] = 0; m[2][1] = 0; m[2][2] = 1;
    r.x = m[0][0]*c.x + m[0][1]*c.y + m[0][2]*c.z;
    r.y = m[1][0]*c.x + m[1][1]*c.y + m[1][2]*c.z;
    r.z = m[2][0]*c.x + m[2][1]*c.y + m[2][2]*c.z;
  }
  return r;
}

CartesianVectorBase::CartesianVectorBase(CartesianCoor3D axis)
{
  CartesianCoor3D ek(0,0,1);
  CartesianCoor3D ej(0,1,0);
  CartesianCoor3D ez = axis/axis.length();
  CartesianCoor3D ekez = ek.cross_product(ez);
  if (ekez.length()==0)
  {
    ekez = ej.cross_product(ez);
  }
  CartesianCoor3D er = ekez/ekez.length();
  CartesianCoor3D ezer = ez.cross_product(er);
  CartesianCoor3D ephi = ezer/ezer.length();
  base_.push_back(er);
  base_.push_back(ephi);
  base_.push_back(ez);
}

CartesianCoor3D& CartesianVectorBase::operator[](size_t index)
{
  return base_.at(index);
}

/// Project the vector vec onto the base and return the 3 components along the three basis vectors
CartesianCoor3D CartesianVectorBase::project(CartesianCoor3D vec)
{
  return CartesianCoor3D(vec*base_[0],vec*base_[1],vec*base_[2]);
}

SampleParameters::~SampleParameters()
{
  for(std::map<std::string,SampleSelectionParameters*>::iterator i=selections.begin();i!=selections.end();i++)
  {
    delete i->second;
  }
}

void ScatteringAverageOrientationVectorsParameters::create()
{
  if (type=="file")
  {
    g_log.information("Reading orientations for orientational averaging from file: "+this->file);
    std::ifstream qqqfile( file.c_str() );
    double x,y,z;
    while (qqqfile >> x >> y >> z)
    {
      CartesianCoor3D q(x,y,z);
      double ql = q.length();
      if (ql!=0) q = (1.0/ql) * q;
      this->push_back(q);
    }
  }
  else if (type=="sphere")
  {
    if (algorithm=="boost_uniform_on_sphere")
    {
      boost::mt19937 rng; // that's my random number generator
      rng.seed( static_cast<unsigned int>(seed) );
      boost::uniform_on_sphere<double> s(3); // that's my distribution
      boost::variate_generator<boost::mt19937, boost::uniform_on_sphere<double> > mysphere(rng,s);
      g_log.information("Generating orientations for orientational averaging using sphere,boost_uniform_on_sphere");
      for(size_t i = 0; i < resolution; ++i)
      {
        vector<double> r = mysphere();
        this->push_back( CartesianCoor3D(r[0],r[1],r[2]) );
      }
    }
    else
    {
      g_log.error("Vectors algorithm not understood: "+this->algorithm);
      throw;
      }
    }
  else if (type=="cylinder")
  {
    if (algorithm=="boost_uniform_on_sphere")
    {
      boost::mt19937 rng; // that's my random number generator
      rng.seed( static_cast<unsigned int>(seed) );
      boost::uniform_on_sphere<double> s(2); // that's my distribution
      boost::variate_generator<boost::mt19937, boost::uniform_on_sphere<double> > mysphere(rng,s);
      g_log.information("Generating orientations for orientational averaging using cylinder,boost_uniform_on_sphere");
      for(size_t i = 0; i < resolution; ++i)
      {
        vector<double> r = mysphere();
        this->push_back( CartesianCoor3D(r[0],r[1],0) );
      }
    }
    else if (algorithm=="raster_linear")
    {
      const double M_2PI = 2*M_PI;
      const double radincr = (M_2PI) / ( 360.0*static_cast<double>(resolution) );
      g_log.information("Generating orientations for orientational averaging using cylinder,raster_linear");
      for (double phi=0;phi<M_2PI;phi+=radincr)
      {
        this->push_back( CartesianCoor3D(cos(phi),sin(phi),0) );
      }
    }
    else
      {
        g_log.error("Vectors algorithm not understood: "+this->algorithm);
        throw;
      }
    }
  else
  {
    g_log.error("Vectors orientation averaging type not understood: "+this->type);
    throw;
  }
  g_log.information("Initialized orientational averaging with "+boost::lexical_cast<std::string>(this->size())+" vectors.");
}// end of ScatteringAverageOrientationVectorsParameters::create()

void ScatteringAverageOrientationMultipoleMomentsParameters::create()
{
  if(type=="file")
  {
    g_log.information("Reading multipole moments for orientational averaging from file: "+this->file);
    std::ifstream mmfile(this->file.c_str());
    long major, minor;
    while (mmfile >> major >> minor)
    {
      this->push_back(std::make_pair(major,minor));
    }
  }
  else if(type=="resolution")
  {
    g_log.information("Generating multipole moments for orientational averaging using a maxium major="+boost::lexical_cast<std::string>(resolution));
    if (Params::Inst()->scattering.average.orientation.multipole.type=="sphere")
    {
      this->push_back(std::make_pair(0L,0L));
      for(long l = 1; l <= resolution; ++l)
      {
        for(long m = -l; m <= l; ++m)
        {
          this->push_back(std::make_pair(l,m));
        }
      }
    }
    else if(Params::Inst()->scattering.average.orientation.multipole.type=="cylinder")
    {
      this->push_back(std::make_pair(0L,0L));
      for(long l = 1; l <= resolution; ++l)
      {
        this->push_back(std::make_pair(l,0L));
        this->push_back(std::make_pair(l,1L));
        this->push_back(std::make_pair(l,2L));
        this->push_back(std::make_pair(l,3L));
      }
    }
    else
    {
      g_log.error("Type not understood: scattering.average.orientation.multipole.moments.type="+type);
      throw;
    }
  }
  else
  {
    g_log.error("Type not understood: scattering.average.orientation.multipole.type="+Params::Inst()->scattering.average.orientation.multipole.type);
        throw;
    }

  // check validity of moments
  g_log.information("Checking multipole moments.");
  if (Params::Inst()->scattering.average.orientation.multipole.type=="cylinder")
  {
    for(size_t i = 0; i < size(); ++i)
    {
      std::pair<long,long> mm = this->at(i);
      if (mm.first<0) {
        g_log.error("Major multipole moment must be >= 0!");
        g_log.error("major="+boost::lexical_cast<std::string>(mm.first)+", minor="+boost::lexical_cast<std::string>(mm.second));
        throw;
      }
      if ((mm.second<0) || (mm.second>3) )
      {
        g_log.error("Minor multipole moment must be between 0 and 3!");
        g_log.error("major="+boost::lexical_cast<std::string>(mm.first)+", minor="+boost::lexical_cast<std::string>(mm.second));
        throw;
      }
      if ((mm.first==0) && (mm.second!=0) )
      {
        g_log.error("Minor multipole moment must be 0 for Major 0!");
        g_log.error("major="+boost::lexical_cast<std::string>(mm.first)+", minor="+boost::lexical_cast<std::string>(mm.second));
        throw;
      }
    }
  }
  else if(Params::Inst()->scattering.average.orientation.multipole.type=="sphere")
  {
    for(size_t i = 0; i < size(); ++i)
    {
      std::pair<long,long> mm = this->at(i);
      if (mm.first<0)
      {
        g_log.error("Major multipole moment must be >= 0!");
        g_log.error("major="+boost::lexical_cast<std::string>(mm.first));
        throw;
      }
      if (labs(mm.second)>mm.first)
      {
        g_log.error("Minor multipole moment must be between -Major and +Major!");
        g_log.error("major="+boost::lexical_cast<std::string>(mm.first)+", minor="+boost::lexical_cast<std::string>(mm.second));
        throw;
      }
    }
  }
  g_log.information("Initialized orientational averaging with "+boost::lexical_cast<std::string>(size())+" moments.");
} // end of ScatteringAverageOrientationMultipoleMomentsParameters::create()

/// read out the scans and push the results onto the internal vector
void ScatteringVectorsParameters::create_from_scans()
{
  if (scans.size()>3)
  {
    g_log.error("More than 3 scan definitions are not supported.");
    throw;
  }
  // local vector unfolds each scan first, then we'll do element-wise vector addition
  vector< vector<CartesianCoor3D> > qvectors(scans.size());
  for(size_t i = 0; i < scans.size(); ++i)
  {
    if (scans[i].points==0) continue;
    if (scans[i].points==1)
    {
      double scal = (scans[i].from+scans[i].to)/2;
      qvectors[i].push_back(scal*scans[i].basevector);
      continue;
    }
    if (scans[i].points==2)
    {
      qvectors[i].push_back(scans[i].from*scans[i].basevector);
      qvectors[i].push_back(scans[i].to*scans[i].basevector);
      continue;
    }
    // else
    qvectors[i].push_back(scans[i].from*scans[i].basevector);
    for (size_t j=1;j<(scans[i].points-1);j++)
    {
      double base = static_cast<double>(j)*1.0/(static_cast<double>(scans[i].points)-1.0);
      double scal = scans[i].from + pow(base,scans[i].exponent)*(scans[i].to-scans[i].from);
      qvectors[i].push_back(scal*scans[i].basevector);
    }
    qvectors[i].push_back(scans[i].to*scans[i].basevector);
  }
  // trivial case: only one scan!
  if (scans.size()==1)
  {
    for(size_t i = 0; i < qvectors[0].size(); ++i)
    {
      this->push_back(qvectors[0][i]);
    }
    return;
  }
  // if scans.size()>1 , not trivial..
  if (scans.size()==2)
  {
    for(size_t i = 0; i < qvectors[0].size(); ++i)
    {
      for(size_t j = 0; j < qvectors[1].size(); ++j)
      {
        this->push_back(qvectors[0][i]+qvectors[1][j]);
      }
    }
  }
  if (scans.size()==3)
  {
    for(size_t i = 0; i < qvectors[0].size(); ++i)
    {
      for(size_t j = 0; j < qvectors[1].size(); ++j)
      {
        for(size_t k = 0; k < qvectors[2].size(); ++k)
        {
          this->push_back(qvectors[0][i]+qvectors[1][j]+qvectors[2][k]);
        }
      }
    }
  }
} // end of ScatteringVectorsParameters::create_from_scans()

std::string Params::get_filepath(std::string fname)
{
  using namespace boost::filesystem;
  path fpath(fname);
  std::string fdir;
  if(fpath.parent_path().is_complete())
    fdir = fpath.parent_path().string();
  else if (!config_rootpath.empty())
    fdir = (path(config_rootpath) / fpath.parent_path()).string();
  else
    fdir = (initial_path() / fpath.parent_path()).string();
  return (path(fdir) / fpath.filename()).string();
}

void Params::write_xml_to_file(std::string filename)
{
  std::ofstream conf(filename.c_str());
  conf << write_xml();
  conf.close();
}

/// store the configuration line by line into an internal buffer
void Params::read_xml(std::string filename)
{
  // this is for keeping history
  std::ifstream conffile(filename.c_str());
  char c;
  while (conffile.good())
  {
    conffile.get(c);
    if (conffile.good())
    {
      rawconfig.push_back(c);
    }
  }
  conffile.close();
  // START OF sample section //
  XMLInterface xmli(filename);
  g_log.information("Reading parameters from XML file: "+filename);
  xmli.dump(config);
  // now read the parameters
  sample.structure.file="sample.pdb";
  sample.structure.filepath = get_filepath(sample.structure.file);
  sample.structure.format="pdb";
  if (xmli.exists("//sample"))
  {
    if (xmli.exists("//sample/structure"))
    {
      if (xmli.exists("//sample/structure/file"))
      {
        sample.structure.file = get_filepath(xmli.get_value<std::string>("//sample/structure/file"));
        sample.structure.filepath = get_filepath(sample.structure.file);
      }
      if (xmli.exists("//sample/structure/format"))
      {
        sample.structure.format   = xmli.get_value<std::string>("//sample/structure/format");
      }
    }
    if (xmli.exists("//sample/selections"))
    {
      std::vector<XMLElement> selections = xmli.get("//sample/selections/selection");
      for(size_t i = 0; i < selections.size(); ++i)
      {
        xmli.set_current(selections[i]);
        std::string type = "index";
        if (xmli.exists("./type")) type = xmli.get_value<std::string>("./type");
        if (type=="index")
        {
          std::string sname = "_"+boost::lexical_cast<std::string>(i);
          std::vector<size_t> ids;
          if (xmli.exists("./name")) sname = xmli.get_value<std::string>("./name") ;
          std::vector<XMLElement> indexes = xmli.get("./index");
          for(size_t pi = 0; pi < indexes.size(); ++pi)
          {
            xmli.set_current(indexes[pi]);
            size_t index = xmli.get_value<size_t>(".");
            ids.push_back(index);
          }
          sample.selections[sname] = new SampleIndexSelectionParameters(ids);
          g_log.information("Creating Index Atomselection: "+sname+" , elements:"+boost::lexical_cast<std::string>(ids.size()));
        }
        else if (type=="range")
        {
          std::string sname = "_"+boost::lexical_cast<std::string>(i);
          size_t from = 0;
          size_t to = 0;
          if (xmli.exists("./name")) sname = xmli.get_value<std::string>("./name");
          if (xmli.exists("./from")) from = xmli.get_value<size_t>("./from");
          if (xmli.exists("./to")) to = xmli.get_value<size_t>("./to") ;
          sample.selections[sname] = new SampleRangeSelectionParameters(from,to);
          g_log.information("Creating Range Atomselection: " + sname +
            " , from:"+boost::lexical_cast<std::string>(from) +
            ", to: "+boost::lexical_cast<std::string>(to) );
        }
        else if (type=="lexical")
        {
          std::string sname = "_"+boost::lexical_cast<std::string>(i);
          std::string expression = "";
          if (xmli.exists("./name")) sname = xmli.get_value<std::string>("./name");
          if (xmli.exists("./expression")) expression = xmli.get_value<std::string>("./expression");
          sample.selections[sname] = new SampleLexicalSelectionParameters(expression);
          g_log.information("Creating Lexical Atomselection: "+sname+" , expression:"+expression);
        }
        else if (type=="file")
        {
          std::string sname = "_"+boost::lexical_cast<std::string>(i); // NOT used by ndx file format
          std::string file  = "selection.pdb";
          std::string filepath = get_filepath(file);
          std::string format = "pdb";
          std::string selector = "beta";
          std::string expression = "1|1\\.0|1\\.00";
          if (xmli.exists("./name")) sname = xmli.get_value<std::string>("./name");
          if (xmli.exists("./format")) format = xmli.get_value<std::string>("./format");
          // this is a convenience overwrite for the ndx file format
          if (format=="ndx")
          {
            filename = "index.ndx";
            selector = "name";
            expression = ".*";
          }
          if (xmli.exists("./file"))
          {
            filename = xmli.get_value<std::string>("./file");
            filepath = get_filepath(file);
          }
          if (xmli.exists("./selector")) selector = xmli.get_value<std::string>("./selector");
          if (xmli.exists("./expression")) expression = xmli.get_value<std::string>("./expression");
          sample.selections[sname] = new SampleFileSelectionParameters(filepath,format,selector,expression);
          g_log.information("Creating File Atomselection: "+sname+" , file: "+filepath+
            ", format: "+format+", selector: "+selector+", expression: "+expression);
        }
      }
    }
    if (xmli.exists("//sample/framesets"))
    {
      size_t def_first=0;
      size_t def_last=0;
      bool def_last_set=false;
      size_t def_stride = 1;
      size_t def_clones = 1;
      std::string def_format = "dcd";
      std::string def_file="sample.dcd";
      if (xmli.exists("//sample/framesets/first")) def_first = xmli.get_value<size_t>("//sample/framesets/first");
      if (xmli.exists("//sample/framesets/last"))
      {
        def_last = xmli.get_value<size_t>("//sample/framesets/last");
        def_last_set = true;
      }
      if (xmli.exists("//sample/framesets/stride")) def_stride = xmli.get_value<size_t>("//sample/framesets/stride");
      if (xmli.exists("//sample/framesets/clones")) def_clones = xmli.get_value<size_t>("//sample/framesets/clones");
      if (xmli.exists("//sample/framesets/format")) def_first = xmli.get_value<size_t>("//sample/framesets/format");
      // read in frame information
      std::vector<XMLElement> framesets = xmli.get("//sample/framesets/frameset");
      for(size_t i = 0; i < framesets.size(); ++i)
      {
        xmli.set_current(framesets[i]);
        SampleFramesetParameters fset;
        fset.first = def_first;
        fset.last = def_last;
        fset.last_set = def_last_set;
        fset.stride = def_stride;
        fset.format = def_format;
        fset.clones = def_clones;
        fset.file = def_file;
        fset.filepath = get_filepath(fset.file);
        if (xmli.exists("./file"))
        {
          fset.file = xmli.get_value<std::string>("./file");
          fset.filepath = get_filepath(fset.file);
        }
        boost::filesystem::path index_path = get_filepath(xmli.get_value<std::string>("./file"));
        fset.index = index_path.parent_path().string()+"/"+index_path.stem().string()+".tnx";
        fset.index_default = true;
        if (xmli.exists("./format")) fset.format = xmli.get_value<std::string>("./format");
        if (xmli.exists("./first")) fset.first = xmli.get_value<size_t>("./first");
        if (xmli.exists("./last"))
        {
          fset.last = xmli.get_value<size_t>("./last");
          fset.last_set = true;
        }
        if (xmli.exists("./stride"))  fset.stride = xmli.get_value<size_t>("./stride");
        if (xmli.exists("./clones"))  fset.clones = xmli.get_value<size_t>("./clones");
        if (xmli.exists("./index"))
        {
          fset.index = get_filepath(xmli.get_value<std::string>("./index"));
          fset.indexpath = get_filepath(fset.index);
          fset.index_default = false;
        }
        sample.framesets.push_back(fset);
        g_log.information("Added frames from "+fset.filepath+" using format: "+fset.format);
        g_log.information("Options: first="+boost::lexical_cast<std::string>(fset.first)+
          ", last="+boost::lexical_cast<std::string>(fset.last)+
          ", lastset="+boost::lexical_cast<std::string>(fset.last_set)+
          ", stride="+boost::lexical_cast<std::string>(fset.stride));
      }
    }
    if (xmli.exists("//sample/motions"))
    {
      std::vector<XMLElement> motions = xmli.get("//sample/motions/motion");
      for(size_t i = 0; i < motions.size(); ++i)
      {
        xmli.set_current(motions[i]);
        SampleMotionParameters motion;
        motion.type = "linear";
        motion.displace = 0.0;
        motion.direction=CartesianCoor3D(1,0,0);
        motion.selection = "system";
        motion.seed = 0;
        motion.sampling = 1;
        // corresponds to one full cycle per 1000 frames, used for linear oscillation and rotation
        motion.frequency=0.001;
        motion.reference.type = "instant";
        motion.reference.frame = 0;
        motion.reference.file = sample.structure.file;
        motion.reference.filepath = get_filepath(motion.reference.file);
        motion.reference.format = sample.structure.format;
        motion.reference.selection = motion.selection;
        if (xmli.exists("./type")) motion.type = xmli.get_value<std::string>("./type");
        if (xmli.exists("./displace")) motion.displace = xmli.get_value<double>("./displace");
        if (xmli.exists("./frequency")) motion.frequency = xmli.get_value<double>("./frequency");
        if (xmli.exists("./seed")) motion.seed = xmli.get_value<unsigned long>("./seed");
        if (xmli.exists("./sampling")) motion.sampling = xmli.get_value<long>("./sampling");
        if (xmli.exists("./selection")) motion.selection = xmli.get_value<std::string>("./selection");
        if (xmli.exists("./direction")) {
        if (xmli.exists("./direction/x")) motion.direction.x = xmli.get_value<double>("./direction/x");
        if (xmli.exists("./direction/y")) motion.direction.y = xmli.get_value<double>("./direction/y");
        if (xmli.exists("./direction/z")) motion.direction.z = xmli.get_value<double>("./direction/z");
      }
       motion.radius = motion.displace*10;
      if (xmli.exists("./radius")) motion.radius = xmli.get_value<double>("./radius");
      if (xmli.exists("./reference"))
      {
        if (xmli.exists("./reference/type")) motion.reference.type = xmli.get_value<std::string>("./reference/type");
        if (xmli.exists("./reference/frame")) motion.reference.frame = xmli.get_value<size_t>("./reference/frame");
        if (xmli.exists("./reference/file"))
        {
          motion.reference.file = xmli.get_value<std::string>("./reference/file");
          motion.reference.filepath   = get_filepath(motion.reference.file);
        }
        if (xmli.exists("./reference/format")) motion.reference.format = xmli.get_value<std::string>("./reference/format");
        if (xmli.exists("./reference/selection")) motion.reference.selection = xmli.get_value<std::string>("./reference/selection");
      }
      if (motion.type=="file")
      {
        g_log.information("Reference for motion alignment: type="+motion.reference.type+
          ", file="+motion.reference.file+", format="+motion.reference.format+
          ", frame="+boost::lexical_cast<std::string>(motion.reference.frame));
      }
      else if (motion.type=="frame")
      {
        g_log.information("Reference for motion alignment: type="+motion.reference.type+
          ", frame="+boost::lexical_cast<std::string>(motion.reference.frame));
        g_log.information("Motion Alignment uses unprocessed coordinates (No alignment and no applied motions)");
      }
      else if (motion.type=="instant")
      {
        g_log.information("Instant motion alignment (Uses coordinates within the current frame)");
      }
      g_log.information("Selection used for alignment with reference="+motion.reference.selection);
      sample.motions.push_back(motion);
      g_log.information("Adding additional motion to sample: type="+motion.type
        +", displacement="+boost::lexical_cast<std::string>(motion.displace)
        +", selection="+motion.selection);
      }
    }
    if (xmli.exists("//sample/alignments"))
    {
      std::vector<XMLElement> alignments = xmli.get("//sample/alignments/alignment");
      for(size_t i = 0; i < alignments.size(); ++i)
      {
        xmli.set_current(alignments[i]);
        SampleAlignmentParameters alignment;
        alignment.type = "center";
        alignment.selection = "system";
        alignment.order = "pre";
        if (xmli.exists("./type")) alignment.type = xmli.get_value<std::string>("./type");
        if (xmli.exists("./selection")) alignment.selection = xmli.get_value<std::string>("./selection");
        if (xmli.exists("./order")) alignment.order = xmli.get_value<std::string>("./order");
        g_log.information("Adding additional alignment to sample: type="+alignment.type
          +", selection="+alignment.selection+", order="+alignment.order);
        alignment.reference.type = "frame";
        alignment.reference.frame = 0;
        alignment.reference.file = sample.structure.file;
        alignment.reference.filepath   = get_filepath(alignment.reference.file);
        alignment.reference.format = sample.structure.format;
        alignment.reference.selection = alignment.selection;
        if (xmli.exists("./reference"))
        {
          if (xmli.exists("./reference/type")) alignment.reference.type = xmli.get_value<std::string>("./reference/type");
          if (xmli.exists("./reference/frame")) alignment.reference.frame = xmli.get_value<size_t>("./reference/frame");
          if (xmli.exists("./reference/file"))
          {
            alignment.reference.file = xmli.get_value<std::string>("./reference/file");
            alignment.reference.filepath   = get_filepath(alignment.reference.file);
          }
          if (xmli.exists("./reference/format")) alignment.reference.format = xmli.get_value<std::string>("./reference/format");
          if (xmli.exists("./reference/selection")) alignment.reference.selection = xmli.get_value<std::string>("./reference/selection");
        }
        if (alignment.type=="file")
        {
          g_log.information("Reference for alignment: type="+alignment.reference.type
           +", file="+alignment.reference.filepath+", format="+alignment.reference.format
           +", frame="+boost::lexical_cast<std::string>(alignment.reference.frame));
         }
        else if (alignment.type=="frame")
        {
          g_log.information("Reference for alignment: type="+alignment.reference.type
          +", frame="+boost::lexical_cast<std::string>(alignment.reference.frame));
          g_log.information("Alignment uses unprocessed coordinates (No alignment and no applied motions)");
        }
        g_log.information("Selection used for alignment with reference="+alignment.reference.selection);
        sample.alignments.push_back(alignment);
      }
    }
  }
  stager.dump=false;
  stager.file="dump.dcd";
  stager.filepath=get_filepath(stager.file);
  stager.format="dcd";
  stager.target = "system";
  stager.mode = "frames";
  if (xmli.exists("//stager"))
  {
    if (xmli.exists("//stager/dump"))
    {
      stager.dump = xmli.get_value<bool>("//stager/dump");
    }
    if (xmli.exists("//stager/file"))
    {
      stager.file = xmli.get_value<std::string>("//stager/file");
      stager.filepath=get_filepath(stager.file);
    }
    if (xmli.exists("//stager/format"))
    {
      stager.format = xmli.get_value<std::string>("//stager/format");
    }
    if (xmli.exists("//stager/target"))
    {
      stager.target = xmli.get_value<std::string>("//stager/target");
    }
    if (xmli.exists("//stager/mode"))
    {
      stager.mode = xmli.get_value<std::string>("//stager/mode");
    }
  }
  g_log.information("stager.target="+stager.target);
  scattering.type="all";
  scattering.background.factor = 0.0;
  scattering.dsp.type="autocorrelate";
  scattering.dsp.method="fftw";
  // defaults
  scattering.average.orientation.type = "none";
  scattering.average.orientation.axis = CartesianCoor3D(0,0,1);
  scattering.average.orientation.vectors.type = "sphere";
  scattering.average.orientation.vectors.algorithm = "boost_uniform_on_sphere";
  scattering.average.orientation.vectors.resolution = 100;
  scattering.average.orientation.vectors.seed = 0;
  scattering.average.orientation.vectors.file = "qvector-orientations.txt";
  scattering.average.orientation.vectors.filepath = get_filepath(scattering.average.orientation.vectors.file);
  scattering.average.orientation.multipole.type = "sphere";
  scattering.average.orientation.multipole.moments.resolution = 20;
  scattering.average.orientation.multipole.moments.file = "moments.txt";
  scattering.average.orientation.multipole.moments.filepath = get_filepath(scattering.average.orientation.multipole.moments.file);
  scattering.average.orientation.exact.type = "sphere";
  scattering.signal.file = "signal.h5";
  scattering.signal.filepath = get_filepath(scattering.signal.file);
  scattering.signal.fqt = true;
  scattering.signal.fq0 = true;
  scattering.signal.fq = true;
  scattering.signal.fq2 = true;
  if (xmli.exists("//scattering"))
  {
    if (xmli.exists("//scattering/type"))
    {
      scattering.type = xmli.get_value<std::string>("//scattering/type");
    }
    if (xmli.exists("//scattering/target")) {
      g_log.error("scattering.target is obsolete. Use stager.target instead.");
      throw;
    }
    g_log.information("scattering.type="+scattering.type);
    if (xmli.exists("//scattering/background"))
    {
      if (xmli.exists("//scattering/background/factor"))
      {
        scattering.background.factor = xmli.get_value<double>("//scattering/background/factor");
        g_log.information("scattering.background.factor="+boost::lexical_cast<std::string>(scattering.background.factor));
      }
      if (xmli.exists("//scattering/background/kappas"))
      {
        std::vector<XMLElement> kappas = xmli.get("//scattering/background/kappas/kappa");
        for(size_t i = 0; i < kappas.size(); ++i)
        {
          xmli.set_current(kappas[i]);
          ScatteringBackgroundKappaParameters kappa;
          kappa.selection = "system";
          kappa.value = 1.0;
          if (xmli.exists("./selection")) kappa.selection = xmli.get_value<std::string>("./selection");
          if (xmli.exists("./value"))  kappa.value   = xmli.get_value<double>("./value");
          scattering.background.kappas.push_back(kappa);
          g_log.information("Rescaling volumes: selection="+kappa.selection
            +", value="+boost::lexical_cast<std::string>(kappa.value));
        }
      }
    }
    // generating qqqvectors, i.e. the spectrum
    if (xmli.exists("//scattering/vectors"))
    {
      std::string vt = "single";
      if (xmli.exists("//scattering/vectors/type")) vt = xmli.get_value<std::string>("//scattering/vectors/type");
      if (vt=="single")
      {
        double x = 1.0;
        double y = 0;
        double z = 0;
        if (xmli.exists("//scattering/vectors/single/x"))
        {
          if (xmli.exists("//scattering/vectors/single/x")) x = xmli.get_value<double>("//scattering/vectors/single/x");
          if (xmli.exists("//scattering/vectors/single/y")) y = xmli.get_value<double>("//scattering/vectors/single/y");
          if (xmli.exists("//scattering/vectors/single/z")) z = xmli.get_value<double>("//scattering/vectors/single/z");
        }
        scattering.qvectors.push_back(CartesianCoor3D(x,y,z));
      }
      else if (vt=="scans")
      {
        if (xmli.exists("//scattering/vectors/scans"))
        {
          std::vector<XMLElement> scans = xmli.get("//scattering/vectors/scans/scan");
          for(size_t i = 0; i < scans.size(); ++i)
          {
            xmli.set_current(scans[i]);
            ScatteringVectorsScanParameters sc;
            sc.basevector.x = 1.0;
            sc.basevector.y = 0;
            sc.basevector.z = 0;
            sc.from = 0;
            sc.to = 1;
            sc.points = 100;
            sc.exponent = 1.0;
            if (xmli.exists("./base"))
            {
              if (xmli.exists("./base/x")) sc.basevector.x = xmli.get_value<double>("./base/x");
              if (xmli.exists("./base/y")) sc.basevector.y = xmli.get_value<double>("./base/y");
              if (xmli.exists("./base/z")) sc.basevector.z = xmli.get_value<double>("./base/z");
            }
            if (xmli.exists("./from")) sc.from = xmli.get_value<double>("./from");
            if (xmli.exists("./to")) sc.to = xmli.get_value<double>("./to");
            if (xmli.exists("./points")) sc.points = xmli.get_value<size_t>("./points");
            if (xmli.exists("./exponent")) sc.exponent = xmli.get_value<double>("./exponent");
            scattering.qvectors.scans.push_back(sc);
          }
        }
        scattering.qvectors.create_from_scans();
      }
      else if (vt=="file")
      {
        std::string qqqfilename = "qvectors.txt";
        if (xmli.exists("//scattering/vectors/file")) qqqfilename = get_filepath(xmli.get_value<std::string>("//scattering/vectors/file"));
        std::ifstream qqqfile(qqqfilename.c_str());
        double x,y,z;
        while (qqqfile >> x >> y >> z)
        {
          scattering.qvectors.push_back(CartesianCoor3D(x,y,z));
        }
      }
    }
    if (scattering.qvectors.size()==0)
    {
      g_log.error("No q vectors generated. Check the scattering.vectors section for errors.");
      throw;
    }
    if (xmli.exists("//scattering/dsp"))
    {
      if (xmli.exists("//scattering/dsp/type"))
        {
          scattering.dsp.type = xmli.get_value<std::string>("//scattering/dsp/type");
          g_log.information("scattering.dsp.type="+scattering.dsp.type);
        }
      if (xmli.exists("//scattering/dsp/method"))
      {
        scattering.dsp.method = xmli.get_value<std::string>("//scattering/dsp/method");
        g_log.information("scattering.dsp.method="+scattering.dsp.method);
      }
    }
    if (xmli.exists("//scattering/average"))
    {
      if (xmli.exists("//scattering/average/orientation"))
      {
        // count vectors ... , or order for multipole...
        if (xmli.exists("//scattering/average/orientation/axis"))
        {
          scattering.average.orientation.axis.x = xmli.get_value<double>("//scattering/average/orientation/axis/x");
          scattering.average.orientation.axis.y = xmli.get_value<double>("//scattering/average/orientation/axis/y");
          scattering.average.orientation.axis.z = xmli.get_value<double>("//scattering/average/orientation/axis/z");
        }
        // vectors multipole
        if (xmli.exists("//scattering/average/orientation/type"))
        {
          scattering.average.orientation.type = xmli.get_value<std::string>("//scattering/average/orientation/type");
        }
        g_log.information("scattering.average.orientation.type="+scattering.average.orientation.type);
        if (scattering.average.orientation.type=="vectors")
        {
          if (xmli.exists("//scattering/average/orientation/vectors/type"))
          {
            scattering.average.orientation.vectors.type = xmli.get_value<std::string>("//scattering/average/orientation/vectors/type");
            g_log.information("scattering.average.orientation.vectors.type="+scattering.average.orientation.vectors.type);
          }
          if (xmli.exists("//scattering/average/orientation/vectors/algorithm"))
          {
            scattering.average.orientation.vectors.algorithm = xmli.get_value<std::string>("//scattering/average/orientation/vectors/algorithm");
            g_log.information("scattering.average.orientation.vectors.algorithm="+scattering.average.orientation.vectors.algorithm);
          }
          if (xmli.exists("//scattering/average/orientation/vectors/resolution"))
          {
            scattering.average.orientation.vectors.resolution = xmli.get_value<long>("//scattering/average/orientation/vectors/resolution");
            g_log.information("scattering.average.orientation.vectors.resolution="
              +boost::lexical_cast<std::string>(scattering.average.orientation.vectors.resolution));
          }
          if (xmli.exists("//scattering/average/orientation/vectors/seed"))
          {
            scattering.average.orientation.vectors.seed = xmli.get_value<unsigned long>("//scattering/average/orientation/vectors/seed");
            g_log.information("scattering.average.orientation.vectors.seed="
              +boost::lexical_cast<std::string>(scattering.average.orientation.vectors.seed));
          }
          if (xmli.exists("//scattering/average/orientation/vectors/file"))
          {
            scattering.average.orientation.vectors.file = get_filepath(xmli.get_value<std::string>("//scattering/average/orientation/vectors/file"));
            scattering.average.orientation.vectors.filepath = get_filepath(scattering.average.orientation.vectors.file);
            g_log.information("scattering.average.orientation.vectors.file="
              +boost::lexical_cast<std::string>(scattering.average.orientation.vectors.file));
            g_log.information("scattering.average.orientation.vectors.filepath="
              +boost::lexical_cast<std::string>(scattering.average.orientation.vectors.filepath));
          }
          scattering.average.orientation.vectors.create();
        }
        else if (scattering.average.orientation.type=="multipole")
        {
          if (xmli.exists("//scattering/average/orientation/multipole/type"))
          {
            scattering.average.orientation.multipole.type = xmli.get_value<std::string>("//scattering/average/orientation/multipole/type");
            g_log.information("scattering.average.orientation.multipole.type="+
              scattering.average.orientation.multipole.type);
          }
          if (xmli.exists("//scattering/average/orientation/multipole/moments"))
          {
            if (xmli.exists("//scattering/average/orientation/multipole/moments/type"))
            {
              scattering.average.orientation.multipole.moments.type = xmli.get_value<std::string>("//scattering/average/orientation/multipole/moments/type");
              g_log.information("scattering.average.orientation.multipole.moments.type="
                +boost::lexical_cast<std::string>(scattering.average.orientation.multipole.moments.type));
            }
            if (xmli.exists("//scattering/average/orientation/multipole/moments/resolution"))
            {
              scattering.average.orientation.multipole.moments.resolution = xmli.get_value<long>("//scattering/average/orientation/multipole/moments/resolution");
              g_log.information("scattering.average.orientation.multipole.moments.resolution="
                +boost::lexical_cast<std::string>(scattering.average.orientation.multipole.moments.resolution));
            }
            if (xmli.exists("//scattering/average/orientation/multipole/moments/file"))
            {
              scattering.average.orientation.multipole.moments.file = xmli.get_value<std::string>("//scattering/average/orientation/multipole/moments/file");
              scattering.average.orientation.multipole.moments.filepath = get_filepath(scattering.average.orientation.multipole.moments.file);
              g_log.information("scattering.average.orientation.multipole.moments.file="
                +boost::lexical_cast<std::string>(scattering.average.orientation.multipole.moments.file));
              g_log.information("scattering.average.orientation.multipole.moments.filepath="
                +boost::lexical_cast<std::string>(scattering.average.orientation.multipole.moments.filepath));
            }
          }
          scattering.average.orientation.multipole.moments.create();
        }
        else if (scattering.average.orientation.type!="none")
        {
          g_log.error("Orientation averaging type not understood: "+scattering.average.orientation.type);
          throw;
        }
      }
    }
    if (xmli.exists("//scattering/signal"))
    {
      if (xmli.exists("//scattering/signal/file"))
      {
        scattering.signal.file = xmli.get_value<std::string>("//scattering/signal/file");
        scattering.signal.filepath = get_filepath(scattering.signal.file);
      }
      if (xmli.exists("//scattering/signal/fqt"))
      {
        scattering.signal.fqt = xmli.get_value<bool>("//scattering/signal/fqt");
      }
      if (xmli.exists("//scattering/signal/fq0"))
      {
        scattering.signal.fq0 = xmli.get_value<bool>("//scattering/signal/fq0");
      }
      if (xmli.exists("//scattering/signal/fq"))
      {
        scattering.signal.fq = xmli.get_value<bool>("//scattering/signal/fq");
      }
      if (xmli.exists("//scattering/signal/fq2"))
      {
        scattering.signal.fq2 = xmli.get_value<bool>("//scattering/signal/fq2");
      }
    }
    g_log.information("scattering.signal.file="+scattering.signal.file);
    g_log.information("scattering.signal.filepath="+scattering.signal.filepath);
    g_log.information("scattering.signal.fqt="+boost::lexical_cast<std::string>(scattering.signal.fqt));
    g_log.information("scattering.signal.fq0="+boost::lexical_cast<std::string>(scattering.signal.fq0));
    g_log.information("scattering.signal.fq="+boost::lexical_cast<std::string>(scattering.signal.fq));
    g_log.information("scattering.signal.fq2="+boost::lexical_cast<std::string>(scattering.signal.fq2));
  }
  // END OF scattering section
  // START OF limits section
  // assign default memory limits:
  limits.stage.memory.buffer = 100*1024*1024;
  limits.stage.memory.data   = 500*1024*1024;
  limits.signal.chunksize = 10000;
  limits.computation.cores = 1;
  limits.computation.processes = 1;
  limits.computation.threads = 1;
  limits.computation.memory.result_buffer = 100*1024*1024; // 100MB
  limits.computation.memory.signal_buffer = 100*1024*1024; // 100MB
  limits.computation.memory.exchange_buffer = 100*1024*1024; // 100MB
  limits.computation.memory.alignpad_buffer = 200*1024*1024; // 200MB
  limits.computation.memory.scale = 1;
  limits.services.signal.memory.server = 100*1024*1024; // 100MB
  limits.services.signal.memory.client = 10*1024*1024; // 10MB
  limits.services.signal.times.serverflush = 600; // 600 seconds
  limits.services.signal.times.clientflush = 600; // 600 seconds
  limits.services.monitor.delay = 1; // 1 second
  limits.services.monitor.sampling = 0; // 0 = automatic
  limits.decomposition.utilization = 0.95; // 5% max loss
  limits.decomposition.partitions.automatic = true; // pick number of independent partitions based on some heuristics
  limits.decomposition.partitions.size = 1; // not used if automatic = true, if false -> this determines the partition size
  if (xmli.exists("//limits"))
  {
    if (xmli.exists("//limits/stage"))
    {
      if (xmli.exists("//limits/stage/memory"))
      {
        if (xmli.exists("//limits/stage/memory/buffer"))
        {
          limits.stage.memory.buffer = xmli.get_value<size_t>("//limits/stage/memory/buffer");
        }
        if (xmli.exists("//limits/stage/memory/data"))
        {
          limits.stage.memory.data = xmli.get_value<size_t>("//limits/stage/memory/data");
        }
      }
    }
    if (xmli.exists("//limits/signal"))
    {
      if (xmli.exists("//limits/signal/chunksize"))
      {
        limits.signal.chunksize = xmli.get_value<size_t>("//limits/signal/chunksize");
      }
    }
    if (xmli.exists("//limits/computation"))
    {
      size_t cores = boost::thread::hardware_concurrency();
      if (xmli.exists("//limits/computation/cores"))
      {
        limits.computation.cores = xmli.get_value<size_t>("//limits/computation/cores");
        g_log.information("Assuming "+boost::lexical_cast<std::string>(limits.computation.cores)+" cores per machine");
      }
      else
      {
        g_log.information("Detect: Number of Processors per machine: "+boost::lexical_cast<std::string>(cores));
        limits.computation.cores = cores;
      }
      if (xmli.exists("//limits/computation/processes"))
      {
        limits.computation.processes = xmli.get_value<size_t>("//limits/computation/processes");
      }
      if (xmli.exists("//limits/computation/threads"))
      {
        limits.computation.threads = xmli.get_value<size_t>("//limits/computation/threads");
      }
      if (xmli.exists("//limits/computation/memory"))
      {
        if (xmli.exists("//limits/computation/memory/result_buffer"))
        {
          limits.computation.memory.result_buffer = xmli.get_value<size_t>("//limits/computation/memory/result_buffer");
        }
        if (xmli.exists("//limits/computation/memory/signal_buffer"))
        {
          limits.computation.memory.signal_buffer = xmli.get_value<size_t>("//limits/computation/memory/signal_buffer");
        }
        if (xmli.exists("//limits/computation/memory/exchange_buffer"))
        {
          limits.computation.memory.exchange_buffer = xmli.get_value<size_t>("//limits/computation/memory/exchange_buffer");
        }
        if (xmli.exists("//limits/computation/memory/alignpad_buffer"))
        {
          limits.computation.memory.alignpad_buffer = xmli.get_value<size_t>("//limits/computation/memory/alignpad_buffer");
        }
        if (xmli.exists("//limits/computation/memory/scale"))
        {
          limits.computation.memory.scale = xmli.get_value<size_t>("//limits/computation/memory/scale");
        }
      }
    }
    if (xmli.exists("//limits/services"))
    {
      if (xmli.exists("//limits/services/signal"))
      {
        if (xmli.exists("//limits/services/signal/memory"))
        {
          if (xmli.exists("//limits/services/signal/memory/server"))
          {
            limits.services.signal.memory.server = xmli.get_value<size_t>("//limits/services/signal/memory/server");
          }
          if (xmli.exists("//limits/services/signal/memory/client"))
          {
            limits.services.signal.memory.client = xmli.get_value<size_t>("//limits/services/signal/memory/client");
          }
        }
        if (xmli.exists("//limits/services/signal/times"))
        {
          if (xmli.exists("//limits/services/signal/times/serverflush"))
          {
            limits.services.signal.times.serverflush = xmli.get_value<size_t>("//limits/services/signal/times/serverflush");
          }
          if (xmli.exists("//limits/services/signal/times/clientflush"))
          {
            limits.services.signal.times.clientflush = xmli.get_value<size_t>("//limits/services/signal/times/clientflush");
          }
        }
      }
    }
    if (xmli.exists("//limits/services"))
    {
      if (xmli.exists("//limits/services/monitor"))
      {
        if (xmli.exists("//limits/services/monitor/delay"))
        {
          limits.services.monitor.delay = xmli.get_value<size_t>("//limits/services/monitor/delay");
        }
        if (xmli.exists("//limits/services/monitor/sampling"))
        {
          limits.services.monitor.sampling = xmli.get_value<size_t>("//limits/services/monitor/sampling");
        }
      }
    }
    if (xmli.exists("//limits/decomposition"))
    {
      if (xmli.exists("//limits/decomposition/utilization"))
      {
        limits.decomposition.utilization = xmli.get_value<double>("//limits/decomposition/utilization");
      }
      if (xmli.exists("//limits/decomposition/partitions"))
      {
        if (xmli.exists("//limits/decomposition/partitions/automatic"))
        {
          limits.decomposition.partitions.automatic = xmli.get_value<bool>("//limits/decomposition/partitions/automatic");
        }
        if (xmli.exists("//limits/decomposition/partitions/size"))
        {
          limits.decomposition.partitions.size = xmli.get_value<size_t>("//limits/decomposition/partitions/size");
        }
      }
    }
  }
  // END OF limits section //
  // START OF debug section //
  debug.timer = false; // this adds a log message when a timer is started/stopped
  debug.barriers = false; // this de-/activates collective barriers before each collective operation, this way all nodes are synchronized before the communication takes place. This is an important step towards analysis of timing.
  debug.monitor.update = true;
  debug.iowrite.write = true;
  debug.iowrite.buffer = true;
  debug.print.orientations = false;
  if (xmli.exists("//debug"))
  {
    if (xmli.exists("//debug/timer"))
    {
      debug.timer = xmli.get_value<bool>("//debug/timer");
    }
    if (xmli.exists("//debug/barriers"))
    {
      debug.barriers = xmli.get_value<bool>("//debug/barriers");
    }
    if (xmli.exists("//debug/monitor"))
    {
      if (xmli.exists("//debug/monitor/update"))
      {
        debug.monitor.update = xmli.get_value<bool>("//debug/monitor/update");
      }
    }
    if (xmli.exists("//debug/iowrite"))
    {
      if (xmli.exists("//debug/iowrite/write"))
      {
        debug.iowrite.write = xmli.get_value<bool>("//debug/iowrite/write");
      }
      if (xmli.exists("//debug/iowrite/buffer"))
      {
        debug.iowrite.buffer = xmli.get_value<bool>("//debug/iowrite/buffer");
      }
    }
    if (xmli.exists("//debug/print"))
    {
      if (xmli.exists("//debug/print/orientations"))
      {
        debug.print.orientations = xmli.get_value<bool>("//debug/print/orientations");
      }
    }
  }
  database.type = "file";
  database.file = "db.xml";
  database.filepath = get_filepath(database.file);
  database.format = "xml";
  if (xmli.exists("//database"))
  {
    if (xmli.exists("//database/type"))
    {
      database.type = xmli.get_value<std::string>("//database/type");
    }
    if (xmli.exists("//database/file"))
    {
      database.file = xmli.get_value<std::string>("//database/file");
    }
    if (xmli.exists("//database/format"))
    {
      database.format = xmli.get_value<std::string>("//database/format");
    }
  }
} // end of Params::read_xml

boost::program_options::options_description Params::options()
{
  namespace po = boost::program_options;
  po::options_description all("Overwrite options");
  po::options_description generic("Generic options");
  generic.add_options()
    ("help", "produce this help message")
    ("config", po::value<std::string>()->default_value("scatter.xml"),  "name of the xml configuration file")
  ;
  po::options_description sample("Sample related options");
  sample.add_options()
    ("sample.structure.file",po::value<std::string>()->default_value("sample.pdb"), "Structure file name")
    ("sample.structure.format",po::value<std::string>()->default_value("pdb"), "Structure file format")
  ;
  po::options_description stager("Data staging related options");
  stager.add_options()
    ("stager.target",po::value<std::string>()->default_value("system"), "Atom selection producing the signal (must be defined)")
    ("stager.dump",po::value<bool>()->default_value(false), "Do/Don't dump the postprocessed coordinates to a file")
    ("stager.file",po::value<std::string>()->default_value("dump.dcd"), "Name of dump file")
    ("stager.format",po::value<std::string>()->default_value("dcd"), "Format of dump file")
  ;
  po::options_description scattering("Scattering related options");
  scattering.add_options()
   ("scattering.signal.file",po::value<std::string>()->default_value("signal.h5"), "name of the signal file")
  ;
  po::options_description limits("Computational limits related options");
  limits.add_options()
    ("limits.computation.threads",po::value<int>()->default_value(1), "Number of worker threads per process")
  ;
  all.add(generic);
  all.add(sample);
  all.add(stager);
  all.add(scattering);
  all.add(limits);
  return all;
} // end of Params::options

void Params::overwrite_options(boost::program_options::variables_map& vm)
{
  if (!vm["sample.structure.file"].defaulted())
  {
    std::string val = vm["sample.structure.file"].as<std::string>();
    g_log.information("OVERWRITE sample.structure.file="+val);
    Params::Inst()->sample.structure.file=val;
  }
  if (!vm["sample.structure.format"].defaulted())
  {
    std::string val = vm["sample.structure.format"].as<std::string>();
    g_log.information("OVERWRITE sample.structure.format="+val);
    Params::Inst()->sample.structure.format=val;
  }
  if (!vm["stager.target"].defaulted())
  {
    std::string val = vm["stager.target"].as<std::string>();
    g_log.information("OVERWRITE stager.target="+val);
    Params::Inst()->stager.target=val;
  }
  if (!vm["stager.dump"].defaulted())
  {
    bool val = vm["stager.dump"].as<bool>();
    g_log.information("OVERWRITE stager.dump="+boost::lexical_cast<std::string>(val));
    Params::Inst()->stager.dump=val;
  }
  if (!vm["stager.file"].defaulted())
  {
    std::string val = vm["stager.file"].as<std::string>();
    g_log.information("OVERWRITE stager.file="+val);
    Params::Inst()->stager.file=val;
  }
  if (!vm["stager.format"].defaulted())
  {
    std::string val = vm["stager.format"].as<std::string>();
    g_log.information("OVERWRITE stager.format="+val);
    Params::Inst()->stager.format=val;
  }
  if (!vm["scattering.signal.file"].defaulted())
  {
    std::string val = vm["scattering.signal.file"].as<std::string>();
    g_log.information("OVERWRITE scattering.signal.file="+val);
    Params::Inst()->scattering.signal.file=val;
  }
  if (!vm["limits.computation.threads"].defaulted())
  {
    int val = vm["limits.computation.threads"].as<int>();
    g_log.information("OVERWRITE limits.computation.threads="+boost::lexical_cast<std::string>(val));
    Params::Inst()->limits.computation.threads=val;
  }
} // end of Params::overwrite_options

void Params::init(int argc,char** argv)
{
  namespace po = boost::program_options;
  po::variables_map vm;
  po::options_description ops = options();
  po::store(po::parse_command_line(argc, argv, ops), vm);
  po::notify(vm);
  if (vm.find("help")!=vm.end())
  {
    std::stringstream ssm;
    ssm << ops;
    std::string message;
    ssm >> message;
    g_log.information(message);
    throw std::runtime_error("Terminate requested");
  }
  if (vm["config"].defaulted())
  {
    g_log.information("No configuration file specified. Will use default");
  }
  std::string filename = vm["config"].as<std::string>();
  if ( boost::filesystem::exists( boost::filesystem::path(filename) ) )
  {
    // make the directory of the main configuration file the root for all others
    if (boost::filesystem::path(filename).is_complete())
      config_rootpath = boost::filesystem::path(filename).parent_path().string();
    else
      config_rootpath = ( boost::filesystem::initial_path() / boost::filesystem::path(filename).parent_path() ).string();
   g_log.information("Looking for configuration file: " + filename);
   read_xml(filename);
   }
  else
  {
    g_log.error(vm["config"].as<std::string>()+" does not exist!");
    throw;
  }
  g_log.information("Analyzing command line for parameter overwrites");
  overwrite_options(vm);
}

/// Sets documentation strings for this algorithm
void LoadSassenaParams::initDocs()
{
  this->setWikiSummary("This algorithm loads a Sassena input XML file into an internal data structure");
  this->setOptionalMessage(" Algorithm to load an XML file into an internal data structure");
  this->setWikiDescription("This algorithm loads a Sassena input XML into an internal data structure. It will create an instance of LoadSassenaParams::Params class.");
}

/**
 * Initialise the algorithm. Declare properties which can be set before execution (input) or
 * read from after the execution (output).
 */
void LoadSassenaParams::init()
{
  this->declareProperty(new API::FileProperty("Filename", "Anonymous", API::FileProperty::Load, ".xml"),"A Sassena XML input file");
  this->declareProperty(new API::WorkspaceProperty<API::WorkspaceGroup>("OutputWorkspace","",Kernel::Direction::Output, API::PropertyMode::Optional), "Optional, group workspace to append the parameters as logs.");
}

/// Execute the algorithm
void LoadSassenaParams::exec()
{
  filename = this->getProperty("Filename");
  const std::string gwsName = this->getPropertyValue("OutputWorkspace");


} // end of LoadSassena::exec()

} // endof namespace DataHandling
} // endof namespace Mantid
