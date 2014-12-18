#ifndef MANTID_GEOMETRY_INSTRUMENTDEFINITIONPARSER_H_
#define MANTID_GEOMETRY_INSTRUMENTDEFINITIONPARSER_H_

#include <string>
#include <vector>
#include <Poco/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/IDFObject.h"

namespace Poco {
namespace XML {
class Element;
}
}

namespace Mantid {
namespace Kernel {
class ProgressBase;
}

namespace Geometry {
class ICompAssembly;
class IComponent;
class Instrument;
class ObjComponent;
class Object;

/** Creates an instrument data from a XML instrument description file

  @author Nick Draper, Tessella Support Services plc
  @date 19/11/2007
  @author Anders Markvardsen, ISIS, RAL
  @date 7/3/2008

  Copyright &copy; 2007-2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class DLLExport InstrumentDefinitionParser {
public:
  InstrumentDefinitionParser();
  ~InstrumentDefinitionParser();

  /// Caching
  enum CachingOption {
    NoneApplied,
    ReadAdjacent,
    ReadFallBack,
    WroteCacheAdjacent,
    WroteCacheTemp
  };

  /// Set up parser
  void initialize(const std::string &filename, const std::string &instName,
                  const std::string &xmlText);

  /// Set up parser.
  void initialize(IDFObject_const_sptr xmlFile,
                  IDFObject_const_sptr expectedCacheFile,
                  const std::string &instName, const std::string &xmlText);

  /// Parse XML contents
  boost::shared_ptr<Instrument> parseXML(Kernel::ProgressBase *prog);

  /// Add/overwrite any parameters specified in instrument with param values
  /// specified in <component-link> XML elements
  void setComponentLinks(boost::shared_ptr<Geometry::Instrument> &instrument,
                         Poco::XML::Element *pElem,
                         Kernel::ProgressBase *progress = NULL);

  std::string getMangledName();

  /// Get parent component element of location element
  static Poco::XML::Element *
  getParentComponent(const Poco::XML::Element *pLocElem);

  /// get name of location element
  static std::string
  getNameOfLocationElement(const Poco::XML::Element *pElem,
                           const Poco::XML::Element *pCompElem);

  /// Save DOM tree to xml file
  void saveDOM_Tree(std::string &outFilename);

  /// Getter the the applied caching option.
  CachingOption getAppliedCachingOption() const;

private:
  /// Set location (position) of comp as specified in XML location element
  void setLocation(Geometry::IComponent *comp, const Poco::XML::Element *pElem,
                   const double angleConvertConst,
                   const bool deltaOffsets = false);

  /// Calculate the position of comp relative to its parent from info provided
  /// by \<location\> element
  Kernel::V3D getRelativeTranslation(const Geometry::IComponent *comp,
                                     const Poco::XML::Element *pElem,
                                     const double angleConvertConst,
                                     const bool deltaOffsets = false);

  /// Check the validity range and add it to the instrument object
  void setValidityRange(const Poco::XML::Element *pRootElem);

  /// Reads the contents of the \<defaults\> element to set member variables,
  void readDefaults(Poco::XML::Element *defaults);

  /// Structure for holding detector IDs
  struct IdList {
    /// Used to count the number of detector encounted so far
    int counted;
    /// list of detector IDs
    std::vector<int> vec;
    /// name of idlist
    std::string idname;

    /// Constructor
    IdList() : counted(0){};

    /// return true if empty
    bool empty() { return vec.empty(); };

    /// reset idlist
    void reset() {
      counted = 0;
      vec.clear();
    };
  };

  /// Method for populating IdList
  void populateIdList(Poco::XML::Element *pElem, IdList &idList);

  std::vector<std::string>
  buildExcludeList(const Poco::XML::Element *const location);

  /// Add XML element to parent assuming the element contains other component
  /// elements
  void appendAssembly(Geometry::ICompAssembly *parent,
                      const Poco::XML::Element *pLocElem,
                      const Poco::XML::Element *pCompElem, IdList &idList);
  /// Return true if assembly, false if not assembly and throws exception if
  /// string not in assembly
  bool isAssembly(std::string) const;

  /// Add XML element to parent assuming the element contains no other component
  /// elements
  void appendLeaf(Geometry::ICompAssembly *parent,
                  const Poco::XML::Element *pLocElem,
                  const Poco::XML::Element *pCompElem, IdList &idList);

  /// Append \<locations\> in a locations element
  void appendLocations(Geometry::ICompAssembly *parent,
                       const Poco::XML::Element *pLocElems,
                       const Poco::XML::Element *pCompElem, IdList &idList);

  /// Set parameter/logfile info (if any) associated with component
  void setLogfile(const Geometry::IComponent *comp,
                  const Poco::XML::Element *pElem,
                  InstrumentParameterCache &logfileCache);

  /// Parse position of facing element to V3D
  Kernel::V3D parseFacingElementToV3D(Poco::XML::Element *pElem);
  /// Set facing of comp as specified in XML facing element
  void setFacing(Geometry::IComponent *comp, const Poco::XML::Element *pElem);
  /// Make the shape defined in 1st argument face the component in the second
  /// argument
  void makeXYplaneFaceComponent(Geometry::IComponent *&in,
                                const Geometry::ObjComponent *facing);
  /// Make the shape defined in 1st argument face the position in the second
  /// argument
  void makeXYplaneFaceComponent(Geometry::IComponent *&in,
                                const Kernel::V3D &facingPoint);

  /// Reads in or creates the geometry cache ('vtp') file
  CachingOption setupGeometryCache();

  /// If appropriate, creates a second instrument containing neutronic detector
  /// positions
  void createNeutronicInstrument();

  /// Takes as input a \<type\> element containing a
  /// <combine-components-into-one-shape>, and
  /// adjust the \<type\> element by replacing its containing \<component\>
  /// elements with \<cuboid\>'s
  /// (note for now this will only work for \<cuboid\>'s and when necessary this
  /// can be extended).
  void adjust(Poco::XML::Element *pElem,
              std::map<std::string, bool> &isTypeAssembly,
              std::map<std::string, Poco::XML::Element *> &getTypeElement);

  /// Take as input a \<locations\> element. Such an element is a short-hand
  /// notation for a sequence of \<location\> elements.
  /// This method return this sequence as a xml string
  std::string convertLocationsElement(const Poco::XML::Element *pElem);

public: // for testing
  /// return absolute position of point which is set relative to the
  /// coordinate system of the input component
  Kernel::V3D getAbsolutPositionInCompCoorSys(Geometry::ICompAssembly *comp,
                                              Kernel::V3D);

private:
  /// Checks if the proposed cache file can be read from.
  bool canUseProposedCacheFile(IDFObject_const_sptr cache) const;

  /// Reads from a cache file.
  void applyCache(IDFObject_const_sptr cacheToApply);

  /// Write out a cache file.
  CachingOption writeAndApplyCache(IDFObject_const_sptr usedCache);

  /// This method returns the parent appended which its child components and
  /// also name of type of the last child component
  std::string getShapeCoorSysComp(
      Geometry::ICompAssembly *parent, Poco::XML::Element *pLocElem,
      std::map<std::string, Poco::XML::Element *> &getTypeElement,
      Geometry::ICompAssembly *&endAssembly);

  /// Returns a translated and rotated \<cuboid\> element
  std::string translateRotateXMLcuboid(Geometry::ICompAssembly *comp,
                                       const Poco::XML::Element *cuboidEle,
                                       const std::string &cuboidName);
  /// Returns a translated and rotated \<cuboid\> element
  std::string translateRotateXMLcuboid(Geometry::ICompAssembly *comp,
                                       const std::string &cuboidXML,
                                       const std::string &cuboidName);

  /// Return a subelement of an XML element
  Poco::XML::Element *getShapeElement(const Poco::XML::Element *pElem,
                                      const std::string &name);

  /// Get position coordinates from XML element
  Kernel::V3D parsePosition(Poco::XML::Element *pElem);

  /// Input xml file
  IDFObject_const_sptr m_xmlFile;

  /// Input vtp file
  IDFObject_const_sptr m_cacheFile;

  /// Name of the instrument
  std::string m_instName;

  /// XML document loaded
  Poco::AutoPtr<Poco::XML::Document> pDoc;
  /// Root element of the parsed XML
  Poco::XML::Element *pRootElem;

  /** Holds all the xml elements that have a \<parameter\> child element.
   *  Added purely for the purpose of computing speed and is used in
   * setLogFile() for the purpose
   *  of quickly accessing if a component have a parameter/logfile associated
   * with it or not
   *  - instead of using the comparatively slow poco call getElementsByTagName()
   * (or getChildElement)
   */
  std::vector<Poco::XML::Element *> m_hasParameterElement;
  /// has m_hasParameterElement been set - used when public method
  /// setComponentLinks is used
  bool m_hasParameterElement_beenSet;
  /** map which holds names of types and whether or not they are categorized as
   * being
   *  assemblies, which means whether the type element contains component
   * elements
   */
  std::map<std::string, bool> isTypeAssembly;
  /// map which maps the type name to a shared pointer to a geometric shape
  std::map<std::string, boost::shared_ptr<Geometry::Object>> mapTypeNameToShape;
  /// Container to hold all detectors and monitors added to the instrument. Used
  /// for 'facing' these to component specified under \<defaults\>. NOTE: Seems
  /// unused, ever.
  std::vector<Geometry::ObjComponent *> m_facingComponent;
  /// True if defaults->components-are-facing is set in instrument def. file
  bool m_haveDefaultFacing;
  /// Hold default facing position
  Kernel::V3D m_defaultFacing;
  /// map which holds names of types and pointers to these type for fast
  /// retrieval in code
  std::map<std::string, Poco::XML::Element *> getTypeElement;

  /// For convenience added pointer to instrument here
  boost::shared_ptr<Geometry::Instrument> m_instrument;

  /// Flag to indicate whether offsets given in spherical coordinates are to be
  /// added to the current
  /// position (true) or are a vector from the current position (false, default)
  bool m_deltaOffsets;

  /// when this const equals 1 it means that angle=degree (default) is set in
  /// IDF
  /// otherwise if this const equals 180/pi it means that angle=radian is set in
  /// IDF
  double m_angleConvertConst;

  bool m_indirectPositions; ///< Flag to indicate whether IDF contains physical
                            ///& neutronic positions
  /// A map containing the neutronic position for each detector. Used when
  /// m_indirectPositions is true.
  std::map<Geometry::IComponent *, Poco::XML::Element *> m_neutronicPos;

  /** Stripped down vector that holds position in terms of spherical
   * coordinates,
    *  Needed when processing instrument definition files that use the 'Ariel
   * format'
    */
  struct SphVec {
    ///@cond Exclude from doxygen documentation
    double r, theta, phi;
    SphVec() : r(0.0), theta(0.0), phi(0.0) {}
    SphVec(const double &r, const double &theta, const double &phi)
        : r(r), theta(theta), phi(phi) {}
    ///@endcond
  };

  /// Map to store positions of parent components in spherical coordinates
  std::map<const Geometry::IComponent *, SphVec> m_tempPosHolder;

  /// Caching applied.
  CachingOption m_cachingOption;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_INSTRUMENTDEFINITIONPARSER_H_ */
