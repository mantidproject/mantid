#ifndef WORKSPACEMATRIXMODEL_H
#define WORKSPACEMATRIXMODEL_H

#include <QMessageBox>
#include "../MatrixModel.h"
#include "../Graph.h"

#include "MantidAPI/Workspace.h"

class WorkspaceMatrixModel : public MatrixModel
{
	Q_OBJECT
	
public:

    WorkspaceMatrixModel(Mantid::API::Workspace_sptr& ws, QObject *parent, int start=-1, int end=-1, bool filter=false, double maxv=0.);
    //~WorkspaceMatrixModel();

    void setGraph2D(Graph* g);
    void setGraph1D(Graph* g);
    int startRow()const{return m_start;}
    int endRow()const{return m_end;}

    bool canResize(int rows, int cols){return false;}
    void setDimensions(int rows, int cols){}

    //int rowCount(const QModelIndex &parent = QModelIndex()) const;
    void setRowCount(int rows){}

    //int columnCount(const QModelIndex &parent = QModelIndex()) const;
    void setColumnCount(int cols){}

    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()){return true;}
	bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()){return true;}

	bool removeColumns(int column, int count, const QModelIndex & parent = QModelIndex()){return true;}
	bool insertColumns(int column, int count, const QModelIndex & parent = QModelIndex()){return true;}

	//double x(int col) const;
	//double y(int row) const;

	double cell(int row, int col) const;
	void setCell(int row, int col, double val){}

    //QString text(int row, int col){return "0o0";}
    void setText(int row, int col, const QString&){}

	//QString saveToString();
	//QImage renderImage();

	//double data(int row, int col) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex & index, const QVariant & value, int role){return true;}

    double* dataVector(){return 0;}//{return d_data;}
    //QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void setImage(const QImage& image){}

	//
    //bool importASCII(const QString &fname, const QString &sep, int ignoredLines, bool stripSpaces,
	//				bool simplifySpaces, const QString& commentString, int importAs,
	//				const QLocale& locale, int endLineChar = 0, int maxRows = -1);

	//void setLocale(const QLocale& locale){d_locale = locale;};
	//void setNumericFormat(char f, int prec);

	bool initWorkspace(){return true;}
    void invert(){}
    void transpose(){}
    void flipVertically(){}
    void flipHorizontally(){}
    void rotate90(bool clockwise){}
    void fft(bool inverse){}
    void clear(int startRow = 0, int endRow = -1, int startCol = 0, int endCol = -1){}
	bool calculate(int startRow = 0, int endRow = -1, int startCol = 0, int endCol = -1){return true;}
	bool muParserCalculate(int startRow = 0, int endRow = -1, int startCol = 0, int endCol = -1){return true;}
	double* dataCopy(int startRow = 0, int endRow = -1, int startCol = 0, int endCol = -1){return 0;}
    void pasteData(double *clipboardBuffer, int topRow, int leftCol, int rows, int cols){}

    // New functions
	double dataX(int row, int col) const;
	double dataE(int row, int col) const;
    int indexX(double s)const;

private:

    Mantid::API::Workspace_sptr m_workspace;
    int m_start;
    int m_end;
    bool m_filter;
    double m_maxValue;

};

#endif /* WORKSPACEMATRIXMODEL_H */
