#ifndef MANTIDCUSTOMINTERFACES_APPROACH_DIALOG_H  
#define MANTIDCUSTOMINTERFACES_APPROACH_DIALOG_H  

#include <QDialog>  

class QComboBox;
namespace MantidQt
{
  namespace CustomInterfaces
  {
    enum ApproachType{ISISInelastic, ISISSingleCrystalDiff}; // Extend 

    /// Dialog allows an approach to be specified.
    class ApproachDialog : public QDialog  
    {  
      Q_OBJECT  
    public:  
      ApproachDialog();  
      ~ApproachDialog(); 
      ApproachType getApproach() const;
      bool getWasAborted() const;

    private:

      QComboBox* m_approaches;

      bool m_aborted;

      private slots:
        void approachChanged();
        void ok();
        void cancel();

    };  
  }
}
#endif // MANTIDCUSTOMINTERFACES_APPROACH_DIALOG_H 