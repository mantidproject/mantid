// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>
#include <vector>
#include <fstream>
//#include "MantidGeometry/V3D.h"

class Component
{
public:
  Component(const std::string& name, const std::string& type);
  virtual ~Component(void);

  // Recursive method that searches the tree finding all children from the starting point
  void findChildren();
  // Sets the parent of the current component
  void setParent(Component* parent);
  // Sets the
  void setSpherical(const double& r, const double& theta, const double& phi);
  // Finds the primary flightpath.
  // Only works on an 'instrument'/top component(returns empty string otherwise)
  const std::string findL1();

  // Returns the name of this component
  const std::string& name() const;
  // Returns the type of this component
  const std::string& type() const;
  // Returns a string with the position of this component
  const std::string printPos() const;
  // Returns the parent of this component
  const Component* const parent() const;
  // Flag indicating whether this component has any children
  const bool hasChildren() const;

  // Static variable to count the total number of detectors found (just for checking)
  static int counter;

protected:
  // Returns a vector of the children of this component
  std::vector<Component*> getChildren() const { return m_children; }

private:
  Component(const Component&);

  const std::string m_name;
  const std::string m_type;

  bool m_isAssembly;

  Component* m_parent;
  std::vector<Component*> m_children;

  double m_r, m_theta, m_phi;
  //Mantid::Geometry::V3D m_pos;
};
