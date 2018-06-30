#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <stack>
#include <vector>

#include "MantidGeometry/Surfaces/BaseVisit.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/General.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/make_unique.h"
//#include "MantidGeometry/Surfaces/Torus.h"
#include "MantidGeometry/Surfaces/SurfaceFactory.h"

namespace Mantid {

namespace Geometry {

SurfaceFactory *SurfaceFactory::FOBJ(nullptr);

SurfaceFactory *SurfaceFactory::Instance()
/**
  Effective new command / this command
  @return Single instance of SurfaceFactory
*/
{
  if (!FOBJ) {
    FOBJ = new SurfaceFactory();
  }
  return FOBJ;
}

SurfaceFactory::SurfaceFactory()
/**
  Constructor
*/
{
  registerSurface();
}

SurfaceFactory::SurfaceFactory(const SurfaceFactory &other) : ID(other.ID) {
  for (const auto &vc : other.SGrid)
    this->SGrid.emplace_back(vc.first, vc.second->clone());
}

SurfaceFactory &SurfaceFactory::operator=(const SurfaceFactory &other) {
  if (this != &other) // protect against invalid self-assignment
  {
    this->ID = other.ID;
    for (const auto &vc : other.SGrid)
      this->SGrid.emplace_back(vc.first, vc.second->clone());
  }
  return *this;
}

void SurfaceFactory::registerSurface()
/**
  Register tallies to be used
*/
{
  using Mantid::Kernel::make_unique;
  SGrid.emplace_back("Plane", make_unique<Plane>());
  SGrid.emplace_back("Cylinder", make_unique<Cylinder>());
  SGrid.emplace_back("Cone", make_unique<Cone>());
  // SGrid["Torus"]=new Torus;
  SGrid.emplace_back("General", make_unique<General>());
  SGrid.emplace_back("Sphere", make_unique<Sphere>());

  ID['c'] = "Cylinder";
  ID['k'] = "Cone";
  ID['g'] = "General";
  ID['p'] = "Plane";
  ID['s'] = "Sphere";
  // ID['t']="Torus";}
}
namespace {
class KeyEquals {
public:
  explicit KeyEquals(const std::string &key) : m_key(key) {}
  bool
  operator()(const std::pair<std::string, std::unique_ptr<Surface>> &element) {
    return m_key == element.first;
  }

private:
  std::string m_key;
};
} // namespace

std::unique_ptr<Surface>
SurfaceFactory::createSurface(const std::string &Key) const
/**
  Creates an instance of tally
  given a valid key.

  @param Key :: Item to get
  @throw NotFoundError for the key if not found
  @return new tally object.
*/
{
  MapType::const_iterator vc;
  vc = std::find_if(SGrid.begin(), SGrid.end(), KeyEquals(Key));
  if (vc == SGrid.end()) {
    throw Kernel::Exception::NotFoundError("SurfaceFactory::createSurface",
                                           Key);
  }
  return vc->second->clone();
}

std::unique_ptr<Surface>
SurfaceFactory::createSurfaceID(const std::string &Key) const
/**
  Creates an instance of tally
  given a valid key.

  @param Key :: Form of first ID
  @throw NotFoundError for the key if not found
  @return new tally object.
*/
{
  std::map<char, std::string>::const_iterator mc;

  mc = (Key.empty()) ? ID.end() : ID.find(static_cast<char>(tolower(Key[0])));
  if (mc == ID.end()) {
    throw Kernel::Exception::NotFoundError("SurfaceFactory::createSurfaceID",
                                           Key);
  }

  return createSurface(mc->second);
}

std::unique_ptr<Surface>
SurfaceFactory::processLine(const std::string &Line) const
/**
  Creates an instance of a surface
  given a valid line

  @param Line :: Full description of line
  @throw InContainerError for the key if not found
  @return new surface object.
*/
{
  std::string key;
  if (!Mantid::Kernel::Strings::convert(Line, key))
    throw Kernel::Exception::NotFoundError("SurfaceFactory::processLine", Line);

  std::unique_ptr<Surface> X = createSurfaceID(key);
  if (X->setSurface(Line)) {
    std::cerr << "X:: " << X->setSurface(Line) << '\n';
    throw Kernel::Exception::NotFoundError("SurfaceFactory::processLine", Line);
  }

  return X;
}

} // NAMESPACE Geometry

} // NAMESPACE Mantid
