#include "MantidKernel/VMD.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::VMD;
using Mantid::Kernel::VMD_t;
using namespace boost::python;

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

    //----------------------------- special methods --------------------------------
    .def("__getitem__", (const VMD_t &(VMD::*)(size_t)const)&VMD::operator[], return_value_policy<copy_const_reference>())
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

