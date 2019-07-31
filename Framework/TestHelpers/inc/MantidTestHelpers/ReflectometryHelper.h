// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_TESTHELPERS_REFLECTOMETRYHELPER_H_
#define MANTID_TESTHELPERS_REFLECTOMETRYHELPER_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

#include <vector>

namespace Mantid {

using namespace API;

namespace TestHelpers {

// Create a workspace equipped with fake instrument from
// unit_testing/REFL_Definition.xml IDF.
// @param nBins :: Number of bins in each spectrum.
// @param startX :: Start value of the x-axis in TOF
// @param endX :: End value of the x-axis in TOF
// @param values :: Y-values. The created workspace will have as many spectra as
// there are values -
//     one value per spectrum.
// @param paramsType :: Defines which instrument parameters file to load.
// paramsType is appended to
//     "REFL_Parameters_" to form the name for the file to load.
MatrixWorkspace_sptr createREFL_WS(size_t nBins, double startX, double endX,
                                   std::vector<double> const &values,
                                   std::string const &paramsType = "",
                                   std::string const &instrumentSuffix = "");

// Create a group of workspaces created with createREFL_WS(...) function and
// store it in the ADS.
// Ys get some hard-coded values.
// @param name :: Group's name in the ADS.
// @param size :: Size of the group.
void prepareInputGroup(std::string const &name,
                       std::string const &paramsType = "", size_t size = 4,
                       double const startX = 5000.0,
                       double const endX = 100000.0, size_t const nBins = 10);

// Retrieve wprkspace group with name name, cast its items to MatrixWorkspace
// and return a vector of those.
std::vector<MatrixWorkspace_sptr> retrieveOutWS(std::string const &name);

// Apply some polarization efficiencies (Fredrikze) to a workspace such that
// when it's run
// through the polarization correction algorithm it gets restored to the
// original.
void applyPolarizationEfficiencies(std::string const &name);

MatrixWorkspace_sptr createWorkspaceSingle(const double startX, const int nBins,
                                           const double deltaX,
                                           const std::vector<double> &yValues);

MatrixWorkspace_sptr createWorkspaceSingle(const double startX = 1,
                                           const int nBins = 3,
                                           const double deltaX = 1);
} // namespace TestHelpers
} // namespace Mantid

#endif // MANTID_TESTHELPERS_REFLECTOMETRYHELPER_H_
