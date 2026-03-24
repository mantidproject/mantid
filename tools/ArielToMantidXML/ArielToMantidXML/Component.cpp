// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Component.h"
#include "DatReader.h"
#include "AssReader.h"
#include <iostream>
#include <sstream>

int Component::counter = 0;

Component::Component(const std::string& name, const std::string& type) :
  m_name(name), m_type(type), m_parent(NULL), m_r(0), m_theta(0), m_phi(0)
{
  // Open the .dat file for this type and check whether it's a composite type
  DatReader dat(type);
  m_isAssembly = dat.isAssembly();

  if ( m_name.find("Det") == 0 ) ++counter;
}

Component::~Component(void)
{
}

void Component::setParent(Component* parent)
{
  m_parent = parent;
}

void Component::findChildren()
{
  // If the current component is a composite type....
  if (m_isAssembly)
  {
    // Open the .ass file
    AssReader ass(m_type);
    Component *comp;
    // Parse the .ass file finding the children
    while( comp = ass.parseFile() )
    {
      comp->setParent(this);
      m_children.push_back(comp);
      // Find the current child's children
      comp->findChildren();
    }
  }
}

void Component::setSpherical(const double &r, const double &theta, const double &phi)
{
  m_r = r;
  m_theta = theta;
  m_phi = phi;
}

const std::string Component::findL1()
{
  DatReader dat(m_type);
  return dat.findL1();
}

/* The methods below were used when I was trying to get the relative positions
   out as offsets in x, y & z.
*/
//void Component::getAbsSpherical(double& r, double& theta, double& phi)
//{
//  r=0; theta=0; phi=0;
//  if ( m_parent ) m_parent->getAbsSpherical(r,theta,phi);
//  r += m_r;
//  theta += m_theta;
//  phi += m_phi;
//}
//
//void Component::calculatePos()
//{
//  if ( m_parent )
//  {
//    double r, theta, phi;
//    this->getAbsSpherical(r,theta,phi);
//    Mantid::Geometry::V3D absPos;
//    absPos.spherical(r,theta,phi);
//    m_pos = absPos - m_parent->m_pos;
//  }
//  else
//  {
//    m_pos.spherical(m_r,m_theta,m_phi);
//  }
//
//  std::vector<Component*>::iterator it;
//  for (it = m_children.begin(); it != m_children.end(); ++it)
//  {
//    (*it)->calculatePos();
//  }
//}
//
//const Mantid::Geometry::V3D& Component::pos() const
//{
//  return m_pos;
//}

const std::string& Component::name() const
{
  return m_name;
}

const std::string& Component::type() const
{
  return m_type;
}

const std::string Component::printPos() const
{
  std::stringstream output;
  output << " r=\"" << m_r << "\" t=\"" << m_theta << "\" p=\"" << m_phi << "\"";
  return output.str();
}

const Component* const Component::parent() const
{
  return m_parent;
}

const bool Component::hasChildren() const
{
  return ( ! m_children.empty() );
}
