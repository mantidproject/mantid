#include "MantidKernel/VMD.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::VMD;
using Mantid::Kernel::VMD_t;
using namespace boost::python;

namespace
{
  /**
   * Safe operator access. Returns the value at the given index
   * checking whether the index is valid. VMD does no checking
   * @param self The calling python object
   * @param index An index whose value is to be returned
   * @throws An out_of_range error if the index is out of range
   */
  VMD_t getItem(VMD & self, const size_t index)
  {
    if( index < self.getNumDims() )
    {
      return self[index];
    }
    else throw std::out_of_range("VMD index out of range. index=" + \
        boost::lexical_cast<std::string>(index) + ", len=" + boost::lexical_cast<std::string>(self.getNumDims()));
  }

  /**
   * Set the value at the given index
   * @param self The calling python object
   * @param index An index whose value is to be set
   * @param value The new value for the index
   * @throws An out_of_range error if the index is out of range
   */
  void setItem(VMD & self, const size_t index, const VMD_t value)
  {
    if( index < self.getNumDims() )
    {
      self[index] = value;
    }
    else throw std::out_of_range("VMD index out of range. index=" + \
        boost::lexical_cast<std::string>(index) + ", len=" + boost::lexical_cast<std::string>(self.getNumDims()));
  }
}

void export_VMD()
{
  class_<VMD>("VMD", init<>("Default constructor gives an object with 1 dimension"))
    .def(init<VMD_t,VMD_t>("Constructs a 2 dimensional vector at the point given", args(("val0"),("val1"))))
    .def(init<VMD_t,VMD_t,VMD_t>("Constructs a 3 dimensional vector at the point given", 
                                 args(("val0"),("val1"),("val2"))))
    .def(init<VMD_t,VMD_t,VMD_t,VMD_t>("Constructs a 4 dimensional vector at the point given", 
                                       args(("val0"),("val1"),("val2"),("val3"))))
    .def(init<VMD_t,VMD_t,VMD_t,VMD_t,VMD_t>("Constructs a 5 dimensional vector at the point given", 
                                             args(("val0"),("val1"),("val2"),("val3"),("val4"))))
    .def(init<VMD_t,VMD_t,VMD_t,VMD_t,VMD_t,VMD_t>("Constructs a 6 dimensional vector at the point given", 
                                                   args(("val0"),("val1"),("val2"),("val3"),("val5"))))

    .def("getNumDims", &VMD::getNumDims, "Returns the number of dimensions the contained in the vector")

    .def("scalar_prod", &VMD::scalar_prod,
         "Returns the scalar product of this vector with another. If the number of dimensions do not match a RuntimeError is raised")

    .def("cross_prod", &VMD::cross_prod,
         "Returns the cross product of this vector with another. If the number of dimensions do not match a RuntimeError is raised")

    .def("norm", &VMD::norm, "Returns the length of the vector")

    .def("norm2", &VMD::norm2, "Returns the the squared length of the vector")

    .def("normalize", &VMD::normalize, "Normalizes the length of the vector to unity and returns the length before it was normalized")

    .def("angle", &VMD::angle, "Returns the angle between the vectors in radians (0 < theta < pi). If the dimensions do not match a RuntimeError is raised")

    //----------------------------- special methods --------------------------------
    .def("__getitem__", &getItem)
    .def("__setitem__", &setItem)
     // cppcheck-suppress duplicateExpression
    .def(self == self)
    .def(self + self)
    .def(self += self)
     // cppcheck-suppress duplicateExpression
    .def(self - self)
    .def(self -= self)
    .def(self * self)
    .def(self *= self)
    .def(self / self)
    .def(self /= self)
    ;
}

