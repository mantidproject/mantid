// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/IDFObject.h"
#include "MantidKernel/V3D.h"
#include <Poco/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <string>
#include <vector>

namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace Kernel {
class ProgressBase;
}

namespace Geometry {
class ICompAssembly;
class IComponent;
class Instrument;
class ObjComponent;
class IObject;
class ShapeFactory;

/** Creates an instrument data from a XML instrument description file

  @author Nick Draper, Tessella Support Services plc
  @date 19/11/2007
  @author Anders Markvardsen, ISIS, RAL
  @date 7/3/2008
*/
class MANTID_GEOMETRY_DLL InstrumentDefinitionParser {
public:
  InstrumentDefinitionParser();
  InstrumentDefinitionParser(const std::string &filename, const std::string &instName, const std::string &xmlText);
  InstrumentDefinitionParser(const IDFObject_const_sptr &xmlFile, const IDFObject_const_sptr &expectedCacheFile,
                             const std::string &instName, const std::string &xmlText);
  ~InstrumentDefinitionParser() = default;

  /// Caching
  enum CachingOption { NoneApplied, ReadGeomCache, ReadFallBack, WroteGeomCache, WroteCacheTemp };

  /// Parse XML contents
  std::shared_ptr<Instrument> parseXML(Kernel::ProgressBase *progressReporter);

  /// Add/overwrite any parameters specified in instrument with param values
  /// specified in <component-link> XML elements
  void setComponentLinks(std::shared_ptr<Geometry::Instrument> &instrument, Poco::XML::Element *pRootElem,
                         Kernel::ProgressBase *progress = nullptr, const std::string &requestedDate = std::string());

  std::string getMangledName();

  /// Get parent component element of location element
  static Poco::XML::Element *getParentComponent(const Poco::XML::Element *pLocElem);

  /// get name of location element
  static std::string getNameOfLocationElement(const Poco::XML::Element *pElem, const Poco::XML::Element *pCompElem);

  /// Save DOM tree to xml file
  void saveDOM_Tree(std::string &outFilename);

  /// Get the applied caching option.
  CachingOption getAppliedCachingOption() const;

  /// creates a vtp filename from a given xml filename
  const std::string createVTPFileName();

private:
  /// shared Constructor logic
  void initialise(const std::string &filename, const std::string &instName, const std::string &xmlText,
                  const std::string &vtpFilename);

  /// lazy loads the document and returns a pointer
  Poco::AutoPtr<Poco::XML::Document> getDocument();

  /// Set location (position) of comp as specified in XML location element
  void setLocation(Geometry::IComponent *comp, const Poco::XML::Element *pElem, const double angleConvertConst,
                   const bool deltaOffsets = false);
  /// Set location (position) of comp as specified in XML side-by-side-view-location element
  void setSideBySideViewLocation(Geometry::IComponent *comp, const Poco::XML::Element *pCompElem);

  /// Calculate the position of comp relative to its parent from info provided
  /// by \<location\> element
  Kernel::V3D getRelativeTranslation(const Geometry::IComponent *comp, const Poco::XML::Element *pElem,
                                     const double angleConvertConst, const bool deltaOffsets = false);

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
  void populateIdList(Poco::XML::Element *pE, IdList &idList);

  std::vector<std::string> buildExcludeList(const Poco::XML::Element *const location);

  /// Add XML element to parent assuming the element contains other component
  /// elements
  void appendAssembly(Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElem,
                      const Poco::XML::Element *pCompElem, IdList &idList);
  /// Return true if assembly, false if not assembly and throws exception if
  /// string not in assembly
  bool isAssembly(const std::string &) const;

  /// Add XML element to parent assuming the element contains no other component
  /// elements
  void appendLeaf(Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElem,
                  const Poco::XML::Element *pCompElem, IdList &idList);

  void createDetectorOrMonitor(Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElem,
                               const Poco::XML::Element *pCompElem, const std::string &filename, IdList &idList,
                               const std::string &category);

  void createGridDetector(Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElem,
                          const Poco::XML::Element *pCompElem, const std::string &filename,
                          const Poco::XML::Element *pType);

  void createRectangularDetector(Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElem,
                                 const Poco::XML::Element *pCompElem, const std::string &filename,
                                 const Poco::XML::Element *pType);

  void createStructuredDetector(Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElem,
                                const Poco::XML::Element *pCompElem, const std::string &filename,
                                const Poco::XML::Element *pType);

  /// Append \<locations\> in a locations element
  void appendLocations(Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElems,
                       const Poco::XML::Element *pCompElem, IdList &idList);

  /// Set parameter/logfile info (if any) associated with component
  void setLogfile(const Geometry::IComponent *comp, const Poco::XML::Element *pElem,
                  InstrumentParameterCache &logfileCache, const std::string &requestedDate = std::string());

  /// Parse position of facing element to V3D
  Kernel::V3D parseFacingElementToV3D(Poco::XML::Element *pElem);
  /// Set facing of comp as specified in XML facing element
  void setFacing(Geometry::IComponent *comp, const Poco::XML::Element *pElem);
  /// Make the shape defined in 1st argument face the component in the second
  /// argument
  void makeXYplaneFaceComponent(Geometry::IComponent *&in, const Geometry::ObjComponent *facing);
  /// Make the shape defined in 1st argument face the position in the second
  /// argument
  void makeXYplaneFaceComponent(Geometry::IComponent *&in, const Kernel::V3D &facingPoint);

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
  void adjust(Poco::XML::Element *pElem, std::map<std::string, bool> &isTypeAssembly,
              std::map<std::string, Poco::XML::Element *> &getTypeElement);

  /// Take as input a \<locations\> element. Such an element is a short-hand
  /// notation for a sequence of \<location\> elements.
  /// This method return this sequence as a xml string
  Poco::AutoPtr<Poco::XML::Document> convertLocationsElement(const Poco::XML::Element *pElem);

  /// return 0 if the attribute doesn't exist. This is to follow the
  /// behavior of atof which always returns 0 if there is a problem.
  double attrToDouble(const Poco::XML::Element *pElem, const std::string &name);

  /// Populate vectors of pointers to type and component xml elements
  void getTypeAndComponentPointers(const Poco::XML::Element *pRootElem, std::vector<Poco::XML::Element *> &typeElems,
                                   std::vector<Poco::XML::Element *> &compElems) const;

  /// Throw exception if type name is not unique in the IDF
  void throwIfTypeNameNotUnique(const std::string &filename, const std::string &typeName) const;

  /// Record type as an assembly if it contains a component, otherwise create a
  /// shape for it
  void createShapeIfTypeIsNotAnAssembly(Mantid::Geometry::ShapeFactory &shapeCreator, size_t iType,
                                        Poco::XML::Element *pTypeElem, const std::string &typeName);

  /// Adjust each type which contains a \<combine-components-into-one-shape\>
  /// element
  void adjustTypesContainingCombineComponentsElement(ShapeFactory &shapeCreator, const std::string &filename,
                                                     const std::vector<Poco::XML::Element *> &typeElems,
                                                     size_t numberOfTypes);

  /// Create a vector of elements which contain a \<parameter\>
  void createVectorOfElementsContainingAParameterElement(Poco::XML::Element *pRootElem);

  /// Check IdList
  void checkIdListExistsAndDefinesEnoughIDs(const IdList &idList, Poco::XML::Element *pElem,
                                            const std::string &filename) const;

  /// Check component has a \<location\> or \<locations\> element
  void checkComponentContainsLocationElement(Poco::XML::Element *pElem, const std::string &filename) const;

  /// Aggregate locations and IDs for components
  void parseLocationsForEachTopLevelComponent(Kernel::ProgressBase *progressReporter, const std::string &filename,
                                              const std::vector<Poco::XML::Element *> &compElems);

  /// Collect some information about types for later use
  void collateTypeInformation(const std::string &filename, const std::vector<Poco::XML::Element *> &typeElems,
                              ShapeFactory &shapeCreator);

public: // for testing
  /// return absolute position of point which is set relative to the
  /// coordinate system of the input component
  Kernel::V3D getAbsolutPositionInCompCoorSys(Geometry::ICompAssembly *comp, Kernel::V3D);

private:
  /// Reads from a cache file.
  void applyCache(const IDFObject_const_sptr &cacheToApply);

  /// Write out a cache file.
  CachingOption writeAndApplyCache(IDFObject_const_sptr firstChoiceCache, IDFObject_const_sptr fallBackCache);

  /// This method returns the parent appended which its child components and
  /// also name of type of the last child component
  std::string getShapeCoorSysComp(Geometry::ICompAssembly *parent, Poco::XML::Element *pLocElem,
                                  std::map<std::string, Poco::XML::Element *> &getTypeElement,
                                  Geometry::ICompAssembly *&endAssembly);

  /// Returns a translated and rotated \<cuboid\> element
  std::string translateRotateXMLcuboid(Geometry::ICompAssembly *comp, const Poco::XML::Element *cuboidEle,
                                       const std::string &cuboidName);
  /// Returns a translated and rotated \<cuboid\> element
  std::string translateRotateXMLcuboid(Geometry::ICompAssembly *comp, const std::string &cuboidXML,
                                       const std::string &cuboidName);

  /// Return a subelement of an XML element
  Poco::XML::Element *getShapeElement(const Poco::XML::Element *pElem, const std::string &name);

  /// Get position coordinates from XML element
  Kernel::V3D parsePosition(Poco::XML::Element *pElem);

  /// Input xml file
  IDFObject_const_sptr m_xmlFile;

  /// Input vtp file
  IDFObject_const_sptr m_cacheFile;

  /// Name of the instrument
  std::string m_instName;

  /// Store if xml text contains side-by-side-view-location string
  bool m_sideBySideViewLocation_exists;

  /// XML document is lazy loaded
  Poco::AutoPtr<Poco::XML::Document> m_pDoc;

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
  std::map<std::string, std::shared_ptr<Geometry::IObject>> mapTypeNameToShape;
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
  std::shared_ptr<Geometry::Instrument> m_instrument;

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
    SphVec(const double &r, const double &theta, const double &phi) : r(r), theta(theta), phi(phi) {}
    ///@endcond
  };

  /// Map to store positions of parent components in spherical coordinates
  std::map<const Geometry::IComponent *, SphVec> m_tempPosHolder;

  /// Caching applied.
  CachingOption m_cachingOption;
};

} // namespace Geometry
} // namespace Mantid
