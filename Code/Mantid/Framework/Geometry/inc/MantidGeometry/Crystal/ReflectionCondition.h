#ifndef MANTID_GEOMETRY_REFLECTIONCONDITION_H_
#define MANTID_GEOMETRY_REFLECTIONCONDITION_H_
    
#include "MantidGeometry/DllConfig.h"

#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif

#include <vector>
#include <string>

namespace Mantid
{
namespace Geometry
{

  /** A class containing the Reflection Condition for a crystal.
   * e.g. Face-centered, etc.
   * determining which HKL's are allows and which are not.
   * 
   * @author Janik Zikovsky
   * @date 2011-05-16 11:55:15.983855
   */
  class MANTID_GEOMETRY_DLL ReflectionCondition 
  {
  public:
    ReflectionCondition() {}
    virtual ~ReflectionCondition() {}
    /// Name of the reflection condition
    virtual std::string getName() = 0;
    /// Symbol of the associated lattice centering.
    virtual std::string getSymbol() = 0;
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int l) = 0;
  };

  //------------------------------------------------------------------------
  /** Primitive ReflectionCondition */
  class MANTID_GEOMETRY_DLL ReflectionConditionPrimitive : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "Primitive"; }
    /// Symbol of the associated lattice centering.
    virtual std::string getSymbol() { return "P"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int /*h*/, int /*k*/, int /*l*/) { return true; }
  };

  //------------------------------------------------------------------------
  /** C-face centred ReflectionCondition */
  class MANTID_GEOMETRY_DLL ReflectionConditionCFaceCentred : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "C-face centred"; }
    /// Symbol of the associated lattice centering.
    virtual std::string getSymbol() { return "C"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int /*l*/) { return (((h+k)%2)==0); }
  };

  //------------------------------------------------------------------------
  /** A-face centred ReflectionCondition */
  class MANTID_GEOMETRY_DLL ReflectionConditionAFaceCentred : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "A-face centred"; }
    /// Symbol of the associated lattice centering.
    virtual std::string getSymbol() { return "A"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int /*h*/, int k, int l) { return (((k+l)%2)==0); }
  };

  //------------------------------------------------------------------------
  /** B-face centred ReflectionCondition */
  class MANTID_GEOMETRY_DLL ReflectionConditionBFaceCentred : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "B-face centred"; }
    /// Symbol of the associated lattice centering.
    virtual std::string getSymbol() { return "B"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int /*k*/, int l) { return (((h+l)%2)==0); }
  };

  //------------------------------------------------------------------------
  /** Body centred ReflectionCondition */
  class MANTID_GEOMETRY_DLL ReflectionConditionBodyCentred : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "Body centred"; }
    /// Symbol of the associated lattice centering.
    virtual std::string getSymbol() { return "I"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int l) { return ((h+k+l)%2)==0; }
  };

  //------------------------------------------------------------------------
  /** All-face centred ReflectionCondition */
  class MANTID_GEOMETRY_DLL ReflectionConditionAllFaceCentred : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "All-face centred"; }
    /// Symbol of the associated lattice centering.
    virtual std::string getSymbol() { return "F"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int l)
    { return (((((h+k)%2)==0) && (((h+l)%2)==0) && (((k+l)%2)==0)) | ((h%2==0) && (k%2==0) && (l%2==0)) || ((h%2==1) && (k%2==1) && (l%2==1))); }
  };

  //------------------------------------------------------------------------
  /** Rhombohedrally centred, obverse ReflectionCondition*/
  class MANTID_GEOMETRY_DLL ReflectionConditionRhombohedrallyObverse : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "Rhombohedrally centred, obverse"; }
    /// Symbol of the associated lattice centering.
    virtual std::string getSymbol() { return "Robv"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int l) { return (((-h+k+l)%3)==0); }
  };

  //------------------------------------------------------------------------
  /** Rhombohedrally centred, reverse ReflectionCondition*/
  class MANTID_GEOMETRY_DLL ReflectionConditionRhombohedrallyReverse : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "Rhombohedrally centred, reverse"; }
    /// Symbol of the associated lattice centering.
    virtual std::string getSymbol() { return "Rrev"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int l) { return (((h-k+l)%3)==0); }
  };

  //------------------------------------------------------------------------
  /** Hexagonally centred, reverse ReflectionCondition*/
  class MANTID_GEOMETRY_DLL ReflectionConditionHexagonallyReverse : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "Hexagonally centred, reverse"; }
    /// Symbol of the associated lattice centering.
    virtual std::string getSymbol() { return "H"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int /*l*/) { return (((h-k)%3)==0); }
  };




  /// Shared pointer to a ReflectionCondition
  typedef boost::shared_ptr<ReflectionCondition> ReflectionCondition_sptr;

  MANTID_GEOMETRY_DLL std::vector<ReflectionCondition_sptr> getAllReflectionConditions();

} // namespace Mantid
} // namespace Geometry

#endif  /* MANTID_GEOMETRY_REFLECTIONCONDITION_H_ */
