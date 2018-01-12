#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/MantidTreeModel.h"
#include "MantidQtWidgets/Common/MantidWSIndexDialog.h"
#include <Poco/ActiveResult.h>
#include <qcoreapplication.h>
#include <QMessageBox>

using namespace std;
using namespace MantidQt;
using namespace MantidWidgets;
using namespace Mantid::API;

MantidTreeModel::MantidTreeModel() {
}

// Data display and saving methods
void MantidTreeModel::updateRecentFilesList(const QString &fname){ /*Not require until tool bar is created*/ }
void MantidTreeModel::enableSaveNexus(const QString &wsName) { /*handled by widget*/ }
void MantidTreeModel::disableSaveNexus(){ /* handled by widget*/ }
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
	Mantid::API::IAlgorithm_sptr alg;
	try {
		alg = Mantid::API::AlgorithmManager::Instance().create(
			algName.toStdString(), version);
	}
	catch (...) {
		QString message = "Cannot create algorithm \"" + algName + "\"";
		if (version != -1) {
			message += " version " + QString::number(version);
		}
		QMessageBox::warning(nullptr, "MantidPlot", message);
		alg = Mantid::API::IAlgorithm_sptr();
	}
	return alg;
}
void MantidTreeModel::showAlgorithmDialog(const QString &algName, int version){
	// Check if Alg is valid
	Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm(algName, version);
	if (!alg)
		return;
	MantidQt::API::InterfaceManager interfaceManager;
	MantidQt::API::AlgorithmDialog *dlg = interfaceManager.createDialog(
		alg, nullptr, false);
	dlg->show();
	dlg->raise();
	dlg->activateWindow();
}

void
	MantidTreeModel::showAlgorithmDialog(const QString &algName, QHash<QString, QString> paramList,
                    Mantid::API::AlgorithmObserver *obs,
                    int version){
	throw runtime_error("Not implemented");
}

void MantidTreeModel::executeAlgorithm(Mantid::API::IAlgorithm_sptr alg) {
	executeAlgorithmAsync(alg);
}

bool MantidTreeModel::executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg,
                                    const bool wait) {throw runtime_error("Not implemented");}

Workspace_const_sptr
	MantidTreeModel::getWorkspace(const QString &workspaceName){
	if (AnalysisDataService::Instance().doesExist(workspaceName.toStdString())) {
		return AnalysisDataService::Instance().retrieve(
			workspaceName.toStdString());
	}
	Workspace_const_sptr empty;
	return empty; //??
}

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

void MantidTreeModel::updateProject() { /*Currently unrequired in Workbench*/ }
void MantidTreeModel::showCritical(const QString &) { throw runtime_error("Not implemented"); }