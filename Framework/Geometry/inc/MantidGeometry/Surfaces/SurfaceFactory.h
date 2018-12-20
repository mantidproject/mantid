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

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
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
