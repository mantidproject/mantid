#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidKernel/System.h"

#include <set>
#include <boost/make_shared.hpp>

namespace Mantid
{
namespace Geometry
{
  using Kernel::V3D;
  using Kernel::IntMatrix;

  std::vector<V3D> PointGroup::getEquivalents(const V3D &hkl) const
  {
    std::set<V3D> equivalents = getEquivalentSet(hkl);

    return std::vector<V3D>(equivalents.rbegin(), equivalents.rend());
  }

  V3D PointGroup::getReflectionFamily(const Kernel::V3D &hkl) const
  {
    return *getEquivalentSet(hkl).rbegin();
  }

  PointGroup::PointGroup() :
      m_symmetryOperations(),
      m_transformationMatrices()
  {
  }

  std::set<V3D> PointGroup::getEquivalentSet(const Kernel::V3D &hkl) const
  {
      std::set<V3D> equivalents;
      equivalents.insert(hkl);

      for(std::vector<IntMatrix>::const_iterator m = m_transformationMatrices.begin();
          m != m_transformationMatrices.end(); ++m) {
          equivalents.insert((*m) * hkl);
      }

      return equivalents;
  }

  void PointGroup::addSymmetryOperation(const SymmetryOperation_const_sptr &symmetryOperation)
  {
      m_symmetryOperations.push_back(symmetryOperation);
  }

  void PointGroup::calculateTransformationMatrices(const std::vector<SymmetryOperation_const_sptr> &symmetryOperations)
  {
      m_transformationMatrices.clear();

      std::vector<IntMatrix> trans;
      trans.push_back(IntMatrix(3,3,true));

      for(std::vector<SymmetryOperation_const_sptr>::const_iterator symOp = symmetryOperations.begin();
          symOp != symmetryOperations.end();
          ++symOp) {
          std::vector<IntMatrix> currentMatrices(trans);

          for(std::vector<IntMatrix>::const_iterator currentMatrix = currentMatrices.begin();
              currentMatrix != currentMatrices.end();
              ++currentMatrix) {
              IntMatrix transformed = *currentMatrix;
              for(size_t i = 0; i < (*symOp)->order() - 1; ++i) {
                  transformed = (*symOp)->apply(transformed);
                  trans.push_back(transformed);
              }
          }
      }

      m_transformationMatrices = std::vector<IntMatrix>(trans.begin(), trans.end());
  }

  std::vector<SymmetryOperation_const_sptr> PointGroup::getSymmetryOperations() const
  {
      return m_symmetryOperations;
  }

  PointGroupLaue1::PointGroupLaue1()
  {
      addSymmetryOperation(boost::make_shared<const SymOpInversion>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue1::getName()
  {
      return "-1 (Triclinic)";
  }

  bool PointGroupLaue1::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,-l));
  }

  PointGroupLaue2::PointGroupLaue2()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldY>());
      addSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneY>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue2::getName()
  {
      return "1 2/m 1 (Monoclinic, unique axis b)";
  }

  bool PointGroupLaue2::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(-h,k,-l)) || (hkl2 == V3D(h,-k,l));
  }

  PointGroupLaue3::PointGroupLaue3()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldZ>());
      addSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneZ>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue3::getName()
  {
      return "1 1 2/m (Monoclinic, unique axis c)";
  }

  bool PointGroupLaue3::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l));
  }

  PointGroupLaue4::PointGroupLaue4()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldX>());
      addSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldY>());
      addSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneZ>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue4::getName()
  {
      return "mmm (Orthorombic)";
  }

  bool PointGroupLaue4::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,k,-l))
              || (hkl2 == V3D(h,-k,-l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l))
              || (hkl2 == V3D(h,-k,l)) || (hkl2 == V3D(-h,k,l));
  }

  PointGroupLaue5::PointGroupLaue5()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationFourFoldZ>());
      addSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneZ>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue5::getName()
  {
      return "4/m (Tetragonal)";
  }

  bool PointGroupLaue5::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-k,h,l))
              || (hkl2 == V3D(k,-h,l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l))
              || (hkl2 == V3D(k,-h,-l)) || (hkl2 == V3D(-k,h,-l));
  }

  PointGroupLaue6::PointGroupLaue6()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationFourFoldZ>());
      addSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldX>());
      addSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneZ>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue6::getName()
  {
      return "4/mmm (Tetragonal)";
  }

  bool PointGroupLaue6::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-k,h,l))
              || (hkl2 == V3D(k,-h,l)) || (hkl2 == V3D(-h,k,-l)) || (hkl2 == V3D(h,-k,-l))
              || (hkl2 == V3D(k,h,-l)) || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(-h,-k,-l))
              || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(k,-h,-l)) || (hkl2 == V3D(-k,h,-l))
              || (hkl2 == V3D(h,-k,l)) || (hkl2 == V3D(-h,k,l)) || (hkl2 == V3D(-k,-h,l))
              || (hkl2 == V3D(k,h,l));
  }

  PointGroupLaue7::PointGroupLaue7()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationThreeFoldZHexagonal>());
      addSymmetryOperation(boost::make_shared<const SymOpInversion>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue7::getName()
  {
      return "-3 (Trigonal - Hexagonal)";
  }

  bool PointGroupLaue7::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l));
  }

  PointGroupLaue8::PointGroupLaue8()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationThreeFoldZHexagonal>());
      addSymmetryOperation(boost::make_shared<const SymOpInversion>());
      addSymmetryOperation(boost::make_shared<const SymOpMirrorPlane210Hexagonal>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue8::getName()
  {
      return "-3m1 (Trigonal - Rhombohedral)";
  }

  bool PointGroupLaue8::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l))
              || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(-h+k,k,-l)) || (hkl2 == V3D(h,h-k,-l))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l))
              || (hkl2 == V3D(k,h,l)) || (hkl2 == V3D(h-k,-k,l)) || (hkl2 == V3D(-h,-h+k,l));
  }

  PointGroupLaue9::PointGroupLaue9()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationThreeFoldZHexagonal>());
      addSymmetryOperation(boost::make_shared<const SymOpInversion>());
      addSymmetryOperation(boost::make_shared<const SymOpRotationTwoFold210Hexagonal>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue9::getName()
  {
      return "-31m (Trigonal - Rhombohedral)";
  }

  bool PointGroupLaue9::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l))
              || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(-h+k,k,-l)) || (hkl2 == V3D(h,h-k,-l))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l))
              || (hkl2 == V3D(k,h,l)) || (hkl2 == V3D(h-k,-k,l)) || (hkl2 == V3D(-h,-h+k,l));
  }

  PointGroupLaue10::PointGroupLaue10()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationSixFoldZHexagonal>());
      addSymmetryOperation(boost::make_shared<const SymOpInversion>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue10::getName()
  {
      return "6/m (Hexagonal)";
  }

  bool PointGroupLaue10::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l))
              || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(k,-h+k,l)) || (hkl2 == V3D(h-k,h,l))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l))
              || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(-k,h-k,-l)) || (hkl2 == V3D(-h+k,-h,-l));
  }

  PointGroupLaue11::PointGroupLaue11()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationSixFoldZHexagonal>());
      addSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldXHexagonal>());
      addSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneZ>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue11::getName()
  {
      return "6/mmm (Hexagonal)";
  }

  bool PointGroupLaue11::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l))
              || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(k,-h+k,l)) || (hkl2 == V3D(h-k,h,l))
              || (hkl2 == V3D(k,h,-l)) || (hkl2 == V3D(h-k,-k,-l)) || (hkl2 == V3D(-h,-h+k,-l))
              || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(-h+k,k,-l)) || (hkl2 == V3D(h,h-k,-l))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l))
              || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(-k,h-k,-l)) || (hkl2 == V3D(-h+k,-h,-l))
              || (hkl2 == V3D(-k,-h,l)) || (hkl2 == V3D(-h+k,k,l)) || (hkl2 == V3D(h,h-k,l))
              || (hkl2 == V3D(k,h,l)) || (hkl2 == V3D(h-k,-k,l)) || (hkl2 == V3D(-h,-h+k,l));
  }

  PointGroupLaue12::PointGroupLaue12()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationThreeFold111>());
      addSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldZ>());
      addSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneY>());
      addSymmetryOperation(boost::make_shared<const SymOpInversion>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue12::getName()
  {
      return "m-3 (Cubic)";
  }

  bool PointGroupLaue12::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,k,-l))
              || (hkl2 == V3D(h,-k,-l)) || (hkl2 == V3D(l,h,k)) || (hkl2 == V3D(l,-h,-k))
              || (hkl2 == V3D(-l,-h,k)) || (hkl2 == V3D(-l,h,-k)) || (hkl2 == V3D(k,l,h))
              || (hkl2 == V3D(-k,l,-h)) || (hkl2 == V3D(k,-l,-h)) || (hkl2 == V3D(-k,-l,h))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(h,-k,l))
              || (hkl2 == V3D(-h,k,l)) || (hkl2 == V3D(-l,-h,-k)) || (hkl2 == V3D(-l,h,k))
              || (hkl2 == V3D(l,h,-k)) || (hkl2 == V3D(l,-h,k)) || (hkl2 == V3D(-k,-l,-h))
              || (hkl2 == V3D(k,-l,h)) || (hkl2 == V3D(-k,l,h)) || (hkl2 == V3D(k,l,-h));
  }

  PointGroupLaue13::PointGroupLaue13()
  {
      addSymmetryOperation(boost::make_shared<const SymOpRotationThreeFold111>());
      addSymmetryOperation(boost::make_shared<const SymOpRotationFourFoldZ>());
      addSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneY>());
      addSymmetryOperation(boost::make_shared<const SymOpInversion>());

      calculateTransformationMatrices(getSymmetryOperations());
  }

  std::string PointGroupLaue13::getName()
  {
      return "m-3m (Cubic)";
  }

  bool PointGroupLaue13::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,k,-l))
              || (hkl2 == V3D(h,-k,-l)) || (hkl2 == V3D(l,h,k)) || (hkl2 == V3D(l,-h,-k))
              || (hkl2 == V3D(-l,-h,k)) || (hkl2 == V3D(-l,h,-k)) || (hkl2 == V3D(k,l,h))
              || (hkl2 == V3D(-k,l,-h)) || (hkl2 == V3D(k,-l,-h)) || (hkl2 == V3D(-k,-l,h))
              || (hkl2 == V3D(k,h,-l)) || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(k,-h,l))
              || (hkl2 == V3D(-k,h,l)) || (hkl2 == V3D(h,l,-k)) || (hkl2 == V3D(-h,l,k))
              || (hkl2 == V3D(-h,-l,-k)) || (hkl2 == V3D(h,-l,k)) || (hkl2 == V3D(l,k,-h))
              || (hkl2 == V3D(l,-k,h)) || (hkl2 == V3D(-l,k,h)) || (hkl2 == V3D(-l,-k,-h))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(h,-k,l))
              || (hkl2 == V3D(-h,k,l)) || (hkl2 == V3D(-l,-h,-k)) || (hkl2 == V3D(-l,h,k))
              || (hkl2 == V3D(l,h,-k)) || (hkl2 == V3D(l,-h,k)) || (hkl2 == V3D(-k,-l,-h))
              || (hkl2 == V3D(k,-l,h)) || (hkl2 == V3D(-k,l,h)) || (hkl2 == V3D(k,l,-h))
              || (hkl2 == V3D(-k,-h,l)) || (hkl2 == V3D(k,h,l)) || (hkl2 == V3D(-k,h,-l))
              || (hkl2 == V3D(k,-h,-l)) || (hkl2 == V3D(-h,-l,k)) || (hkl2 == V3D(h,-l,-k))
              || (hkl2 == V3D(h,l,k)) || (hkl2 == V3D(-h,l,-k)) || (hkl2 == V3D(-l,-k,h))
              || (hkl2 == V3D(-l,k,-h)) || (hkl2 == V3D(l,-k,-h)) || (hkl2 == V3D(l,k,h));
  }

  /** @return a vector with all possible PointGroup objects */
  std::vector<PointGroup_sptr> getAllPointGroups()
  {
    std::vector<PointGroup_sptr> out;
    out.push_back( boost::make_shared<PointGroupLaue1>() );
    out.push_back( boost::make_shared<PointGroupLaue2>() );
    out.push_back( boost::make_shared<PointGroupLaue3>() );
    out.push_back( boost::make_shared<PointGroupLaue4>() );
    out.push_back( boost::make_shared<PointGroupLaue5>() );
    out.push_back( boost::make_shared<PointGroupLaue6>() );
    out.push_back( boost::make_shared<PointGroupLaue7>() );
    out.push_back( boost::make_shared<PointGroupLaue8>() );
    out.push_back( boost::make_shared<PointGroupLaue9>() );
    out.push_back( boost::make_shared<PointGroupLaue10>() );
    out.push_back( boost::make_shared<PointGroupLaue11>() );
    out.push_back( boost::make_shared<PointGroupLaue12>() );
    out.push_back( boost::make_shared<PointGroupLaue13>() );
    return out;
  }



} // namespace Mantid
} // namespace Geometry

