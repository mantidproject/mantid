#ifndef ObjComp_h
#define ObjComp_h

///\cond WORKING

namespace Geometry
{

class ObjComp
{ 
 private:
 
  std::string Name;              ///< Name of object 
  int virutalization;            ///< Reality level 0 : real, >=1 : virtual/logical
  int level;                     ///< Priority to calculation.
  
 protected:

  Vec3D Centre;                  ///< Centre position 
  Quat Orientation;              ///< Orientation / Rotation 

 public:
  ///Returns if the point is within the object component
  virtual int isValid(const Vec3D&) const
  {
    Vec3D PtTrans(Pt-Centre);
  }
};


class ObjCompVector : public ObjComp
{
  
};

class ObjCompGraph : public ObjComp
{
  
};

template<Type2Int<1> >
class ObjCompItem : public ObjComp
{
 protected:

  ObjComp* Comp;  ///< Pointer to the object component

};

class ObjObjItem : public ObjComp
{
 private:

  Object* Comp;  ///< Pointer to the object component

 public:
  ///constructor
  ObjComp();
  ///Destructor
  ~ObjComp();

  
  int isValid(const Geometry::Vec3D&) const;    ///< Check if a point is valid

  
};

}  // NAMESPACE 

///\endcond WORKING

#endif

