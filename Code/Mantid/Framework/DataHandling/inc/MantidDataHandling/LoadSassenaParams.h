#ifndef MANTID_DATAHANDLING_LOADSASSENAPARAMS_H_
#define MANTID_DATAHANDLING_LOADSASSENAPARAMS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup.h"

// special library headers
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

namespace Mantid
{

namespace DataHandling
{

  /** Load Sassena Output files.

  Required Properties:
  <UL>
  <LI> Filename - The name of and path to the Sassena input XML file </LI>
  </UL>

  Optional properties:
  <UL>
  <LI> Workspace - The name of the group workspace to append the parameters as logs
  </UL>

  @author Benjamin Lindner & Jose Borreguero
  @date 2012-07-15

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  */


class CartesianCoor3D;
class CylinderCoor3D ;
class SphericalCoor3D;

typedef std::pair< CartesianCoor3D,CartesianCoor3D> cartrect;

//helper funcions:
inline float sign(float a,float b) { return (b<0.0) ? -a : a; }

/**
 Models a XML node and provides convenience functions to access its properties.
*/
class XMLElement
{
public:
  /// constructor
  XMLElement(xmlNodePtr ptr);
  /// initialize m_node_ptr
  void set(xmlNodePtr ptr) { node_ptr = ptr; }
  xmlElementType type() const { return node_ptr->type; }
  const std::string name() const { if (node_ptr->name==NULL) return ""; else return (const char*) node_ptr->name; }
  const std::string content() const { if (node_ptr->content==NULL) return ""; else return (const char*)  node_ptr->content; }
  void print(const std::string prepend, bool showchildren);
  xmlNodePtr get_node_ptr() const { return node_ptr; }
  xmlNodePtr node_ptr;
  std::vector<XMLElement> children;
};

/**
 Models a XML file and allows access through XPATH.
*/
class XMLInterface
{
public:
  XMLInterface(const std::string filename);
  ~XMLInterface();
  std::vector<XMLElement> get(const char* xpathexp);
  std::vector<XMLElement> get(std::string xpathexp);
  void dump(std::vector<char>& c);
  template<class convT> convT get_value(const char* xpathexp);
  bool exists(const char* xpathexp);
  void set_current(XMLElement thisel){ p_xpathCtx->node = thisel.get_node_ptr();}
  xmlDocPtr p_doc;
  xmlXPathContextPtr p_xpathCtx;
protected:
  size_t maxrecursion;
private:
  static Kernel::Logger& g_log; /// Reference to the logger class
};


///Type class which represents coordinates in cartesian space. Allows transformation into other coordinate representations and implements some basic linear algebra.
class CartesianCoor3D
{
public:
  CartesianCoor3D() : x(0), y(0), z(0) {}
  CartesianCoor3D(double v1,double v2,double v3) : x(v1), y(v2), z(v3) {}
  // conversion constructors:
  CartesianCoor3D(const CylinderCoor3D cc);
  CartesianCoor3D(const SphericalCoor3D cc);
  ~CartesianCoor3D() {}
  friend std::ostream& operator<<(std::ostream& os, const CartesianCoor3D& cc);
  CartesianCoor3D& operator=(const CartesianCoor3D& that);
  CartesianCoor3D operator-(const CartesianCoor3D& that);
  CartesianCoor3D operator+(const CartesianCoor3D& that);
  CartesianCoor3D cross_product(const CartesianCoor3D& that);
  double operator*(const CartesianCoor3D& that);
  bool operator<(const CartesianCoor3D& that) const; // for use in maps only

  double x,y,z;
  double length();
};
CartesianCoor3D operator*(const double lambda, const CartesianCoor3D& that);
CartesianCoor3D operator*(const CartesianCoor3D& that,const double lambda);
CartesianCoor3D operator/(const CartesianCoor3D& that,const double lambda);
CartesianCoor3D rotate(CartesianCoor3D,std::string axis,double rad);

/**
 * Type class which represents a vector base (3 orthonormal vectors) for cartesian coordinates.
 * Can be constructed out of thin air or from partial vectors.
*/
class CartesianVectorBase
{
public:
  CartesianVectorBase() {}
  CartesianVectorBase(CartesianCoor3D axis);
  std::vector<CartesianCoor3D> get_base() {return base_;}
  CartesianCoor3D& operator[](size_t index);
  CartesianCoor3D project(CartesianCoor3D vec);

  std::vector<CartesianCoor3D> base_;
};

/**
 * Type class which represents coordinates in cylinder space.
 * Allows transformation into other coordinate representations and implements some basic linear algebra.
 * Cylinder coords have a range:
 * r >= 0
 * 0 <= phi < 2 M_PI
*/
class CylinderCoor3D
{
public:
  CylinderCoor3D() : r(0), phi(0), z(0) {}
  CylinderCoor3D(double v1,double v2,double v3);
  // conversion constructors:
  CylinderCoor3D(const CartesianCoor3D cc);
  CylinderCoor3D(const SphericalCoor3D cc);
  ~CylinderCoor3D() {}
  friend std::ostream& operator<<(std::ostream& os, const CylinderCoor3D& cc);
  CylinderCoor3D& operator=(const CylinderCoor3D& that);
  CylinderCoor3D operator-(const CylinderCoor3D& that);

  double r,phi,z;
};

/**
 * Type class which represents coordinates in spherical space.
 * Allows transformation into other coordinate representations and implements some basic linear algebra.
 * Spherical coords have a range:
 * r >= 0
 * 0 <= phi < 2 M_PI
 * 0 <= theta < M_PI
*/
class SphericalCoor3D
{
public:
  SphericalCoor3D() : r(0), phi(0), theta(0) {}
  SphericalCoor3D(double v1,double v2,double v3) : r(v1), phi(v2), theta(v3) {}
  // conversion constructors:
  SphericalCoor3D(const CartesianCoor3D cc);
  SphericalCoor3D(const CylinderCoor3D cc);
  ~SphericalCoor3D() {}
  friend std::ostream& operator<<(std::ostream& os, const SphericalCoor3D& cc);
  SphericalCoor3D& operator=(const SphericalCoor3D& that);
  SphericalCoor3D operator-(const SphericalCoor3D& that);

  double r,phi,theta;
};

/// Section which defines the structure
class SampleStructureParameters
{
public:
  std::string file;
  std::string filepath; // runtime
  std::string format;
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<file>" << file << "</file>" << std::endl;
    ss << std::string(pad,' ') << "<format>" << format << "</format>" << std::endl;
    return ss.str();
  }
};

/// Section which defines a generic selection (used as parent by specific selections)
class SampleSelectionParameters
{
public:
  SampleSelectionParameters() {}
  SampleSelectionParameters(std::string type) : type_(type) {}
  std::string type() { return type_;}
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type_ << "</type>" << std::endl;
    return ss.str();
  }
    std::string type_;
};

/// Section which defines a selection based on individual indexes
class SampleIndexSelectionParameters : public SampleSelectionParameters
{
public:
  SampleIndexSelectionParameters() {}
  SampleIndexSelectionParameters(std::vector<size_t> ids) : SampleSelectionParameters("index"), ids_(ids) {}
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << SampleSelectionParameters::write_xml(pad);
    for(size_t i = 0; i < ids_.size(); ++i)
    {
      ss << std::string(pad,' ') << "<index>" << ids_[i] << "</index>" << std::endl;
    }
    return ss.str();
  }
  std::vector<size_t> ids_;
};

/// Section which defines a selection based on a given range
class SampleRangeSelectionParameters : public SampleSelectionParameters
{
public:
  SampleRangeSelectionParameters() {}
  SampleRangeSelectionParameters(size_t from, size_t to) : SampleSelectionParameters("range"), from_(from), to_(to) {}
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << SampleSelectionParameters::write_xml(pad);
    ss << std::string(pad,' ') << "<from>" << from_ << "</from>" << std::endl;
    ss << std::string(pad,' ') << "<to>" << to_ << "</to>" << std::endl;
    return ss.str();
  }
  size_t from_;
  size_t to_;
};

/// Section which defines a selection based on a lexical pattern (regular expression matching atom labels)
class SampleLexicalSelectionParameters : public SampleSelectionParameters
{
public:
  SampleLexicalSelectionParameters() {}
  SampleLexicalSelectionParameters(std::string expression) : SampleSelectionParameters("lexical"), expression_(expression) {}
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << SampleSelectionParameters::write_xml(pad);
    ss << std::string(pad,' ') << "<expression>" << expression_ << "</expression>" << std::endl;
    return ss.str();
  }
  std::string expression_;
};

/// Section which defines a file based selection
class SampleFileSelectionParameters : public SampleSelectionParameters
{
public:
  SampleFileSelectionParameters() {}
  SampleFileSelectionParameters(std::string file, std::string format, std::string selector, std::string expression) : SampleSelectionParameters("file"), file_(file), format_(format), selector_(selector), expression_(expression) {}
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << SampleSelectionParameters::write_xml(pad);
    ss << std::string(pad,' ') << "<file>" << file_ << "</file>" << std::endl;
    ss << std::string(pad,' ') << "<format>" << format_ << "</format>" << std::endl;
    ss << std::string(pad,' ') << "<selector>" << selector_ << "</selector>" << std::endl;
    ss << std::string(pad,' ') << "<expression>" << expression_ << "</expression>" << std::endl;
    return ss.str();
  }
  std::string file_;
  std::string filepath_; // runtime
  std::string format_;
  std::string selector_;
  std::string expression_;
};

/// Section which defines a single trajectory file entry
class SampleFramesetParameters
{
public:
  size_t first;
  size_t last;
  size_t clones;
  bool last_set;
  size_t stride;
  std::string file;
  std::string filepath; // runtime
  std::string format;
  std::string index;
  std::string indexpath;
  bool index_default;
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<format>" << format << "</format>" << std::endl;
    ss << std::string(pad,' ') << "<file>" << file << "</file>" << std::endl;
    // don't write location specific filepath or indexpath! not well defined behavior
    if (!index_default)
    {
      ss << std::string(pad,' ') << "<index>" << index << "</index>" << std::endl;
    }
    ss << std::string(pad,' ') << "<first>" << first << "</first>" << std::endl;
    if (last_set)
    {
      ss << std::string(pad,' ') << "<last>" << last << "</last>" << std::endl;
    }
    ss << std::string(pad,' ') << "<clones>" << clones << "</clones>" << std::endl;
    ss << std::string(pad,' ') << "<stride>" << stride << "</stride>" << std::endl;
    return ss.str();
  }
};

/// Section which lists the used trajectory files
class SampleFramesetsParameters : public std::vector<SampleFramesetParameters >
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    for(size_t i = 0; i < this->size(); ++i)
    {
      ss << std::string(pad,' ') << "<frameset>" << std::endl;
      ss << this->at(i).write_xml(pad+1);
      ss << std::string(pad,' ') << "</frameset>" << std::endl;
    }
    return ss.str();
  }
};

/// Section which stores reference information which may be required during some alignment procedures
class SampleMotionReferenceParameters
{
public:
  std::string type;
  std::string selection;
  std::string file;
  std::string filepath; // runtime
  std::string format;
  size_t frame;
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>" << std::endl;
    ss << std::string(pad,' ') << "<selection>" << selection << "</selection>" << std::endl;
    ss << std::string(pad,' ') << "<file>" << file << "</file>" << std::endl;
    ss << std::string(pad,' ') << "<format>" << format << "</format>" << std::endl;
    ss << std::string(pad,' ') << "<frame>" << frame << "</frame>" << std::endl;
return ss.str();
  }
};

/// Section which defines artificial motions
class SampleMotionParameters {
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>" << std::endl;
    ss << std::string(pad,' ') << "<displace>" << displace << "</displace>" << std::endl;
    ss << std::string(pad,' ') << "<frequency>" << frequency << "</frequency>" << std::endl;
    ss << std::string(pad,' ') << "<radius>" << radius << "</radius>" << std::endl;
    ss << std::string(pad,' ') << "<selection>" << selection << "</selection>" << std::endl;
    ss << std::string(pad,' ') << "<seed>" << seed << "</seed>" << std::endl;
    ss << std::string(pad,' ') << "<sampling>" << sampling << "</sampling>" << std::endl;
    ss << std::string(pad,' ') << "<referemce>" << std::endl;
    ss << reference.write_xml(pad+1);
    ss << std::string(pad,' ') << "</referemce>" << std::endl;
    return ss.str();
  }
  std::string type;
  double displace;
  double frequency;
  double radius;
  std::string selection;
  unsigned long seed;
  long sampling;
  CartesianCoor3D direction;
  SampleMotionReferenceParameters reference;
};

/**
 * Section which stores reference information which may be required during some alignment procedures
*/
class SampleAlignmentReferenceParameters
{
private:
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>" << std::endl;
    ss << std::string(pad,' ') << "<selection>" << selection << "</selection>" << std::endl;
    ss << std::string(pad,' ') << "<file>" << file << "</file>" << std::endl;
    ss << std::string(pad,' ') << "<format>" << format << "</format>" << std::endl;
    ss << std::string(pad,' ') << "<frame>" << frame << "</frame>" << std::endl;
    return ss.str();
  }
  std::string type;
  std::string selection;
  std::string file;
  std::string filepath; // runtime
  std::string format;
  size_t frame;
};

/// Section which stores alignment information, which is applied during the staging of the trajectory data
class SampleAlignmentParameters
{
private:
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>" << std::endl;
    ss << std::string(pad,' ') << "<selection>" << selection << "</selection>" << std::endl;
    ss << std::string(pad,' ') << "<order>" << order << "</order>" << std::endl;
    ss << std::string(pad,' ') << "<referemce>" << std::endl;
    ss << reference.write_xml(pad+1);
    ss << std::string(pad,' ') << "</referemce>" << std::endl;
    return ss.str();
  }
  SampleAlignmentReferenceParameters reference;
  std::string type;
  std::string selection;
  std::string order;
};

/// Section which stores sample specific parameters
class SampleParameters
{
public:
  ~SampleParameters();
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<structure>" << std::endl;
    ss << structure.write_xml(pad+1);
    ss << std::string(pad,' ') << "</structure>" << std::endl;
    ss << std::string(pad,' ') << "<framesets>" << std::endl;
    ss << framesets.write_xml(pad+1);
    ss << std::string(pad,' ') << "</framesets>" << std::endl;
    ss << std::string(pad,' ') << "<selections>" << std::endl;
    for(std::map<std::string,SampleSelectionParameters*>::iterator i = selections.begin(); i != selections.end(); ++i)
    {
      ss << std::string(pad+1,' ') << "<selection>" << std::endl;
      ss << std::string(pad+2,' ') << "<name>" << i->first << "</name>" << std::endl;
      ss << i->second->write_xml(pad+2);
      ss << std::string(pad+1,' ') << "</selection>" << std::endl;
    }
    ss << std::string(pad,' ') << "</selections>" << std::endl;
    ss << std::string(pad,' ') << "<motions>" << std::endl;
    for(size_t i = 0; i < motions.size(); ++i)
    {
      ss << std::string(pad+1,' ') << "<motion>" << std::endl;
      ss << motions[i].write_xml(pad+2);
      ss << std::string(pad+1,' ') << "</motion>" << std::endl;
    }
    ss << std::string(pad,' ') << "</motions>" << std::endl;
    ss << std::string(pad,' ') << "<alignments>" << std::endl;
    for(size_t i = 0; i < alignments.size(); ++i)
    {
      ss << std::string(pad+1,' ') << "<alignment>" << std::endl;
      ss << alignments[i].write_xml(pad+2);
      ss << std::string(pad+1,' ') << "</alignment>" << std::endl;
    }
    ss << std::string(pad,' ') << "</alignments>" << std::endl;
    return ss.str();
  }
  SampleStructureParameters structure;
  std::map<std::string,SampleSelectionParameters*> selections;
  SampleFramesetsParameters framesets;
  std::vector<SampleMotionParameters> motions;
  std::vector<SampleAlignmentParameters> alignments;
};

/// Section which stores selection based scaling factors for background correction
class ScatteringBackgroundKappaParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<selection>" << selection << "</selection>" << std::endl;
    ss << std::string(pad,' ') << "<value>" << value << "</value>" << std::endl;
    return ss.str();
  }
  std::string selection;
  double value;
};

/// Section which stores background correction parameters
class ScatteringBackgroundParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>" << std::endl;
    ss << std::string(pad,' ') << "<factor>" << factor << "</factor>" << std::endl;
    ss << std::string(pad,' ') << "<kappas>" << std::endl;
    for(size_t i = 0; i < kappas.size(); ++i)
    {
      ss << std::string(pad+1,' ') << "<kappa>" << std::endl;
      ss << kappas[i].write_xml(pad+2);
      ss << std::string(pad+1,' ') << "</kappa>" << std::endl;
    }
    ss << std::string(pad,' ') << "</kappas>" << std::endl;
    return ss.str();
  }
  std::string type;
  double factor;
  std::vector<ScatteringBackgroundKappaParameters> kappas;
};

/// Section which is used when vector based orientational averaging is performed
class ScatteringAverageOrientationVectorsParameters : public std::vector<CartesianCoor3D>
{
private:
  static Kernel::Logger& g_log; /// Reference to the logger class
public:
  void create();
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>" << std::endl;
    ss << std::string(pad,' ') << "<algorithm>" << algorithm << "</algorithm>" << std::endl;
    ss << std::string(pad,' ') << "<resolution>" << resolution << "</resolution>" << std::endl;
    ss << std::string(pad,' ') << "<file>" << file << "</file>" << std::endl;
    ss << std::string(pad,' ') << "<seed>" << seed << "</seed>" << std::endl;
    return ss.str();
  }
  std::string type;
  std::string algorithm;
  std::string file;
  std::string filepath; // runtime
  size_t resolution;
  unsigned long seed;
};

/// Section which is used to store the used multipole identifiers when multipole based orientational averaging is performed
class ScatteringAverageOrientationMultipoleMomentsParameters : public std::vector<std::pair<long,long> >
{
private:
  static Kernel::Logger& g_log; /// Reference to the logger class
public:
  std::string type;
  long resolution;
  std::string file;
  std::string filepath; // runtime

  void create();
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>" << std::endl;
    ss << std::string(pad,' ') << "<resolution>" << resolution << "</resolution>" << std::endl;
    ss << std::string(pad,' ') << "<file>" << file << "</file>" << std::endl;
    return ss.str();
  }
};

/// Section which is used when multipole based orientational averaging is performed
class ScatteringAverageOrientationMultipoleParameters
{
public:
  std::string type;
  ScatteringAverageOrientationMultipoleMomentsParameters moments;
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>" << std::endl;
    ss << moments.write_xml(pad+1);
    return ss.str();
  }
};

/// Section which is used when exact orientational averaging is performed
class ScatteringAverageOrientationExactParameters
{
public:
  std::string type;
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>" << std::endl;
    return ss.str();
  }
};

/// Section which defines orientational averaging procedures
class ScatteringAverageOrientationParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>" << std::endl;
    ss << std::string(pad,' ') << "<axis>" << std::endl;
    ss << std::string(pad+1,' ') << "<x>" << axis.x << "</x>" << std::endl;
    ss << std::string(pad+1,' ') << "<y>" << axis.y << "</y>" << std::endl;
    ss << std::string(pad+1,' ') << "<z>" << axis.z << "</z>" << std::endl;
    ss << std::string(pad,' ') << "</axis>" << std::endl;
    ss << std::string(pad,' ') << "<vectors>" << std::endl;
    ss << vectors.write_xml(pad+1);
    ss << std::string(pad,' ') << "</vectors>" << std::endl;
    ss << std::string(pad,' ') << "<multipole>" << std::endl;
    ss << multipole.write_xml(pad+1);
    ss << std::string(pad,' ') << "</multipole>" << std::endl;
    ss << std::string(pad,' ') << "<exact>" << std::endl;
    ss << exact.write_xml(pad+1);
    ss << std::string(pad,' ') << "</exact>" << std::endl;
    return ss.str();
  }
  std::string type;
  CartesianCoor3D axis;
  ScatteringAverageOrientationVectorsParameters vectors;
  ScatteringAverageOrientationMultipoleParameters multipole;
  ScatteringAverageOrientationExactParameters exact;
};

/// Section which defines averaging procedures
class ScatteringAverageParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<orientation>" << std::endl;
    ss << orientation.write_xml(pad+1);
    ss << std::string(pad,' ') << "</orientation>" << std::endl;
    return ss.str();
  }
  ScatteringAverageOrientationParameters orientation;
};

/// Section which defines further processing of the scattering signal, e.g. autocorrelation
class ScatteringDspParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>" << std::endl;
    ss << std::string(pad,' ') << "<method>" << method << "</method>" << std::endl;
    return ss.str();
  }
  std::string type;
  std::string method;
};

/// Section which describes a scan entry used to construct the scattering vectors
class ScatteringVectorsScanParameters
{
public:
  ScatteringVectorsScanParameters() : points(100), exponent(1.0), basevector(CartesianCoor3D(1,0,0)) {}
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<from>" << from << "</from>" << std::endl;
    ss << std::string(pad,' ') << "<to>" << to << "</to>" << std::endl;
    ss << std::string(pad,' ') << "<points>" << points << "</points>" << std::endl;
    ss << std::string(pad,' ') << "<exponent>" << exponent << "</exponent>" << std::endl;
    ss << std::string(pad,' ') << "<base>" << std::endl;
    ss << std::string(pad,' ') << "<x>" << basevector.x << "</x>" << std::endl;
    ss << std::string(pad,' ') << "<y>" << basevector.y << "</y>" << std::endl;
    ss << std::string(pad,' ') << "<z>" << basevector.z << "</z>" << std::endl;
    ss << std::string(pad,' ') << "</base>" << std::endl;
    return ss.str();
  }
  double from;
  double to;
  size_t points;
  double exponent;
  CartesianCoor3D basevector;
};

/// Section which defines the scattering vectors
class ScatteringVectorsParameters : public std::vector<CartesianCoor3D>
{
private:
  static Kernel::Logger& g_log; /// Reference to the logger class
public:
  void create_from_scans();
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<scans>" << std::endl;
    for(size_t i = 0; i < scans.size(); ++i)
    {
      ss << std::string(pad+1,' ') << "<scan>" << std::endl;
      ss << scans[i].write_xml(pad+2);
      ss << std::string(pad+1,' ') << "</scan>" << std::endl;
    }
    ss << std::string(pad,' ') << "</scans>" << std::endl;
    return ss.str();
  }
  std::vector<ScatteringVectorsScanParameters> scans;
};

/// Section which stores parameters used during the writing of the signal file
class ScatteringSignalParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<file>" << file << "</file>"<< std::endl;
    ss << std::string(pad,' ') << "<fqt>" << fqt << "</fqt>"<< std::endl;
    ss << std::string(pad,' ') << "<fq>" << fq << "</fq>"<< std::endl;
    ss << std::string(pad,' ') << "<fq0>" << fq0 << "</fq0>"<< std::endl;
    ss << std::string(pad,' ') << "<fq2>" << fq2 << "</fq2>"<< std::endl;
    return ss.str();
  }
  std::string file;
  std::string filepath; // runtime
  bool fqt;
  bool fq;
  bool fq0;
  bool fq2;
};

/// Section which stores parameters used during the scattering calculation
class ScatteringParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>"<< std::endl;
    ss << std::string(pad,' ') << "<dsp>" << std::endl;
    ss << dsp.write_xml(pad+1);
    ss << std::string(pad,' ') << "</dsp>" << std::endl;
    ss << std::string(pad,' ') << "<vectors>" << std::endl;
    ss << qvectors.write_xml(pad+1);
    ss << std::string(pad,' ') << "</vectors>" << std::endl;
    ss << std::string(pad,' ') << "<average>" << std::endl;
    ss << average.write_xml(pad+1);
    ss << std::string(pad,' ') << "</average>" << std::endl;
    ss << std::string(pad,' ') << "<background>" << std::endl;
    ss << background.write_xml(pad+1);
    ss << std::string(pad,' ') << "</background>" << std::endl;
    ss << std::string(pad,' ') << "<signal>" << std::endl;
    ss << signal.write_xml(pad+1);
    ss << std::string(pad,' ') << "</signal>" << std::endl;
    return ss.str();
  }
  std::string type;
  ScatteringDspParameters dsp;
  ScatteringVectorsParameters qvectors;
  ScatteringAverageParameters average;
  ScatteringBackgroundParameters background;
  ScatteringSignalParameters signal;
};

/// Section which stores parameters used during data staging
class StagerParameters
{
public:
  bool dump;
  std::string file;
  std::string filepath; // runtime
  std::string format;
  std::string target;
  std::string mode;
  std::string write_xml(int pad=0)
  {
  std::stringstream ss;
    ss << std::string(pad,' ') << "<dump>" << dump << "</dump>"<< std::endl;
    ss << std::string(pad,' ') << "<file>" << file << "</file>"<< std::endl;
    ss << std::string(pad,' ') << "<format>" << format << "</format>"<< std::endl;
    ss << std::string(pad,' ') << "<target>" << target << "</target>"<< std::endl;
    ss << std::string(pad,' ') << "<mode>" << mode << "</mode>"<< std::endl;
    return ss.str();
  }
};

/// Section which stores parameters affecting the memory limitations of the file writer service
class LimitsServicesSignalMemoryParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<server>" << server << "</server>"<< std::endl;
    ss << std::string(pad,' ') << "<client>" << client << "</client>"<< std::endl;
    return ss.str();
  }
  size_t server;
  size_t client;
};

/// Section which stores parameters affecting the timing of the file writer service
class LimitsServicesSignalTimesParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<serverflush>" << serverflush << "</serverflush>"<< std::endl;
    ss << std::string(pad,' ') << "<clientflush>" << clientflush << "</clientflush>"<< std::endl;
    return ss.str();
  }
  size_t serverflush;
  size_t clientflush;
};

/// Section which stores parameters affecting the file writer service, which writes results to the signal file
class LimitsServicesSignalParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<memory>" << std::endl;
    ss << memory.write_xml(pad+1);
    ss << std::string(pad,' ') << "</memory>" << std::endl;
    ss << std::string(pad,' ') << "<times>" << std::endl;
    ss << times.write_xml(pad+1);
    ss << std::string(pad,' ') << "</times>" << std::endl;
    return ss.str();
  }
  LimitsServicesSignalMemoryParameters memory;
  LimitsServicesSignalTimesParameters times;
};

/// Section which stores parameters affecting the monitoring service, which reports progress to the console
class LimitsServicesMonitorParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<delay>" << delay << "</delay>"<< std::endl;
    ss << std::string(pad,' ') << "<sampling>" << sampling << "</sampling>"<< std::endl;
    return ss.str();
  }
  size_t delay;
  size_t sampling;
};

/// Section which stores parameters affecting the services
class LimitsServicesParameters
{
public:
  std::string write_xml(int pad=0)
  {
   	std::stringstream ss;
    ss << std::string(pad,' ') << "<signal>" << std::endl;
    ss << signal.write_xml(pad+1);
    ss << std::string(pad,' ') << "</signal>" << std::endl;
    ss << std::string(pad,' ') << "<monitor>" << std::endl;
    ss << monitor.write_xml(pad+1);
    ss << std::string(pad,' ') << "</monitor>" << std::endl;
    return ss.str();
  }
  LimitsServicesSignalParameters signal;
  LimitsServicesMonitorParameters monitor;
};

/// Section which stores memory limits during the computation
class LimitsComputationMemoryParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<result_buffer>" << result_buffer << "</result_buffer>"<< std::endl;
    ss << std::string(pad,' ') << "<alignpad_buffer>" << alignpad_buffer << "</alignpad_buffer>"<< std::endl;
    ss << std::string(pad,' ') << "<exchange_buffer>" << exchange_buffer << "</exchange_buffer>"<< std::endl;
    ss << std::string(pad,' ') << "<signal_buffer>" << signal_buffer << "</signal_buffer>"<< std::endl;
    ss << std::string(pad,' ') << "<scale>" << scale << "</scale>"<< std::endl;
    return ss.str();
  }
  size_t result_buffer;
  size_t alignpad_buffer;
  size_t exchange_buffer;
  size_t signal_buffer;
  size_t scale;
};

/// Section which stores parameters used during the computation
class LimitsComputationParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<threads>" << threads << "</threads>"<< std::endl;
    ss << std::string(pad,' ') << "<processes>" << processes << "</processes>"<< std::endl;
    ss << std::string(pad,' ') << "<cores>" << cores << "</cores>"<< std::endl;
    ss << std::string(pad,' ') << "<memory>" << std::endl;
    ss << memory.write_xml(pad+1);
    ss << std::string(pad,' ') << "</memory>" << std::endl;
    return ss.str();
  }
  size_t threads;
  size_t processes;
  size_t cores;
  LimitsComputationMemoryParameters memory;
};

/// Section which stores parameters determining the computational partition size
class LimitsDecompositionPartitionsParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<automatic>" << automatic << "</automatic>"<< std::endl;
    ss << std::string(pad,' ') << "<size>" << size << "</size>"<< std::endl;
    return ss.str();
  }
  bool automatic;
  size_t size;
};

/// Section which stores parameters used for partitioning the computation among the available compute nodes
class LimitsDecompositionParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<utilization>" << utilization << "</utilization>"<< std::endl;
    ss << std::string(pad,' ') << "<partitions>" << std::endl;
    ss << partitions.write_xml(pad+1);
    ss << std::string(pad,' ') << "</partitions>" << std::endl;
    return ss.str();
  }
  double utilization;
  LimitsDecompositionPartitionsParameters partitions;
};

/// Section which stores parameters used during the writing of the signal output file
class LimitsSignalParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<chunksize>" << chunksize << "</chunksize>"<< std::endl;
    return ss.str();
  }
  size_t chunksize;
};

/// Section which stores memory limits during the data staging process.
class LimitsStageMemoryParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<data>" << data << "</data>"<< std::endl;
    ss << std::string(pad,' ') << "<buffer>" << buffer << "</buffer>"<< std::endl;
    return ss.str();
  }
  size_t data;
  size_t buffer;
};

/// Section which stores limits regarding the data staging process.
class LimitsStageParameters{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<memory>" << std::endl;
    ss << memory.write_xml(pad+1);
    ss << std::string(pad,' ') << "</memory>" << std::endl;
    return ss.str();
  }
  LimitsStageMemoryParameters memory;
};

/// Section which stores computational limitations and performance characteristics.
class LimitsParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<stage>" << std::endl;
    ss << stage.write_xml(pad+1);
    ss << std::string(pad,' ') << "</stage>" << std::endl;
    ss << std::string(pad,' ') << "<signal>" << std::endl;
    ss << signal.write_xml(pad+1);
    ss << std::string(pad,' ') << "</signal>" << std::endl;
    ss << std::string(pad,' ') << "<services>" << std::endl;
    ss << services.write_xml(pad+1);
    ss << std::string(pad,' ') << "</services>" << std::endl;
    ss << std::string(pad,' ') << "<computation>" << std::endl;
    ss << computation.write_xml(pad+1);
    ss << std::string(pad,' ') << "</computation>" << std::endl;
    ss << std::string(pad,' ') << "<decomposition>" << std::endl;
    ss << decomposition.write_xml(pad+1);
    ss << std::string(pad,' ') << "</decomposition>" << std::endl;
    return ss.str();
  }
  LimitsStageParameters stage;
  LimitsSignalParameters signal;
  LimitsServicesParameters services;
  LimitsComputationParameters computation;
  LimitsDecompositionParameters decomposition;
};

/// Section which stores parameters influencing the progress monitoring
class DebugMonitorParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<update>" << update << "</update>"<< std::endl;
    return ss.str();
  }
  bool update;
};

///Section which stores switches for dumping information to console output.
class DebugPrintParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<orientations>" << orientations << "</orientations>"<< std::endl;
    return ss.str();
  }
  bool orientations;
};

/// Section which stores IO write specific debug parameters. Used to tune the frequency by which the results are written to the signal file. Can also be used to avoid writing to the signal file.
class DebugIowriteParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<write>" << write << "</write>"<< std::endl;
    ss << std::string(pad,' ') << "<buffer>" << buffer << "</buffer>"<< std::endl;
    return ss.str();
  }
  bool write;
  bool buffer;
};

/// Section which stores debug parameters
class DebugParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<timer>" << timer << "</timer>"<< std::endl;
    ss << std::string(pad,' ') << "<barriers>" << barriers << "</barriers>"<< std::endl;
    ss << std::string(pad,' ') << "<iowrite>" << std::endl;
    ss << iowrite.write_xml(pad+1);
    ss << std::string(pad,' ') << "</iowrite>" << std::endl;
    ss << std::string(pad,' ') << "<print>" << std::endl;
    ss << print.write_xml(pad+1);
    ss << std::string(pad,' ') << "</print>" << std::endl;
    ss << std::string(pad,' ') << "<monitor>" << std::endl;
    ss << monitor.write_xml(pad+1);
    ss << std::string(pad,' ') << "</monitor>" << std::endl;
    return ss.str();
  }
  bool timer;
  bool barriers;
  DebugIowriteParameters iowrite;
  DebugPrintParameters print;
  DebugMonitorParameters monitor;
};

/// Section which stores a reference to the used database
class DatabaseParameters
{
public:
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<type>" << type << "</type>"<< std::endl;
    ss << std::string(pad,' ') << "<file>" << file << "</file>"<< std::endl;
    ss << std::string(pad,' ') << "<format>" << format << "</format>"<< std::endl;
    return ss.str();
  }
  std::string type;
  std::string file;
  std::string filepath;
  std::string format;
};

/**
 * This is a wrapper class to interface the settings implementation. The rational is to
 * move all possible configuration errors towards the initialization of the software
 * preferably the 'parameters' class checks for all required settings and implementents
 * default values.
 * Also, use hardwired constant names to move possible errors to compile time.
 * Basically this class maps the structure of the configuration file, more or less
 *
 * these constructs are to be used w/ in the code the following way:
 * \verbatim
 * string fs = Params::Inst()->sample.structure.file
*/

// Implement Parameters as a singleton class, this makes it globally available
// it requires the call of the init() function, which also implements all checks

class Params
{
private:
  Params() {}
  Params(const Params&);
  Params& operator=(const Params&);
  std::string get_filepath(std::string filename);
  void read_xml(std::string filename);
  boost::program_options::options_description options();
  void overwrite_options(boost::program_options::variables_map& vm);
  std::vector<char> rawconfig; // contains raw copy of the input configuration
  std::vector<char> config; // contains complete text copy of the input configuration
  std::string config_rootpath;
  static Kernel::Logger& g_log; /// Reference to the logger class
public:
  // interface for initialization and interfacing
  static Params* Inst() { static Params instance; return &instance;}
  void get_rawconfig(std::vector<char>& rc) { rc = rawconfig; }
  void get_config(std::vector<char>& c) { c=config; }
  void init(int argc,char** argv);
  ~Params() {}; // it is said some compilers have problems w/ private destructors.
  void write_xml_to_file(std::string filename);
  std::string write_xml(int pad=0)
  {
    std::stringstream ss;
    ss << std::string(pad,' ') << "<root>" << std::endl;
    ss << std::string(pad+1,' ') << "<sample>" << std::endl;
    ss << sample.write_xml(pad+2);
    ss << std::string(pad+1,' ') << "</sample>" << std::endl;
    ss << std::string(pad+1,' ') << "<scattering>" << std::endl;
    ss << scattering.write_xml(pad+2);
    ss << std::string(pad+1,' ') << "</scattering>" << std::endl;
    ss << std::string(pad+1,' ') << "<stager>" << std::endl;
    ss << stager.write_xml(pad+2);
    ss << std::string(pad+1,' ') << "</stager>" << std::endl;
    ss << std::string(pad+1,' ') << "<database>" << std::endl;
    ss << database.write_xml(pad+2);
    ss << std::string(pad+1,' ') << "</database>" << std::endl;
    ss << std::string(pad+1,' ') << "<limits>" << std::endl;
    ss << limits.write_xml(pad+2);
    ss << std::string(pad+1,' ') << "</limits>" << std::endl;
    ss << std::string(pad+1,' ') << "<debug>" << std::endl;
   	ss << debug.write_xml(pad+2);
    ss << std::string(pad+1,' ') << "</debug>" << std::endl;
    ss << std::string(pad,' ') << "</root>" << std::endl;
    return ss.str();
  }
  // interface for parameters
  SampleParameters sample;
  ScatteringParameters scattering;
  StagerParameters stager;
  DatabaseParameters database;
  LimitsParameters limits;
  DebugParameters debug;
};

class DLLExport LoadSassenaParams : public API::Algorithm
{
public:
  /// Constructor
  LoadSassenaParams();
  /// Virtual Destructor
  ~LoadSassenaParams() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LoadSassenaParams"; }
  /// Algorithm's version
  virtual int version() const { return 1; }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling\\Sassena"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs(); // Sets documentation strings for this algorithm
  /// Initialization code
  void init();             // Overwrites Algorithm method.
  /// Execution code
  void exec();             // Overwrites Algorithm method
  std::string m_filename;
  Params* m_parameters;
}; // class LoadSassena

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADSASSENAPARAMS_H_*/
