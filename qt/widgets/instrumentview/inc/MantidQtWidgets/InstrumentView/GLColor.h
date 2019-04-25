// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GLCOLOR_H_
#define GLCOLOR_H_

#include <iosfwd>

namespace MantidQt {
namespace MantidWidgets {

/**
\class  GLColor
\brief  class handling OpenGL color for objects
\author Chapon Laurent & Srikanth Nagella
\date   August 2008
\version 1.0

GLColor class handles the OpenGL color for an object based on the type of the
rendering selected. eg. MATERIAL by specifying color as glMaterial rather than
glColor.
*/
class GLColor {
public:
  /// Default Constructor
  GLColor(float red = 0, float green = 0, float blue = 0, float alpha = 1.0f);
  GLColor(int r, int g, int b, int a = 255);
  /// Destructor
  virtual ~GLColor();

  /// Set all four values atomically
  void set(float red, float green, float blue, float alpha);
  /// Retrieve the component colours
  void get(float & /*red*/, float & /*green*/, float & /*blue*/,
           float & /*alpha*/) const;
  void get(unsigned char &r, unsigned char &g, unsigned char &b) const;
  /// Retrieve the component colours
  void getUB3(unsigned char *c) const;
  /// Set the painting method
  void paint() const;
  int red() const { return int(m_rgba[0]); }
  int green() const { return int(m_rgba[1]); }
  int blue() const { return int(m_rgba[2]); }
  int alpha() const { return int(m_rgba[3]); }

private:
  /// The individual components
  // float m_rgba[4];
  unsigned char m_rgba[4];
};

std::ostream &operator<<(std::ostream &ostr, const GLColor &c);
} // namespace MantidWidgets
} // namespace MantidQt

#endif /*GLCOLOR_H_*/
