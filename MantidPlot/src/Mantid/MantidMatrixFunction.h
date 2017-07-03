#ifndef MANTIDMATRIXFUNCTION_H
#define MANTIDMATRIXFUNCTION_H

#include "../UserFunction.h"
#include "MantidMatrix.h"

#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidHistogramData/HistogramX.h"

#include <QPointer>
#include <vector>

/* Forward declaration */
class MantidMatrixFunction;

/**
 * A helper class listening to ADS notifications. Helps to avoid issues with
 *multiple inheritance
 * and QObject.
 *
 */
class MantidMatrixFunctionWorkspaceObserver
    : public QObject,
      public MantidQt::API::WorkspaceObserver {
  Q_OBJECT
public:
  explicit MantidMatrixFunctionWorkspaceObserver(MantidMatrixFunction *);

signals:
  void requestRedraw();
  void requestClose();

private:
  /* Base class virtual methods */

  void afterReplaceHandle(
      const std::string &wsName,
      const boost::shared_ptr<Mantid::API::Workspace> ws) override;
  void
  preDeleteHandle(const std::string &wsName,
                  const boost::shared_ptr<Mantid::API::Workspace>) override;
  void clearADSHandle() override;

  MantidMatrixFunction *m_function;
};

/**
 * This class helps displaying a MantidMatrix in a 2D or 3D graph.
 */
class MantidMatrixFunction : public Function2D {
public:
  explicit MantidMatrixFunction(MantidMatrix &matrix);
  ~MantidMatrixFunction() override;

  /* Base class virtual methods */

  double operator()(double x, double y) override;
  double getMinPositiveValue() const override;
  QString saveToString() const override;
  void connectToViewer(QObject *viewer) override;

  /* Public methods */

  /// Value at a mesh node
  double value(size_t row, size_t col) const;
  /// Return in ymin and ymax the inetrval the row takes on the y axis
  void getRowYRange(size_t row, double &ymin, double &ymax) const;
  /// Return in xmin and xmax the inetrval the cell takes on the x axis
  void getRowXRange(int row, double &xmin, double &xmax) const;
  const Mantid::HistogramData::HistogramX &getHistogramX(int row) const;

private:
  /* Private methods */

  void init(const Mantid::API::MatrixWorkspace_const_sptr &workspace);
  void reset(const Mantid::API::MatrixWorkspace_const_sptr &workspace);
  size_t indexX(size_t row, double xValue) const;
  size_t indexY(double s) const;

  /* Data */

  Mantid::API::MatrixWorkspace_const_sptr m_workspace;
  double m_outside;
  QPointer<MantidMatrixFunctionWorkspaceObserver> m_observer;

  friend class MantidMatrixFunctionWorkspaceObserver;
};

#endif // MANTIDMATRIXFUNCTION_H
