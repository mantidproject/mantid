#include "MantidKernel/Material.h"
#include "MantidKernel/NeutronAtom.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::Material;
using Mantid::Kernel::NeutronAtom;
using namespace boost::python;

void export_Material()
{
  register_ptr_to_python<Material*>();
  register_ptr_to_python<boost::shared_ptr<Material> >();

  class_<Material, boost::noncopyable >("Material", no_init)
    .def("name", &Material::name, return_value_policy<copy_const_reference>(), "Name of the material")
    .add_property("numberDensity", make_function(&Material::numberDensity), "Number density")
    .add_property("temperature", make_function(&Material::temperature), "Temperature")
    .add_property("pressure", make_function(&Material::pressure), "Pressure")
    .def("cohScatterXSection", (double (Material::*)(double) const)(&Material::cohScatterXSection),
         (arg("lambda")=(double)NeutronAtom::ReferenceLambda),
         "Coherent Scattering Cross-Section")
    .def("incohScatterXSection", (double (Material::*)(double) const)(&Material::incohScatterXSection),
         (arg("lambda")=(double)NeutronAtom::ReferenceLambda),
         "Incoherent Scattering Cross-Section")
    .def("totalScatterXSection", (double (Material::*)(double) const)(&Material::totalScatterXSection),
         (arg("lambda")=(double)NeutronAtom::ReferenceLambda),
         "Total Scattering Cross-Section")
    .def("absorbXSection", (double (Material::*)(double) const)(&Material::absorbXSection),
         (arg("lambda")=(double)NeutronAtom::ReferenceLambda),
         "Absorption Cross-Section")
  ;
}
