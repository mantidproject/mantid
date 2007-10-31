#ifndef Property_h
#define Property_h

/*!
  \class Property
  \version 1.0
  \author S. Ansell
  \date October 2007

  Lifted from the JAVA example on JCWA
 */

template <class T>
class Property 
{
 private:

  T data;

 public:

  Property() : data() {}

  T operator()() const { return data; }
  T operator()(const T& V) { data = V; return data; }


  T get() const { return data; }
  T set(const T& value) { data = value; return data; }
  
  operator T() const { return data; }
  T operator=(const T& value) { data = value; return data; }

  typedef T value_type;

};

v/*!
  \class ROProperty
  \version 1.0
  \author S. Ansell
  \date October 2007
  \brief Read Object Property
 */

template <typename T, typename Object,
          T (Object::*real_getter)()>
class ROProperty 
{
private:

  Object* self;

public:

  ROProperty(Object * me = 0) : self(me) {}

  void operator()(Object * obj) { self = obj; }

  // function call syntax
  T operator()() const { return (self->*real_getter)();  }

  // get/set syntax
  T get() const { return (self->*real_getter)();  }
  void set(T const & value);
  // use on rhs of '='
  operator T() const { return (self->*real_getter)(); }

  typedef T value_type;

};

// a write-only property calling a
// user-defined setter
template <class T, class Object,
          T (Object::*real_setter)(T const &)>
class WOProperty 
{
  Object * self;

public:
  
  WOProperty(Object* Aobj = 0)   // Explicit ????
    : self(Aobj) {}

  void operator()(Object* Aobj) { self = Aobj; }
  // function call syntax
  T operator()(const T& value) { return (self->*real_setter)(value); }
  // get/set syntax
  T get() const;
  T set(const T& value) {  return (self->*real_setter)(value); }

  T operator=(const T& value) { return (self->*real_setter)(value); }

  typedef T value_type;

};

/*!
  \class RWProperty
  \version 1.0
  \author S. Ansell
  \brief Holds a Read/Write property
*/

template <class T,
          class Object,
          T (Object::*real_getter)(),
          T (Object::*real_setter)(const T&)>
class RWProperty 
{
 private:

  Object* self;

 public:

  RWProperty() : self(0) {}
  RWProperty(Object* me) : self(me) {}

  void operator()(Object * obj) {  self = obj;  }

  // function call syntax
  T operator()() const { return (self->*real_getter)();  }
  T operator()(const T& value) {  return (self->*real_setter)(value);  }

  // get/set syntax
  T get() const { return (self->*real_getter)(); }

  T set(const T& value) { return (self->*real_setter)(value); }

  operator T() const {  return (self->*real_getter)();  }
  T operator=(const T& value) { return (self->*real_setter)(value); }
  typedef T value_type;

};

/*!
  \class IndexProperty
  \version 1.0
  \author S. Ansell
  \brief Holds Index components DOES NOT WORK
  \todo FINISH
*/

template <class Key,
          class T,
          class Compare = std::less<Key>,
          class Allocator = std::allocator<std::pair<const Key, T> > >
class IndexedProperty 
{
 private:

  std::map<Key,T,Compare,Allocator> data;
  typedef typename std::map<Key,T,Compare,Allocator>::iterator map_iterator;

public:

  // function call syntax
  T operator()(const Key& key) 
    { 
      std::pair<map_iterator, bool> result;
      result=data.insert(std::make_pair(key,T()));
      return (*result.first).second;
    }

  T operator()(const Key& key,const T& t) 
    {
      std::pair<map_iterator, bool> result;
      result=data.insert(std::make_pair(key, t));
      return (*result.first).second;
    }

  // get/set syntax
  T get_Item(const Key& key) 
    {
      std::pair<map_iterator, bool> result;
      result=data.insert(std::make_pair(key, T()));
      return (*result.first).second;
    }
  T set_Item(const Key& key,const T& t) 
    {
      std::pair<map_iterator, bool> result;
      result=data.insert(std::make_pair(key, t));
      return (*result.first).second;
    }

  // operator [] syntax
  T& operator[](const Key& key) 
    {
      return (*((data.insert(make_pair(key, T()))).first)).second;
    }
};

#endif
