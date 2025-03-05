// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/RegistrationHelper.h"
#include "MantidKernel/SingletonHolder.h"

#include <boost/regex.hpp>

namespace Mantid {
namespace Geometry {

class MANTID_GEOMETRY_DLL PointGroupGenerator {
public:
  PointGroupGenerator(std::string hmSymbol, std::string generatorInformation, std::string description);

  const inline std::string &getHMSymbol() const { return m_hmSymbol; }
  const inline std::string &getGeneratorString() const { return m_generatorString; }
  const inline std::string &getDescription() const { return m_description; }

  PointGroup_sptr getPrototype();

private:
  inline bool hasValidPrototype() const { return static_cast<bool>(m_prototype); }

  PointGroup_sptr generatePrototype();

  std::string m_hmSymbol;
  std::string m_generatorString;
  std::string m_description;

  PointGroup_sptr m_prototype;
};

using PointGroupGenerator_sptr = std::shared_ptr<PointGroupGenerator>;

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
*/
class MANTID_GEOMETRY_DLL PointGroupFactoryImpl {
public:
  PointGroup_sptr createPointGroup(const std::string &hmSymbol);
  PointGroup_sptr createPointGroupFromSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);
  PointGroup_sptr createPointGroupFromSpaceGroup(const SpaceGroup &spaceGroup);

  bool isSubscribed(const std::string &hmSymbol) const;

  std::vector<std::string> getAllPointGroupSymbols() const;
  std::vector<std::string> getPointGroupSymbols(const PointGroup::CrystalSystem &crystalSystem);

  void subscribePointGroup(const std::string &hmSymbol, const std::string &generatorString,
                           const std::string &description);

  /// Unsubscribes a point group from the factory
  void unsubscribePointGroup(const std::string &hmSymbol) { m_generatorMap.erase(hmSymbol); }

private:
  friend struct Mantid::Kernel::CreateUsingNew<PointGroupFactoryImpl>;

  PointGroupFactoryImpl();

  std::string pointGroupSymbolFromSpaceGroupSymbol(const std::string &spaceGroupSymbol) const;

  PointGroup_sptr getPrototype(const std::string &hmSymbol);
  void subscribe(const PointGroupGenerator_sptr &generator);
  PointGroup_sptr constructFromPrototype(const PointGroup_sptr &prototype) const;

  std::map<std::string, PointGroupGenerator_sptr> m_generatorMap;
  std::map<std::string, PointGroup::CrystalSystem> m_crystalSystemMap;

  boost::regex m_screwAxisRegex;
  boost::regex m_glidePlaneRegex;
  boost::regex m_centeringRegex;
  boost::regex m_originChoiceRegex;
};

using PointGroupFactory = Mantid::Kernel::SingletonHolder<PointGroupFactoryImpl>;

} // namespace Geometry
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_GEOMETRY template class MANTID_GEOMETRY_DLL
    Mantid::Kernel::SingletonHolder<Mantid::Geometry::PointGroupFactoryImpl>;
}
} // namespace Mantid

#define PGF_CONCAT_IMPL(x, y) x##y
#define PGF_CONCAT(x, y) PGF_CONCAT_IMPL(x, y)

#define DECLARE_POINTGROUP(hmSymbol, generators, description)                                                          \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper PGF_CONCAT(register_pointgroup, __COUNTER__)(                                     \
      ((Mantid::Geometry::PointGroupFactory::Instance().subscribePointGroup(hmSymbol, generators, description)), 0));  \
  }
