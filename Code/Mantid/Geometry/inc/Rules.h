#ifndef Rules_h
#define Rules_h

namespace MonteCarlo
{

class Object;
class Surface;

/*!
  \class Rule
  \brief Object generation rule tree
  \author S.Ansell
  \version 1.0
  \date April 2005
 
  Base class for a rule item in the tree. 
*/

class Rule
{
 private:

  Rule* Parent;                    ///< Parent object (for tree)

  static int addToKey(std::vector<int>&,const int =-1);     

  int getBaseKeys(std::vector<int>&) const;  ///< Fills the vector with the surfaces 

 public:

  static int makeCNFcopy(Rule*&);  ///< Make Rule into a CNF format (slow)
  static int makeFullDNF(Rule*&);  ///< Make Rule into a full DNF format
  static int makeCNF(Rule*&);      ///< Make Rule into a CNF format
  static int removeComplementary(Rule*&);       ///< NOT WORKING
  static int removeItem(Rule*&,const int);

  Rule();
  Rule(Rule*);
  Rule(const Rule&);  
  Rule& operator=(const Rule&);
  virtual ~Rule();
  virtual Rule* clone() const=0;  ///< abstract clone object


  /// No leaf for a base rule
  virtual Rule* leaf(const int =0) const { return 0; }  
  void setParent(Rule*);
  Rule* getParent() const;
  void makeParents();
  int checkParents() const;  ///< Debug test for parents
  int getKeyList(std::vector<int>&) const;
  int commonType() const;      ///< Gets a common type

  virtual void setLeaves(Rule*,Rule*)=0;          ///< abstract set leaves
  virtual void setLeaf(Rule*,const int =0) =0;    ///< Abstract set 
  virtual int findLeaf(const Rule*) const =0;     ///< Abstract find
  virtual Rule* findKey(const int)  =0;           ///< Abstract key find    
  virtual int type() const { return 0; }          ///< Null rule    
  

  /// Abstract: The point is within the object
  virtual int isValid(const Geometry::Vec3D&) const =0;           
  /// Abstract Validity based on surface true/false map
  virtual int isValid(const std::map<int,int>&) const =0; 
  /// Abstract: Can the rule be simplified 
  virtual int simplify() =0;

  virtual int isComplementary() const { return 0; }

  int Eliminate();            ///< elimination not written
  int substituteSurf(const int,const int,Surface*);  

  /// Abstract Display
  virtual std::string display() const=0;
  /// Abstract Display Address
  virtual std::string displayAddress() const=0;
  
};

  /*!
    \class Intersection
    \brief Combines two Rule objects in an intersection
    \author S. Ansell
    \version 1.0
    \date August 2005

    Intersection is defined as a valid Rule A and
    valid rule B 
  */

class Intersection : public Rule
{

 private:
   
  Rule* A;    ///< Rule 1 
  Rule* B;    ///< Rule 2 
  
 public:

  Intersection();
  explicit Intersection(Rule*,Rule*);
  explicit Intersection(Rule*,Rule*,Rule*);
  Intersection* clone() const;    ///< Makes a copy of the whole downward tree

  Intersection(const Intersection&);
  Intersection& operator=(const Intersection&);
  ~Intersection();
  Rule* leaf(const int ipt=0) const { return ipt ? B : A; }   ///< selects leaf component
  void setLeaves(Rule*,Rule*);           ///< set leaves
  void setLeaf(Rule*,const int =0);    ///< set one leaf.
  int findLeaf(const Rule*) const;
  Rule* findKey(const int);
  int isComplementary() const;

  int type() const { return 1; }   //effective name
  std::string display() const;
  std::string displayAddress() const;

  int isValid(const Geometry::Vec3D&) const;
  int isValid(const std::map<int,int>&) const;    
  int simplify();      ///< apply general intersection simplification

};


/*!
  \class Union
  \brief Combines two Rule objects in an union
  \author S. Ansell
  \version 1.0
  \date August 2005
  
  Union is defined as a valid Rule A or
  valid rule B 
*/

class Union : public Rule
{

 private:
  
  Rule* A;    ///< Leaf rule A
  Rule* B;    ///< Leaf rule B
  
 public:

  Union();
  explicit Union(Rule*,Rule*);
  explicit Union(Rule*,Rule*,Rule*);
  Union(const Union&);

  Union* clone() const;          
  Union& operator=(const Union&);
  ~Union();

  Rule* leaf(const int ipt=0) const { return ipt ? B : A; }      ///< Select a leaf component
  void setLeaves(Rule*,Rule*);           ///< set leaves
  void setLeaf(Rule*,const int =0);     
  int findLeaf(const Rule*) const;
  Rule* findKey(const int);

  int isComplementary() const;
  int type() const { return -1; }   ///< effective name


  int isValid(const Geometry::Vec3D&) const;
  int isValid(const std::map<int,int>&) const;    
  std::string display() const;
  std::string displayAddress() const;
  int simplify();      ///< apply general intersection simplification

};

/*!
  \class SurfPoint
  \brief Surface leaf node
  \author S.Ansell
  \version 1.0 
  \date August 2005

  This class acts as an interface between a general
  surface and a rule. Allowing an Rule chain to
  be calculated
*/

class SurfPoint : public Rule
{
 private:

  Surface* key;               ///< Actual Surface Base Object
  int keyN;                   ///< Key Number (identifer)
  int sign;                   ///< +/- in Object unit
  
 public:
  
  SurfPoint();
  SurfPoint(const SurfPoint&);
  SurfPoint* clone() const;                        
  SurfPoint& operator=(const SurfPoint&);
  ~SurfPoint();

  Rule* leaf(const int =0) const { return 0; }   ///< No Leaves
  void setLeaves(Rule*,Rule*);
  void setLeaf(Rule*,const int =0);
  int findLeaf(const Rule*) const;
  Rule* findKey(const int);

  int type() const { return 0; }   ///< Effective name

  void setKeyN(const int);             ///< set keyNumber
  void setKey(Surface*);
  int isValid(const Geometry::Vec3D&) const;
  int isValid(const std::map<int,int>&) const;    
  int getSign() const { return sign; }         ///< Get Sign
  int getKeyN() const { return keyN; }         ///< Get Key
  int simplify();

  Surface* getKey() const { return key; }     ///< Get Surface Ptr
  std::string display() const;
  std::string displayAddress() const;  

};

/*!
  \class CompObj
  \brief Compemented Object
  \author S.Ansell
  \version 1.0 
  \date January 2006

  This class holds a complement object 
  of a single object group.
  Care must be taken to avoid a cyclic loop
*/

class CompObj : public Rule
{
 private:

  int objN;                   ///< Object number
  Object* key;                ///< Object Pointer
  
 public:
  
  CompObj();
  CompObj(const CompObj&);
  CompObj* clone() const;                        
  CompObj& operator=(const CompObj&);
  ~CompObj();

  void setLeaves(Rule*,Rule*);
  void setLeaf(Rule*,const int =0);
  int findLeaf(const Rule*) const;
  Rule* findKey(const int);

  int type() const { return 0; }   ///< Is it a branched object
  int isComplementary() const { return 1; }

  void setObjN(const int);             ///< set object Number
  void setObj(Object*);               ///< Set a Object state
  int isValid(const Geometry::Vec3D&) const;
  int isValid(const std::map<int,int>&) const;    
  /// Get object number of component
  int getObjN() const { return objN; } 
  int simplify();

  Object* getObj() const { return key; }   ///< Get Object Ptr
  std::string display() const;      
  std::string displayAddress() const;  

};

/*!
  \class CompGrp
  \brief Compemented Grup 
  \author S.Ansell
  \version 1.0 
  \date January 2006

  This class holds a complement object 
  of a single object group.
  Care must be taken to avoid a cyclic loop
*/

class CompGrp : public Rule
{
 private:

  Rule* A;
  
 public:
  
  CompGrp();
  explicit CompGrp(Rule*,Rule*);  
  CompGrp(const CompGrp&);
  CompGrp* clone() const;                        
  CompGrp& operator=(const CompGrp&);
  ~CompGrp();

  Rule* leaf(const int) const { return A; }   ///< selects leaf component
  void setLeaves(Rule*,Rule*);
  void setLeaf(Rule*,const int =0);
  int findLeaf(const Rule*) const;
  Rule* findKey(const int);

  int type() const { return 0; }   ///< Is it a branched object
  int isComplementary() const { return 1; }

  int isValid(const Geometry::Vec3D&) const;
  int isValid(const std::map<int,int>&) const;    
  int simplify();

  std::string display() const;      
  std::string displayAddress() const;  

};

/*!
  \class BoolValue 
  \brief Rule Status class
  \author S.Ansell
  \version 1.0
  \date April 2005
 
  Class to handle a rule with a truth value
  but can be true/false/unknown.
*/

class BoolValue : public Rule
{
 private:

  int status;          ///< Three values 0 False : 1 True : -1 doesn't matter
  
 public:
  
  BoolValue();
  BoolValue(const BoolValue&);
  BoolValue* clone() const;                        
  BoolValue& operator=(const BoolValue&);
  ~BoolValue();

  Rule* leaf(const int =0) const { return 0; } ///< No leaves
  void setLeaves(Rule*,Rule*);
  void setLeaf(Rule*,const int =0);
  int findLeaf(const Rule*) const;
  Rule* findKey(const int) { return 0; }

  int type() const { return 0; }   //effective name

  int isValid(const Geometry::Vec3D&) const;
  int isValid(const std::map<int,int>&) const;  ///< isValue :: Based on a surface status map
  int simplify();                             


  std::string display() const;
  std::string displayAddress() const;  

};

}  // NAMESPACE 

#endif
