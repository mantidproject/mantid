#ifndef MANTIDQTCUSTOMINTERFACES_EXCITATIONSDIAGRESULTS_H_
#define MANTIDQTCUSTOMINTERFACES_EXCITATIONSDIAGRESULTS_H_

#include "MantidQtAPI/MantidDialog.h"
#include "WidgetDllOption.h"
#include <QHash>
#include <QSignalMapper>
#include <QGridLayout>

namespace MantidQt
{
  namespace MantidWidgets
  {

    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DiagResults : public API::MantidDialog
    {
      Q_OBJECT

    public:
      DiagResults(QWidget *parent);
      void updateResults(const QString & testSummary);
      void showButtons(bool show=true);

    signals:
      /// is emitted just before the window dies to let the window that created this know the pointer it has is invalid
      void died();

    private:
      void updateRow(int row, QString text);
      int addRow(QString firstColumn, QString secondColumn);
      void addButtons(int row);
      void setupButtons(int row, QString test);
      void closeEvent(QCloseEvent *event);
    private slots:
      void tableList(int row);
      void instruView(int row);

    private:
      /// the layout that widgets are added to
      QGridLayout *m_Grid;
      /// points to the slot that deals with list buttons being pressed
      QSignalMapper *m_ListMapper;
      /// points to the slot that deals with view buttons being pressed
      QSignalMapper *m_ViewMapper;
      /// stores the name of the workspaces that contains the results of each test
      QHash<int,QString> m_diagWS;
    };
  }
}

#endif //MANTIDQTCUSTOMINTERFACES_EXCITATIONSDIAGRESULTS_H_
