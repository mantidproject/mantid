#ifndef MDFDATACONTROLLER_H_
#define MDFDATACONTROLLER_H_

#include <QObject>

// Forward declaration
class QTableWidget;

namespace MantidQt
{

namespace CustomInterfaces
{

// Forward declaration
class MultiDatasetFit;

namespace MDF
{

/**
  * A class for controlling a table widget containing a list of
  * data sources for the fit.
  *
  * Each data source is described by:
  *  - workspace name;
  *  - workspace index;
  *  - start of the fitting range;
  *  - end of the fitting range;
  *
  * This controller has a pointer to the table widget and controls
  * its behaviour but not its position on the parent widget.
  */
class DataController: public QObject
{
  Q_OBJECT
public:
  DataController(MultiDatasetFit *parent, QTableWidget *dataTable);
  std::string getWorkspaceName(int i) const;
  int getWorkspaceIndex(int i) const;
  int getNumberOfSpectra() const;
  void checkSpectra();
  std::pair<double,double> getFittingRange(int i) const;

signals:
  void dataTableUpdated();
  void dataSetUpdated(int i);
  void hasSelection(bool);
  void spectraRemoved(QList<int>);
  void spectraAdded(int n);

public slots:
  void setFittingRangeGlobal(bool);
  void setFittingRange(int, double, double);

private slots:
  void addWorkspace();
  void workspaceSelectionChanged();
  void removeSelectedSpectra();
  void updateDataset(int, int);

private:
  MultiDatasetFit *owner() const;
  void addWorkspaceSpectrum(const QString &wsName, int wsIndex, const Mantid::API::MatrixWorkspace& ws);
  void removeSpectra(QList<int> rows);

  /// Table with data set names and other data.
  QTableWidget *m_dataTable;
  /// Flag for setting the fitting range.
  bool m_isFittingRangeGlobal;
};

} // MDF
} // CustomInterfaces
} // MantidQt


#endif /*MDFDATACONTROLLER_H_*/
