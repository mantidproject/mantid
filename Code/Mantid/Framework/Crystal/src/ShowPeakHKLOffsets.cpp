#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCrystal/ShowPeakHKLOffsets.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using Mantid::DataObjects::PeaksWorkspace;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using Mantid::DataObjects::TableWorkspace;
using Mantid::DataObjects::PeaksWorkspace_sptr;
using Mantid::DataObjects::TableWorkspace_sptr;
using Mantid::DataObjects::Peak;
using Mantid::Kernel::V3D;
using Mantid::API::WorkspaceFactory;
using Mantid::API::ITableWorkspace;
/*
 * ShowPeakHKLOffsets.cpp *
 *  Created on: May 13, 2013
 *      Author: ruth
 */
namespace Mantid {

namespace Crystal {

DECLARE_ALGORITHM(ShowPeakHKLOffsets)

ShowPeakHKLOffsets::ShowPeakHKLOffsets() : Algorithm() {}
ShowPeakHKLOffsets::~ShowPeakHKLOffsets() {}

void ShowPeakHKLOffsets::init() {
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace", "",
                                                        Direction::Input),
                  "Workspace of Peaks with UB loaded");

  declareProperty(new WorkspaceProperty<ITableWorkspace>("HKLIntegerOffsets",
                                                        "HKLIntegerOffsets",
                                                        Direction::Output),
                  "Workspace with the Results");
}

void ShowPeakHKLOffsets::exec() {
  PeaksWorkspace_sptr Peaks = getProperty(std::string("PeaksWorkspace"));

  if (!Peaks) {
    g_log.error(" Invalid peaks workspace ");
    throw std::invalid_argument(" Invalid peaks workspace ");
  }

  if (Peaks->getNumberPeaks() < 1) {
    g_log.error("The peaks workspace has NO peaks");
    throw std::invalid_argument("The peaks workspace has NO peaks");
  }

  if (!Peaks->sample().hasOrientedLattice()) {
    g_log.error("The peaks workspace does not have an oriented lattice");
    throw std::invalid_argument(
        "The peaks workspace does not have an oriented lattice");
  }

  Kernel::Matrix<double> UBinv =
      Peaks->mutableSample().getOrientedLattice().getUB();
  UBinv.Invert();
  UBinv /= 2 * M_PI;

  // TableWorkspace_sptr Res( new TableWorkspace());
  boost::shared_ptr<ITableWorkspace> Res =
      WorkspaceFactory::Instance().createTable("TableWorkspace");
  Res->setTitle("HKL int offsets for " + Peaks->getName());

  Res->addColumn("double", "H offset from int");
  Res->addColumn("double", "K offset from int");
  Res->addColumn("double", "L offset from int");
  Res->addColumn("double", "Max offset from int");

  Res->addColumn("int", "bank");
  Res->addColumn("int", "RunNumber");

  for (int i = 0; i < Peaks->getNumberPeaks(); i++) {
    Res->appendRow();
    Peak peak = Peaks->getPeak(i);
    V3D hkl = UBinv * peak.getQSampleFrame();
    double maxOffset = 0;
    for (int j = 0; j < 3; j++) {
      double offset = hkl[j] - floor(hkl[j]);
      if (offset > .5)
        offset -= 1;
      Res->Double(i, j) = offset;
      if (fabs(offset) > fabs(maxOffset))
        maxOffset = offset;
    }
    Res->Double(i, 3) = maxOffset;
    Res->Int(i, 5) = peak.getRunNumber();
    std::string bankName = peak.getBankName();
    size_t k = bankName.find_last_not_of("0123456789");
    int bank = 0;
    if (k < bankName.length())
      bank = boost::lexical_cast<int>(bankName.substr(k + 1));
    Res->Int(i, 4) = bank;
  }

  setProperty("HKLIntegerOffsets", Res);
}

} // namespace Crystal
} // namespace Mantid
