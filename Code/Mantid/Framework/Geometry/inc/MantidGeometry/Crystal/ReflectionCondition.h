#ifndef MANTID_GEOMETRY_REFLECTIONCONDITION_H_
#define MANTID_GEOMETRY_REFLECTIONCONDITION_H_
    
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <vector>


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
  class DLLExport ReflectionCondition 
  {
  public:
    ReflectionCondition() {}
    virtual ~ReflectionCondition() {}
    /// Name of the reflection condition
    virtual std::string getName() = 0;
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int l) = 0;
  };

  //------------------------------------------------------------------------
  /** Primitive ReflectionCondition */
  class DLLExport ReflectionConditionPrimitive : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "Primitive"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int /*h*/, int /*k*/, int /*l*/) { return true; }
  };

  //------------------------------------------------------------------------
  /** C-face centred ReflectionCondition */
  class DLLExport ReflectionConditionCFaceCentred : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "C-face centred"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int /*l*/) { return (((h+k)%2)==0); }
  };

  //------------------------------------------------------------------------
  /** A-face centred ReflectionCondition */
  class DLLExport ReflectionConditionAFaceCentred : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "A-face centred"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int /*h*/, int k, int l) { return (((k+l)%2)==0); }
  };

  //------------------------------------------------------------------------
  /** B-face centred ReflectionCondition */
  class DLLExport ReflectionConditionBFaceCentred : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "B-face centred"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int /*k*/, int l) { return (((h+l)%2)==0); }
  };

  //------------------------------------------------------------------------
  /** Body centred ReflectionCondition */
  class DLLExport ReflectionConditionBodyCentred : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "Body centred"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int l) { return ((h+k+l)%2)==0; }
  };

  //------------------------------------------------------------------------
  /** All-face centred ReflectionCondition */
  class DLLExport ReflectionConditionAllFaceCentred : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "All-face centred"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int l)
    { return (((((h+k)%2)==0) && (((h+l)%2)==0) && (((k+l)%2)==0)) | ((h%2==0) && (k%2==0) && (l%2==0)) || ((h%2==1) && (k%2==1) && (l%2==1))); }
  };

  //------------------------------------------------------------------------
  /** Rhombohedrally centred, obverse ReflectionCondition*/
  class DLLExport ReflectionConditionRhombohedrallyObverse : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "Rhombohedrally centred, obverse"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int l) { return (((-h+k+l)%3)==0); }
  };

  //------------------------------------------------------------------------
  /** Rhombohedrally centred, reverse ReflectionCondition*/
  class DLLExport ReflectionConditionRhombohedrallyReverse : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "Rhombohedrally centred, reverse"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int l) { return (((h-k+l)%3)==0); }
  };

  //------------------------------------------------------------------------
  /** Hexagonally centred, reverse ReflectionCondition*/
  class DLLExport ReflectionConditionHexagonallyReverse : public ReflectionCondition
  {
  public:
    /// Name of the reflection condition
    virtual std::string getName() { return "Hexagonally centred, reverse"; }
    /// Return true if the hkl is allowed.
    virtual bool isAllowed(int h, int k, int /*l*/) { return (((h-k)%3)==0); }
  };




  /// Shared pointer to a ReflectionCondition
  typedef boost::shared_ptr<ReflectionCondition> ReflectionCondition_sptr;

  DLLExport std::vector<ReflectionCondition_sptr> getAllReflectionConditions();

} // namespace Mantid
} // namespace Geometry

#endif  /* MANTID_GEOMETRY_REFLECTIONCONDITION_H_ */
