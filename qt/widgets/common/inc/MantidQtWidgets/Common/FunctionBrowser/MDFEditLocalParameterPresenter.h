#ifndef MDFEDITLOCALPARAMETERPRESENTER_H_
#define MDFEDITLOCALPARAMETERPRESENTER_H_

#include "MDFEditLocalParameterDialog.h"
#include "MDFEditLocalParameterModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace MDF {

class EditLocalParameterPresenter : public EditLocalParameterDialogSubscriber {
public:
  EditLocalParameterPresenter(EditLocalParameterModel model,
                              QWidget *dialogParent);

  bool executeDialog(MantidWidgets::MultiDomainFunctionModel &modelToUpdate);

  void setParameters(double value) override;
  void setFixed(bool fixed) override;
  void setTies(std::string const &tie) override;
  void setParameter(double value, int index) override;
  void fixParameter(bool fixed, int index) override;
  void setTie(std::string const &tie, int index) override;
  void copyValuesToClipboard() override;
  void pasteValuesFromClipboard(std::string const &text) override;
  void setValuesToLog(std::string const &logName,
                      std::string const &function) override;
  void setValueToLog(std::string const &logName, std::string const &function,
                     int index) override;

private:
  void updateDialogParameterValues();
  void updateDialogParameterRoles();
  void updateDialogParameterRole(int index);

  QENS::EditLocalParameterDialog m_dialog;
  EditLocalParameterModel m_model;
};

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
