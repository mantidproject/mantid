#ifndef MANTID_GEOMETRY_POINTGROUPFACTORY_H_
#define MANTID_GEOMETRY_POINTGROUPFACTORY_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidKernel/RegistrationHelper.h"

#include <boost/regex.hpp>

namespace Mantid {
namespace Geometry {

class MANTID_GEOMETRY_DLL PointGroupGenerator {
public:
  PointGroupGenerator(const std::string &hmSymbol,
                      const std::string &generatorInformation,
                      const std::string &description);

  ~PointGroupGenerator() {}

  inline std::string getHMSymbol() const { return m_hmSymbol; }
  inline std::string getGeneratorString() const { return m_generatorString; }
  inline std::string getDescription() const { return m_description; }

  PointGroup_sptr getPrototype();

private:
  inline bool hasValidPrototype() const {
    return static_cast<bool>(m_prototype);
  }

  PointGroup_sptr generatePrototype();

  std::string m_hmSymbol;
  std::string m_generatorString;
  std::string m_description;

  PointGroup_sptr m_prototype;
};

typedef boost::shared_ptr<PointGroupGenerator> PointGroupGenerator_sptr;

/**
  @class PointGroupFactory

  A factory for point groups. Point group objects can be constructed by
  supplying the Hermann-Mauguin-symbol like this:

      PointGroup_sptr cubic =
          PointGroupFactory::Instance().createPointgroup("m-3m");

  Furthermore it's possible to query available point groups, either all
  available
  groups or only point groups belonging to a certain crystal system.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 09/09/2014

    Copyright © 2014 PSI-MSS

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
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_GEOMETRY_DLL PointGroupFactoryImpl {
public:
  PointGroup_sptr createPointGroup(const std::string &hmSymbol);
  PointGroup_sptr
  createPointGroupFromSpaceGroupSymbol(const std::string &spaceGroupSymbol);

  bool isSubscribed(const std::string &hmSymbol) const;

  std::vector<std::string> getAllPointGroupSymbols() const;
  std::vector<std::string>
  getPointGroupSymbols(const PointGroup::CrystalSystem &crystalSystem) const;

  void subscribePointGroup(const std::string &hmSymbol,
                           const std::string &generatorString,
                           const std::string &description);

  /// Unsubscribes a point group from the factory
  void unsubscribePointGroup(const std::string &hmSymbol) {
    // unsubscribe(hmSymbol);
    removeFromCrystalSystemMap(hmSymbol);
  }

private:
  friend struct Mantid::Kernel::CreateUsingNew<PointGroupFactoryImpl>;

  PointGroupFactoryImpl();
  void addToCrystalSystemMap(const PointGroup::CrystalSystem &crystalSystem,
                             const std::string &hmSymbol);
  void removeFromCrystalSystemMap(const std::string &hmSymbol);

  std::string pointGroupSymbolFromSpaceGroupSymbol(
      const std::string &spaceGroupSymbol) const;

  PointGroup_sptr getPrototype(const std::string &hmSymbol);
  void subscribe(const PointGroupGenerator_sptr &generator);
  PointGroup_sptr
  constructFromPrototype(const PointGroup_sptr &prototype) const;

  std::map<std::string, PointGroupGenerator_sptr> m_generatorMap;
  std::map<std::string, PointGroup::CrystalSystem> m_crystalSystemMap;

  boost::regex m_screwAxisRegex;
  boost::regex m_glidePlaneRegex;
  boost::regex m_centeringRegex;
  boost::regex m_originChoiceRegex;
};

// This is taken from FuncMinimizerFactory
#ifdef _WIN32
template class MANTID_GEOMETRY_DLL
Mantid::Kernel::SingletonHolder<PointGroupFactoryImpl>;
#endif

typedef Mantid::Kernel::SingletonHolder<PointGroupFactoryImpl>
PointGroupFactory;

} // namespace Geometry
} // namespace Mantid

#define PGF_CONCAT_IMPL(x, y) x##y
#define PGF_CONCAT(x, y) PGF_CONCAT_IMPL(x, y)

#define DECLARE_POINTGROUP(hmSymbol, generators, description)                  \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper PGF_CONCAT(register_pointgroup,           \
                                                __COUNTER__)(                  \
      ((Mantid::Geometry::PointGroupFactory::Instance().subscribePointGroup(   \
           hmSymbol, generators, description)),                                \
       0));                                                                    \
  }

#endif /* MANTID_GEOMETRY_POINTGROUPFACTORY_H_ */
