#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_

//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/ui_indirectAnalysis.h"
#include "MantidQtAPI/UserSubWindow.h"

#include "MantidAPI/CompositeFunction.h"

#include <QIntValidator>
#include <QDoubleValidator>

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class indirectAnalysis : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      /// The name of the interface as registered into the factory
      static std::string name() { return "Indirect Data Analysis"; }

    public:
      /// Default Constructor
      indirectAnalysis(QWidget *parent = 0);

    protected:
      virtual void closeEvent(QCloseEvent* close);

    private:
      /// Initialize the layout
      virtual void initLayout();
      /// init python-dependent sections
      virtual void initLocalPython();

      void loadSettings();
      void saveSettings();

      void setupTreePropertyBrowser();

      bool validateFury();
      bool validateElwin();
      bool validateMsd();
      bool validateAbsorption();
      bool validateDemon();

      Mantid::API::CompositeFunction* createFunction();

      void addLorentz();
      void addStressed();

    private slots:

      void instrumentChanged(int index);
      void analyserSelected(int index);
      void reflectionSelected(int index);
      void furyRun();
      void furyResType(const QString& type);
      void furyPlotInput();

      void runFuryFit();
      void furyfit_typeSelection(int index);

      void elwinRun();
      void elwinPlotInput();
      void elwinTwoRanges(bool state);
      void msdRun();
      void msdPlotInput();
      void absorptionRun();
      void absorptionShape(int index);
      void demonRun();

      void openDirectoryDialog();
      void help();

    private:
      Ui::indirectAnalysis m_uiForm;

      QString m_settingsGroup;
      
      QString m_dataDir;
      QString m_saveDir;

      QIntValidator *m_valInt;
      QDoubleValidator *m_valDbl;

      QtTreePropertyBrowser* m_propBrowser;

      QtGroupPropertyManager* m_groupManager;
      QtDoublePropertyManager* m_doubleManager;
      // QtStringPropertyManager* m_stringManager;
      // QtEnumPropertyManager* m_enumManager;
      // QtIntPropertyManager* m_intManager;
      // QtBoolPropertyManager* m_boolManager;
      // QtStringPropertyManager* m_filenameManager;

      bool m_furyResFileType;

      // QList<QPair<QString,double> > m_baseProperties;
      QMap<QString, double> m_baseProperties;

    };
  }
}
#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_