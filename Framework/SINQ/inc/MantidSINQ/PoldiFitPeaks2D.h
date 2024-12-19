// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidSINQ/DllConfig.h"

#include "MantidKernel/Matrix.h"

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidSINQ/PoldiUtilities/Poldi2DFunction.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidSINQ/PoldiUtilities/PoldiTimeTransformer.h"

namespace Mantid {
namespace Poldi {

/** PoldiFitPeaks2D

    An Algorithm to fit a POLDI 2D-spectrum from a given table containing POLDI
    peak data. A MatrixWorkspace containing a proper POLDI instrument definition
   is required
    to determine output workspace dimensions etc.

    In order to use the algorithm for calculating a theoretical spectrum,
    the MaximumIterations property can be set to 0.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 16/05/2014
  */

class MANTID_SINQ_DLL PoldiFitPeaks2D : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"PoldiFitPeaks1D"}; }
  const std::string category() const override;

  const std::string summary() const override;

  std::map<std::string, std::string> validateInputs() override;

  bool checkGroups() override { return false; }

protected:
  // Workspace handling
  std::vector<PoldiPeakCollection_sptr> getPeakCollectionsFromInput() const;

  PoldiPeakCollection_sptr getPeakCollection(const DataObjects::TableWorkspace_sptr &peakTable) const;

  // Peak integration and transformations
  std::vector<PoldiPeakCollection_sptr>
  getNormalizedPeakCollections(const std::vector<PoldiPeakCollection_sptr> &peakCollections) const;

  PoldiPeakCollection_sptr getIntegratedPeakCollection(const PoldiPeakCollection_sptr &rawPeakCollection) const;
  PoldiPeakCollection_sptr getNormalizedPeakCollection(const PoldiPeakCollection_sptr &peakCollection) const;

  std::vector<PoldiPeakCollection_sptr> getCountPeakCollections(const API::IFunction_sptr &fitFunction);
  PoldiPeakCollection_sptr getCountPeakCollection(const PoldiPeakCollection_sptr &peakCollection) const;

  // Conversion between peaks and functions
  PoldiPeak_sptr getPeakFromPeakFunction(const API::IPeakFunction_sptr &profileFunction, const Kernel::V3D &hkl);

  // Conversion between peak collections and functions
  Poldi2DFunction_sptr getFunctionFromPeakCollection(const PoldiPeakCollection_sptr &peakCollection);

  Poldi2DFunction_sptr getFunctionIndividualPeaks(const std::string &profileFunctionName,
                                                  const PoldiPeakCollection_sptr &peakCollection) const;

  Poldi2DFunction_sptr getFunctionPawley(const std::string &profileFunctionName,
                                         const PoldiPeakCollection_sptr &peakCollection);

  std::string getLatticeSystemFromPointGroup(const Geometry::PointGroup_sptr &pointGroup) const;

  std::string getRefinedStartingCell(const std::string &initialCell, const std::string &latticeSystem,
                                     const PoldiPeakCollection_sptr &peakCollection);

  std::string getUserSpecifiedTies(const API::IFunction_sptr &poldiFn);

  std::string getUserSpecifiedBounds(const API::IFunction_sptr &poldiFn);

  PoldiPeakCollection_sptr getPeakCollectionFromFunction(const API::IFunction_sptr &fitFunction);

  void assignMillerIndices(const PoldiPeakCollection_sptr &from, const PoldiPeakCollection_sptr &to) const;

  void assignCrystalData(PoldiPeakCollection_sptr &normalizedPeakCollection,
                         const PoldiPeakCollection_sptr &peakCollection) const;

  // Extraction of 1D spectrum and cell
  API::MatrixWorkspace_sptr get1DSpectrum(const API::IFunction_sptr &fitFunction,
                                          const API::MatrixWorkspace_sptr &workspace) const;

  API::MatrixWorkspace_sptr getQSpectrum(const API::FunctionDomain1D &domain, const API::FunctionValues &values) const;

  API::ITableWorkspace_sptr getRefinedCellParameters(const API::IFunction_sptr &fitFunction) const;

  // Interacting with Fit
  API::IAlgorithm_sptr calculateSpectrum(const std::vector<PoldiPeakCollection_sptr> &peakCollections,
                                         const API::MatrixWorkspace_sptr &matrixWorkspace);

  API::MatrixWorkspace_sptr getWorkspace(const API::IAlgorithm_sptr &fitAlgorithm) const;
  API::IFunction_sptr getFunction(const API::IAlgorithm_sptr &fitAlgorithm) const;

  void addBackgroundTerms(const Poldi2DFunction_sptr &poldi2DFunction) const;

  std::shared_ptr<Kernel::DblMatrix>
  getLocalCovarianceMatrix(const std::shared_ptr<const Kernel::DblMatrix> &covarianceMatrix, size_t parameterOffset,
                           size_t nParams) const;

  // Poldi instrument book-keeping
  void setPoldiInstrument(const PoldiInstrumentAdapter_sptr &instrument);
  void setTimeTransformerFromInstrument(const PoldiInstrumentAdapter_sptr &poldiInstrument);
  void setTimeTransformer(const PoldiTimeTransformer_sptr &poldiTimeTransformer);

  void setDeltaTFromWorkspace(const API::MatrixWorkspace_sptr &matrixWorkspace);
  void setDeltaT(double newDeltaT);
  bool isValidDeltaT(double deltaT) const;

  PoldiInstrumentAdapter_sptr m_poldiInstrument;
  PoldiTimeTransformer_sptr m_timeTransformer;
  double m_deltaT{0.0};

private:
  void init() override;
  void exec() override;
};

} // namespace Poldi
} // namespace Mantid
