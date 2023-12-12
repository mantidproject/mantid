// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/IAlgorithm_fwd.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"

/* Forward declarations */

class QObject;

namespace MantidQt::MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON AlgorithmFinishObserver : public QObject, public Mantid::API::AlgorithmObserver {
  Q_OBJECT
signals:
  /// Emitted when alg completes
  void algCompletedSignal();

protected:
  void finishHandle(const Mantid::API::IAlgorithm *alg) override {
    UNUSED_ARG(alg);
    emit algCompletedSignal();
  }
};

class EXPORT_OPT_MANTIDQT_COMMON FindPeakStrategyGeneric {
public:
  virtual void initialise(const std::string &wsName, const int workspaceIndex, const std::string &peakListName,
                          const int FWHM, AlgorithmFinishObserver *obs) = 0;
  virtual void execute() = 0;
  virtual size_t peakNumber() const = 0;
  virtual double getPeakCentre(const size_t peakIndex) const = 0;
  virtual double getPeakHeight(const size_t peakIndex) const = 0;
  virtual double getPeakWidth(const size_t peakIndex) const = 0;
  virtual ~FindPeakStrategyGeneric(){};
};

template <typename T> class EXPORT_OPT_MANTIDQT_COMMON FindPeakStrategy : public FindPeakStrategyGeneric {
public:
  size_t peakNumber() const override { return m_peakCentres->size(); };
  double getPeakCentre(const size_t peakIndex) const override { return m_peakCentres->operator[](peakIndex); };
  double getPeakHeight(const size_t peakIndex) const override { return m_peakHeights->operator[](peakIndex); };
  double getPeakWidth(const size_t peakIndex) const override { return m_peakWidths->operator[](peakIndex); };

protected:
  std::string m_peakListName;
  std::unique_ptr<T> m_peakCentres;
  std::unique_ptr<T> m_peakHeights;
  std::unique_ptr<T> m_peakWidths;
};

class EXPORT_OPT_MANTIDQT_COMMON FindPeakConvolveStrategy : public QObject,
                                                            public FindPeakStrategy<std::vector<double>> {
  Q_OBJECT
public:
  void initialise(const std::string &wsName, const int workspaceIndex, const std::string &peakListName, const int FWHM,
                  AlgorithmFinishObserver *obs) override;
  void execute() override;

private:
  int m_FWHM;
  AlgorithmFinishObserver *m_obs;        // Non-owning
  MantidQt::API::AlgorithmDialog *m_dlg; // Non-owning
};

class EXPORT_OPT_MANTIDQT_COMMON FindPeakDefaultStrategy : public FindPeakStrategy<Mantid::API::ColumnVector<double>> {
public:
  void initialise(const std::string &wsName, const int workspaceIndex, const std::string &peakListName, const int FWHM,
                  AlgorithmFinishObserver *obs) override;
  void execute() override;

private:
  Mantid::API::IAlgorithm_sptr m_alg;
};

} // namespace MantidQt::MantidWidgets