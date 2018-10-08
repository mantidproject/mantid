// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef surfaceFactory_h
#define surfaceFactory_h

namespace Mantid {

namespace Geometry {

/**
  \class SurfaceFactory
  \brief Creates instances of Surfaces
  \version 1.0
  \date May 2006
  \author S. Ansell

  This is a singleton class.
  It creates Surface* depending registered tallies
  and the key given. Predominately for creating
  tallies from an input deck where we only have the number.
*/

class MANTID_GEOMETRY_DLL SurfaceFactory {
private:
  // workaround because gcc 4.4 cannot have std::unique_ptr inside a std::map.
  // http://stackoverflow.com/questions/7342703/gcc-4-4-4-5-unique-ptr-not-work-for-unordered-set-unordered-map

  ///< Storage of surface pointers.
  using MapType = std::vector<std::pair<std::string, std::unique_ptr<Surface>>>;

  static SurfaceFactory *FOBJ; ///< Effective "this"

  MapType SGrid;                  ///< The tally stack
  std::map<char, std::string> ID; ///< Short letter identifiers
  SurfaceFactory(const SurfaceFactory &other);
  SurfaceFactory &operator=(const SurfaceFactory &other);
  SurfaceFactory(); ///< singleton constructor
  void registerSurface();

public:
  static SurfaceFactory *Instance();

  std::unique_ptr<Surface> createSurface(const std::string &) const;
  std::unique_ptr<Surface> createSurfaceID(const std::string &) const;
  std::unique_ptr<Surface> processLine(const std::string &) const;
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif
