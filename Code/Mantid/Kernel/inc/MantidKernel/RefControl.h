#ifndef RefControl_h
#define RefControl_h

namespace Mantid
{

/*!
  \class RefControl
  \brief Impliments a reference counted data template 
  \version 1.0
  \date February 2006
  \author S.Ansell
  
  This version works only on data that is created via new().
  It is thread safe and works in the Standard template 
  libraries (but appropiate functionals are needed for 
  sorting etc.).

  The underlying data can be accessed via the normal pointer
  semantics but call the access function if the data is required
  to be modified.
*/

template<typename DataType>
class RefControl
{
 public:

  typedef boost::shared_ptr<DataType> ptr_type;   ///< typedef for the storage
  typedef DataType value_type;                    ///< typedef for the data type

 private:

  ptr_type Data;                                  ///< Real object Ptr

 public:

  RefControl();
  RefControl(const RefControl<DataType>&);      
  RefControl<DataType>& operator=(const RefControl<DataType>&);
  RefControl<DataType>& operator=(const ptr_type&);
  ~RefControl();

  const DataType& operator*() const { return *Data; }  ///< Pointer dereference access
  const DataType* operator->() const { return Data.get(); }  ///<indirectrion dereference access
  bool operator==(const RefControl<DataType>& A) { return Data==A.Data; } ///< Based on ptr equality
  DataType& access();

};

template<typename DataType>
RefControl<DataType>::RefControl() :
  Data(new DataType())
  /*!
    Constructor : creates new data() object
  */
{ }


template<typename DataType>
RefControl<DataType>::RefControl(const RefControl<DataType>& A) :
  Data(A.Data)
  /*!
    Copy constructor : double references the data object
    \param A :: object to copy
  */
{ }

template<typename DataType>
RefControl<DataType>&
RefControl<DataType>::operator=(const RefControl<DataType>& A) 
  /*!
    Assignment operator : double references the data object
    maybe drops the old reference.
    \param A :: object to copy
    \return *this
  */
{
  if (this!=&A)
    {
      Data=A.Data;
    }
  return *this;
}

template<typename DataType>
RefControl<DataType>&
RefControl<DataType>::operator=(const ptr_type& A) 
  /*!
    Assignment operator : double references the data object
    maybe drops the old reference.
    \param A :: object to copy
    \return *this
  */
{
  if (this->Data != A)
    {
      Data=A;
    }
  return *this;
}


template<typename DataType>
RefControl<DataType>::~RefControl()
  /*!
    Destructor : No work is required since Data is
    a shared_ptr.
  */
{}

template<typename DataType>
DataType&
RefControl<DataType>::access()
  /*!
    Access function 
    Creates a copy of Data so that it can be modified.
    Believed to be thread safe sicne
    creates an extra reference before deleteing.
    \return new copy of *this
  */
{
  if (Data.unique())
    return *Data;

  ptr_type oldData=Data; 
  Data.reset();
  Data=ptr_type(new DataType(*oldData));
  return *Data;
}

} // NAMESPACE Mantid

#endif
