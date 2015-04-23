#ifndef MANTID_SINQ_POLDICALCULATESPECTRUM2D_H_
#define MANTID_SINQ_POLDICALCULATESPECTRUM2D_H_

#include "MantidKernel/System.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IPeakFunction.h"

#include "MantidKernel/Matrix.h"

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidSINQ/PoldiUtilities/PoldiTimeTransformer.h"
#include "MantidSINQ/PoldiUtilities/Poldi2DFunction.h"

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

class MANTID_SINQ_DLL PoldiFitPeaks2D : public API::Algorithm {
public:
  PoldiFitPeaks2D();
  virtual ~PoldiFitPeaks2D();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;

  virtual const std::string summary() const;

  std::map<std::string, std::string> validateInputs();

  bool checkGroups() { return false; }

  boost::shared_ptr<Kernel::DblMatrix> getLocalCovarianceMatrix(
      const boost::shared_ptr<const Kernel::DblMatrix> &covarianceMatrix,
      size_t parameterOffset, size_t nParams) const;

  std::vector<PoldiPeakCollection_sptr> getPeakCollectionsFromInput() const;
protected:
  Poldi2DFunction_sptr getFunctionIndividualPeaks(
      std::string profileFunctionName,
      const PoldiPeakCollection_sptr &peakCollection) const;

  Poldi2DFunction_sptr
  getFunctionPawley(std::string profileFunctionName,
                    const PoldiPeakCollection_sptr &peakCollection);

  std::string
  getRefinedStartingCell(const std::string &initialCell,
                         const std::string &crystalSystem,
                         const PoldiPeakCollection_sptr &peakCollection);

  PoldiPeak_sptr
  getPeakFromPeakFunction(API::IPeakFunction_sptr profileFunction,
                          const Kernel::V3D &hkl);

  API::ITableWorkspace_sptr
  getRefinedCellParameters(const API::IFunction_sptr &fitFunction) const;

  PoldiPeakCollection_sptr
  getPeakCollection(const DataObjects::TableWorkspace_sptr &peakTable) const;
  PoldiPeakCollection_sptr getIntegratedPeakCollection(
      const PoldiPeakCollection_sptr &rawPeakCollection) const;
  PoldiPeakCollection_sptr getNormalizedPeakCollection(
      const PoldiPeakCollection_sptr &peakCollection) const;
  PoldiPeakCollection_sptr
  getCountPeakCollection(const PoldiPeakCollection_sptr &peakCollection) const;

  void assignMillerIndices(const PoldiPeakCollection_sptr &from,
                           PoldiPeakCollection_sptr &to) const;

  PoldiPeakCollection_sptr
  getPeakCollectionFromFunction(const API::IFunction_sptr &fitFunction);
  Poldi2DFunction_sptr
  getFunctionFromPeakCollection(const PoldiPeakCollection_sptr &peakCollection);
  void addBackgroundTerms(Poldi2DFunction_sptr poldi2DFunction) const;

  API::IAlgorithm_sptr
  calculateSpectrum(const PoldiPeakCollection_sptr &peakCollection,
                    const API::MatrixWorkspace_sptr &matrixWorkspace);
  API::MatrixWorkspace_sptr
  getWorkspace(const API::IAlgorithm_sptr &fitAlgorithm) const;
  API::IFunction_sptr
  getFunction(const API::IAlgorithm_sptr &fitAlgorithm) const;

  API::MatrixWorkspace_sptr
  get1DSpectrum(const API::IFunction_sptr &fitFunction,
                const API::MatrixWorkspace_sptr &workspace) const;

  API::MatrixWorkspace_sptr
  getQSpectrum(const API::FunctionDomain1D &domain,
               const API::FunctionValues &values) const;

  void setPoldiInstrument(const PoldiInstrumentAdapter_sptr &instrument);
  void setTimeTransformerFromInstrument(
      const PoldiInstrumentAdapter_sptr &poldiInstrument);
  void
  setTimeTransformer(const PoldiTimeTransformer_sptr &poldiTimeTransformer);

  void setDeltaTFromWorkspace(const API::MatrixWorkspace_sptr &matrixWorkspace);
  void setDeltaT(double newDeltaT);
  bool isValidDeltaT(double deltaT) const;

  void assignCrystalData(PoldiPeakCollection_sptr &normalizedPeakCollection,
                         const PoldiPeakCollection_sptr &peakCollection) const;

  PoldiInstrumentAdapter_sptr m_poldiInstrument;
  PoldiTimeTransformer_sptr m_timeTransformer;
  double m_deltaT;

private:
  void init();
  void exec();
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDICALCULATESPECTRUM2D_H_ */
