#ifndef Rules_h
#define Rules_h

#include <map>

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
  Rule *Parent; ///< Parent object (for tree)

  static int addToKey(std::vector<int> &AV, const int passN = -1);

  int
  getBaseKeys(std::vector<int> &) const; ///< Fills the vector with the surfaces

public:
  static int makeCNFcopy(Rule *&); ///< Make Rule into a CNF format (slow)
  static int makeFullDNF(Rule *&); ///< Make Rule into a full DNF format
  static int makeCNF(Rule *&);     ///< Make Rule into a CNF format
  static int removeComplementary(Rule *&); ///< NOT WORKING
  static int removeItem(Rule *&TRule, const int SurfN);

  Rule();
  Rule(Rule *);
  Rule(const Rule &);
  Rule &operator=(const Rule &);
  virtual ~Rule();
  virtual Rule *clone() const = 0; ///< abstract clone object
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

  virtual void setLeaves(Rule *, Rule *) = 0;      ///< abstract set leaves
  virtual void setLeaf(Rule *, const int = 0) = 0; ///< Abstract set
  virtual int findLeaf(const Rule *) const = 0;    ///< Abstract find
  virtual Rule *findKey(const int) = 0;            ///< Abstract key find
  virtual int type() const { return 0; }           ///< Null rule

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
  int substituteSurf(const int SurfN, const int newSurfN, Surface *SPtr);

  /// Abstract Display
  virtual std::string display() const = 0;
  /// Abstract Display Address
  virtual std::string displayAddress() const = 0;
  /// Abstract getBoundingBox
  virtual void getBoundingBox(double &xmax, double &ymax, double &zmax,
                              double &xmin, double &ymin, double &zmin) = 0;
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
  Rule *A; ///< Rule 1
  Rule *B; ///< Rule 2

public:
  Intersection();
  explicit Intersection(Rule *, Rule *);
  explicit Intersection(Rule *, Rule *, Rule *);
  Intersection *clone() const; ///< Makes a copy of the whole downward tree
  virtual std::string className() const {
    return "Intersection";
  } ///< Returns class name as string

  Intersection(const Intersection &);
  Intersection &operator=(const Intersection &);
  ~Intersection();
  Rule *leaf(const int ipt = 0) const {
    return ipt ? B : A;
  }                                           ///< selects leaf component
  void setLeaves(Rule *, Rule *);             ///< set leaves
  void setLeaf(Rule *nR, const int side = 0); ///< set one leaf.
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
  Rule *A; ///< Leaf rule A
  Rule *B; ///< Leaf rule B

public:
  Union();
  explicit Union(Rule *, Rule *);
  explicit Union(Rule *, Rule *, Rule *);
  Union(const Union &);

  Union *clone() const;
  Union &operator=(const Union &);
  ~Union();
  virtual std::string className() const {
    return "Union";
  } ///< Returns class name as string

  Rule *leaf(const int ipt = 0) const {
    return ipt ? B : A;
  }                               ///< Select a leaf component
  void setLeaves(Rule *, Rule *); ///< set leaves
  void setLeaf(Rule *nR, const int side = 0);
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
  Surface *key; ///< Actual Surface Base Object
  int keyN;     ///< Key Number (identifer)
  int sign;     ///< +/- in Object unit

public:
  SurfPoint();
  SurfPoint(const SurfPoint &);
  SurfPoint *clone() const;
  SurfPoint &operator=(const SurfPoint &);
  ~SurfPoint();
  virtual std::string className() const {
    return "SurfPoint";
  } ///< Returns class name as string

  Rule *leaf(const int = 0) const { return 0; } ///< No Leaves
  void setLeaves(Rule *, Rule *);
  void setLeaf(Rule *, const int = 0);
  int findLeaf(const Rule *) const;
  Rule *findKey(const int KeyNum);

  int type() const { return 0; } ///< Effective name

  void setKeyN(const int Ky); ///< set keyNumber
  void setKey(Surface *);
  bool isValid(const Kernel::V3D &) const;
  bool isValid(const std::map<int, int> &) const;
  int getSign() const { return sign; } ///< Get Sign
  int getKeyN() const { return keyN; } ///< Get Key
  int simplify();

  Surface *getKey() const { return key; } ///< Get Surface Ptr
  std::string display() const;
  std::string displayAddress() const;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin); /// bounding box
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

public:
  CompObj();
  CompObj(const CompObj &);
  CompObj *clone() const;
  CompObj &operator=(const CompObj &);
  ~CompObj();
  virtual std::string className() const {
    return "CompObj";
  } ///< Returns class name as string

  void setLeaves(Rule *, Rule *);
  void setLeaf(Rule *, const int = 0);
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
  Rule *A; ///< The rule

public:
  CompGrp();
  explicit CompGrp(Rule *, Rule *);
  CompGrp(const CompGrp &);
  CompGrp *clone() const;
  CompGrp &operator=(const CompGrp &);
  ~CompGrp();
  virtual std::string className() const {
    return "CompGrp";
  } ///< Returns class name as string

  Rule *leaf(const int) const { return A; } ///< selects leaf component
  void setLeaves(Rule *, Rule *);
  void setLeaf(Rule *nR, const int side = 0);
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

public:
  BoolValue();
  BoolValue(const BoolValue &);
  BoolValue *clone() const;
  BoolValue &operator=(const BoolValue &);
  ~BoolValue();
  virtual std::string className() const {
    return "BoolValue";
  } ///< Returns class name as string

  Rule *leaf(const int = 0) const { return 0; } ///< No leaves
  void setLeaves(Rule *, Rule *);
  void setLeaf(Rule *, const int = 0);
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
};

} // NAMESPACE  Geometry

} // NAMESPACE  Mantid

#endif
