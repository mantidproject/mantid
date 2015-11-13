#ifndef Rules_h
#define Rules_h

#include <map>
#include <boost/shared_ptr.hpp>

class TopoDS_Shape;

namespace Mantid {

namespace Geometry {

class Object;
class Surface;

/**
\class Rule
\brief Object generation rule tree
\author S.Ansell
\version 1.0
\date April 2005

Base class for a rule item in the tree.

Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MANTID_GEOMETRY_DLL Rule {
private:
  Rule *Parent;                      ///< Parent object (for tree)
  virtual Rule *doClone() const = 0; ///< abstract clone object

  static int addToKey(std::vector<int> &AV, const int passN = -1);

  int getBaseKeys(
      std::vector<int> &) const; ///< Fills the vector with the surfaces
protected:
  Rule(const Rule &);
  Rule &operator=(const Rule &);

public:
  static int
  makeCNFcopy(std::unique_ptr<Rule> &); ///< Make Rule into a CNF format (slow)
  static int
  makeFullDNF(std::unique_ptr<Rule> &); ///< Make Rule into a full DNF format
  static int makeCNF(std::unique_ptr<Rule> &); ///< Make Rule into a CNF format
  static int removeComplementary(std::unique_ptr<Rule> &); ///< NOT WORKING
  static int removeItem(std::unique_ptr<Rule> &TRule, const int SurfN);

  Rule();
  Rule(Rule *);
  std::unique_ptr<Rule> clone() const {
    return std::unique_ptr<Rule>(doClone());
  }
  virtual ~Rule();
  virtual std::string className() const {
    return "Rule";
  } ///< Returns class name as string

  /// No leaf for a base rule
  virtual Rule *leaf(const int = 0) const { return 0; }
  void setParent(Rule *);
  Rule *getParent() const;
  void makeParents();
  int checkParents() const; ///< Debug test for parents
  int getKeyList(std::vector<int> &) const;
  int commonType() const; ///< Gets a common type

  virtual void setLeaves(std::unique_ptr<Rule>,
                         std::unique_ptr<Rule>) = 0; ///< abstract set leaves
  virtual void setLeaf(std::unique_ptr<Rule>,
                       const int = 0) = 0;      ///< Abstract set
  virtual int findLeaf(const Rule *) const = 0; ///< Abstract find
  virtual Rule *findKey(const int) = 0;         ///< Abstract key find
  virtual int type() const { return 0; }        ///< Null rule

  /// Abstract: The point is within the object
  virtual bool isValid(const Kernel::V3D &) const = 0;
  /// Abstract Validity based on surface true/false map
  virtual bool isValid(const std::map<int, int> &) const = 0;
  /// Abstract: Can the rule be simplified
  virtual int simplify() = 0;

  virtual int isComplementary() const {
    return 0;
  } ///< Always returns false (0)

  int Eliminate(); ///< elimination not written
  int substituteSurf(const int SurfN, const int newSurfN,
                     const boost::shared_ptr<Surface> &SPtr);

  /// Abstract Display
  virtual std::string display() const = 0;
  /// Abstract Display Address
  virtual std::string displayAddress() const = 0;
  /// Abstract getBoundingBox
  virtual void getBoundingBox(double &xmax, double &ymax, double &zmax,
                              double &xmin, double &ymin, double &zmin) = 0;
#ifdef ENABLE_OPENCASCADE
  virtual TopoDS_Shape analyze() = 0;
#endif
};

/**
\class Intersection
\brief Combines two Rule objects in an intersection
\author S. Ansell
\version 1.0
\date August 2005

Intersection is defined as a valid Rule A and
valid rule B
*/

class MANTID_GEOMETRY_DLL Intersection : public Rule {

private:
  std::unique_ptr<Rule> A;       ///< Rule 1
  std::unique_ptr<Rule> B;       ///< Rule 2
  Intersection *doClone() const; ///< Makes a copy of the whole downward tree
protected:
  Intersection(const Intersection &);
  Intersection &operator=(const Intersection &);

public:
  Intersection();
  explicit Intersection(std::unique_ptr<Rule>, std::unique_ptr<Rule>);
  explicit Intersection(Rule *, std::unique_ptr<Rule>, std::unique_ptr<Rule>);
  std::unique_ptr<Intersection>
  clone() const; ///< Makes a copy of the whole downward tree
  virtual std::string className() const {
    return "Intersection";
  } ///< Returns class name as string

  Rule *leaf(const int ipt = 0) const {
    return ipt ? B.get() : A.get();
  } ///< selects leaf component
  void setLeaves(std::unique_ptr<Rule>, std::unique_ptr<Rule>); ///< set leaves
  void setLeaf(std::unique_ptr<Rule> nR, const int side = 0); ///< set one leaf.
  int findLeaf(const Rule *) const;
  Rule *findKey(const int KeyN);
  int isComplementary() const;

  int type() const { return 1; } // effective name
  std::string display() const;
  std::string displayAddress() const;

  bool isValid(const Kernel::V3D &) const;
  bool isValid(const std::map<int, int> &) const;
  int simplify(); ///< apply general intersection simplification
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin); /// bounding box
#ifdef ENABLE_OPENCASCADE
  virtual TopoDS_Shape analyze();
#endif
};

/**
\class Union
\brief Combines two Rule objects in an union
\author S. Ansell
\version 1.0
\date August 2005

Union is defined as a valid Rule A or
valid rule B
*/

class MANTID_GEOMETRY_DLL Union : public Rule {

private:
  std::unique_ptr<Rule> A; ///< Leaf rule A
  std::unique_ptr<Rule> B; ///< Leaf rule B
  Union *doClone() const;

protected:
  Union(const Union &);
  Union &operator=(const Union &);

public:
  Union();
  explicit Union(std::unique_ptr<Rule>, std::unique_ptr<Rule>);
  explicit Union(Rule *, std::unique_ptr<Rule>, std::unique_ptr<Rule>);

  std::unique_ptr<Union> clone() const;
  virtual std::string className() const {
    return "Union";
  } ///< Returns class name as string

  Rule *leaf(const int ipt = 0) const {
    return ipt ? B.get() : A.get();
  } ///< Select a leaf component
  void setLeaves(std::unique_ptr<Rule>, std::unique_ptr<Rule>); ///< set leaves
  void setLeaf(std::unique_ptr<Rule>, const int side = 0);
  int findLeaf(const Rule *) const;
  Rule *findKey(const int KeyN);

  int isComplementary() const;
  int type() const { return -1; } ///< effective name

  bool isValid(const Kernel::V3D &) const;
  bool isValid(const std::map<int, int> &) const;
  std::string display() const;
  std::string displayAddress() const;
  int simplify(); ///< apply general intersection simplification
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin); /// bounding box
#ifdef ENABLE_OPENCASCADE
  virtual TopoDS_Shape analyze();
#endif
};

/**
\class SurfPoint
\brief Surface leaf node
\author S.Ansell
\version 1.0
\date August 2005

This class acts as an interface between a general
surface and a rule. Allowing an Rule chain to
be calculated
*/

class MANTID_GEOMETRY_DLL SurfPoint : public Rule {
private:
  boost::shared_ptr<Surface> m_key; ///< Actual Surface Base Object
  SurfPoint *doClone() const;
  int keyN; ///< Key Number (identifer)
  int sign; ///< +/- in Object unit
public:
  SurfPoint();
  virtual std::string className() const {
    return "SurfPoint";
  } ///< Returns class name as string
  std::unique_ptr<SurfPoint> clone() const;

  Rule *leaf(const int = 0) const { return 0; } ///< No Leaves
  void setLeaves(std::unique_ptr<Rule>, std::unique_ptr<Rule>);
  void setLeaf(std::unique_ptr<Rule>, const int = 0);
  int findLeaf(const Rule *) const;
  Rule *findKey(const int KeyNum);

  int type() const { return 0; } ///< Effective name

  void setKeyN(const int Ky); ///< set keyNumber
  void setKey(const boost::shared_ptr<Surface> &key);
  bool isValid(const Kernel::V3D &) const;
  bool isValid(const std::map<int, int> &) const;
  int getSign() const { return sign; } ///< Get Sign
  int getKeyN() const { return keyN; } ///< Get Key
  int simplify();

  Surface *getKey() const { return m_key.get(); } ///< Get Surface Ptr
  std::string display() const;
  std::string displayAddress() const;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin); /// bounding box
#ifdef ENABLE_OPENCASCADE
  virtual TopoDS_Shape analyze();
#endif
};

/**
\class CompObj
\brief Compemented Object
\author S.Ansell
\version 1.0
\date January 2006

This class holds a complement object
of a single object group.
Care must be taken to avoid a cyclic loop
*/

class MANTID_GEOMETRY_DLL CompObj : public Rule {
private:
  int objN;    ///< Object number
  Object *key; ///< Object Pointer
  CompObj *doClone() const;

protected:
  CompObj(const CompObj &);
  CompObj &operator=(const CompObj &);

public:
  CompObj();
  std::unique_ptr<CompObj> clone() const;
  virtual std::string className() const {
    return "CompObj";
  } ///< Returns class name as string

  void setLeaves(std::unique_ptr<Rule>, std::unique_ptr<Rule>);
  void setLeaf(std::unique_ptr<Rule>, const int = 0);
  int findLeaf(const Rule *) const;
  Rule *findKey(const int i);

  int type() const { return 0; }            ///< Is it a branched object
  int isComplementary() const { return 1; } ///< Always returns true (1)

  void setObjN(const int Ky); ///< set object Number
  void setObj(Object *);      ///< Set a Object state
  bool isValid(const Kernel::V3D &) const;
  bool isValid(const std::map<int, int> &) const;
  /// Get object number of component
  int getObjN() const { return objN; }
  int simplify();

  Object *getObj() const { return key; } ///< Get Object Ptr
  std::string display() const;
  std::string displayAddress() const;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin); /// bounding box
#ifdef ENABLE_OPENCASCADE
  virtual TopoDS_Shape analyze();
#endif
};

/**
\class CompGrp
\brief Compemented Grup
\author S.Ansell
\version 1.0
\date January 2006

This class holds a complement object
of a single object group.
Care must be taken to avoid a cyclic loop
*/

class MANTID_GEOMETRY_DLL CompGrp : public Rule {
private:
  std::unique_ptr<Rule> A; ///< The rule
  CompGrp *doClone() const;

protected:
  CompGrp(const CompGrp &);
  CompGrp &operator=(const CompGrp &);

public:
  CompGrp();
  explicit CompGrp(Rule *, std::unique_ptr<Rule>);
  std::unique_ptr<CompGrp> clone() const;
  virtual std::string className() const {
    return "CompGrp";
  } ///< Returns class name as string

  Rule *leaf(const int) const { return A.get(); } ///< selects leaf component
  void setLeaves(std::unique_ptr<Rule>, std::unique_ptr<Rule>);
  void setLeaf(std::unique_ptr<Rule> nR, const int side = 0);
  int findLeaf(const Rule *) const;
  Rule *findKey(const int i);

  int type() const { return 0; }            ///< Is it a branched object
  int isComplementary() const { return 1; } ///< Always returns true (1)

  bool isValid(const Kernel::V3D &) const;
  bool isValid(const std::map<int, int> &) const;
  int simplify();

  std::string display() const;
  std::string displayAddress() const;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin); /// bounding box
#ifdef ENABLE_OPENCASCADE
  virtual TopoDS_Shape analyze();
#endif
};

/**
\class BoolValue
\brief Rule Status class
\author S.Ansell
\version 1.0
\date April 2005

Class to handle a rule with a truth value
but can be true/false/unknown.
*/

class MANTID_GEOMETRY_DLL BoolValue : public Rule {
private:
  int status; ///< Three values 0 False : 1 True : -1 doesn't matter
  BoolValue *doClone() const;

protected:
  BoolValue(const BoolValue &);
  BoolValue &operator=(const BoolValue &);

public:
  BoolValue();
  std::unique_ptr<BoolValue> clone() const;
  virtual std::string className() const {
    return "BoolValue";
  } ///< Returns class name as string

  Rule *leaf(const int = 0) const { return 0; } ///< No leaves
  void setLeaves(std::unique_ptr<Rule>, std::unique_ptr<Rule>);
  void setLeaf(std::unique_ptr<Rule>, const int = 0);
  int findLeaf(const Rule *) const;
  Rule *findKey(const int) { return 0; }

  int type() const { return 0; } // effective name

  ///< write val into status, if in valid range
  void setStatus(int val) {
    if (val == 0 || val == 1 || val == -1)
      status = val;
  }
  bool isValid(const Kernel::V3D &) const;
  bool isValid(const std::map<int, int> &)
      const; ///< isValue :: Based on a surface status map
  int simplify();

  std::string display() const;
  std::string displayAddress() const;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin); /// bounding box
#ifdef ENABLE_OPENCASCADE
  TopoDS_Shape analyze();
#endif
};

} // NAMESPACE  Geometry

} // NAMESPACE  Mantid

#endif
