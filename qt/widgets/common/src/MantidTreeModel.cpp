#include "MantidQtWidgets/Common/MantidTreeModel.h"
#include "MantidQtWidgets/Common/MantidWSIndexDialog.h"

using namespace std;
using namespace MantidQt;
using namespace MantidWidgets;


// Data display and saving methods
void MantidTreeModel::updateRecentFilesList(const QString &fname){ throw runtime_error("Not implemented"); }
void MantidTreeModel::enableSaveNexus(const QString &wsName){ throw runtime_error("Not implemented"); }
void MantidTreeModel::disableSaveNexus(){ throw runtime_error("Not implemented"); }
void MantidTreeModel::deleteWorkspaces(const QStringList &wsNames){ throw runtime_error("Not implemented"); }
void MantidTreeModel::importWorkspace(){ throw runtime_error("Not implemented"); }
MantidMatrix *
	MantidTreeModel::importMatrixWorkspace(const Mantid::API::MatrixWorkspace_sptr workspace,
                    int lower, int upper,bool showDlg){
	throw runtime_error("Not implemented");
}

void MantidTreeModel::importWorkspace(const QString &wsName, bool showDlg,
                            bool makeVisible){
	throw runtime_error("Not implemented");
}
void MantidTreeModel::renameWorkspace(QStringList wsName){ throw runtime_error("Not implemented"); }
void MantidTreeModel::showMantidInstrumentSelected(){ throw runtime_error("Not implemented"); }
Table *MantidTreeModel::createDetectorTable(const QString &wsName,
                                    const std::vector<int> &indices,
                                    bool include_data){
	throw runtime_error("Not implemented");
}
void MantidTreeModel::importBoxDataTable(){ throw runtime_error("Not implemented"); }
void MantidTreeModel::showListData(){ throw runtime_error("Not implemented"); }
void MantidTreeModel::importTransposed(){ throw runtime_error("Not implemented"); }

// Algorithm Display and Execution Methods
Mantid::API::IAlgorithm_sptr MantidTreeModel::createAlgorithm(const QString &algName,
                                                    int version){
	throw runtime_error("Not implemented");
}
void MantidTreeModel::showAlgorithmDialog(const QString &algName, int version){ throw runtime_error("Not implemented"); }
void
	MantidTreeModel::showAlgorithmDialog(const QString &algName, QHash<QString, QString> paramList,
                    Mantid::API::AlgorithmObserver *obs,
                    int version){
	throw runtime_error("Not implemented");
}
void MantidTreeModel::executeAlgorithm(Mantid::API::IAlgorithm_sptr alg) {};
bool MantidTreeModel::executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg,
                                    const bool wait){
	throw runtime_error("Not implemented");
}

Mantid::API::Workspace_const_sptr
	MantidTreeModel::getWorkspace(const QString &workspaceName){ throw runtime_error("Not implemented"); }

QWidget *MantidTreeModel::getParent(){ throw runtime_error("Not implemented"); }

// Plotting Methods
MultiLayer *
	MantidTreeModel::plot1D(const QMultiMap<QString, std::set<int>> &toPlot, bool spectrumPlot,
        MantidQt::DistributionFlag distr,
        bool errs, MultiLayer *plotWindow,
        bool clearWindow, bool waterfallPlot,
        const QString &log,
        const std::set<double> &customLogValues){ throw runtime_error("Not implemented");}

void MantidTreeModel::drawColorFillPlots(
    const QStringList &wsNames,
    GraphOptions::CurveType curveType){ throw runtime_error("Not implemented");}

void MantidTreeModel::showMDPlot(){ throw runtime_error("Not implemented");}

MultiLayer *
	MantidTreeModel::plotSubplots(const QMultiMap<QString, std::set<int>> &toPlot,
            MantidQt::DistributionFlag distr,
            bool errs, MultiLayer *plotWindow){ throw runtime_error("Not implemented");}

void MantidTreeModel::plotSurface(bool accepted, int plotIndex,
                        const QString &axisName, const QString &logName,
                        const std::set<double> &customLogValues,
                        const QList<QString> &workspaceNames){ throw runtime_error("Not implemented");}
void MantidTreeModel::plotContour(bool accepted, int plotIndex,
                                  const QString &axisName, const QString &logName,
                                  const std::set<double> &customLogValues,
                                  const QList<QString> &workspaceNames){ throw runtime_error("Not implemented");}

// Interface Methods
void MantidTreeModel::showVatesSimpleInterface(){ throw runtime_error("Not implemented");}
void MantidTreeModel::showSpectrumViewer(){ throw runtime_error("Not implemented");}
void MantidTreeModel::showSliceViewer(){ throw runtime_error("Not implemented");}
void MantidTreeModel::showLogFileWindow(){}
void MantidTreeModel::showSampleMaterialWindow(){}
void MantidTreeModel::showAlgorithmHistory(){ throw runtime_error("Not implemented");}

MantidQt::MantidWidgets::MantidWSIndexDialog *
	MantidTreeModel::createWorkspaceIndexDialog(int flags, const QStringList &wsNames,
                            bool showWaterfall, bool showPlotAll,
                            bool showTiledOpt, bool isAdvanced){ throw runtime_error("Not implemented");}

void MantidTreeModel::updateProject(){ throw runtime_error("Not implemented"); }
void MantidTreeModel::showCritical(const QString &) { throw runtime_error("Not implemented"); }