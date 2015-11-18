#include "MantidQtCustomInterfaces/SANSBackgroundCorrectionWidget.h"
#include "MantidQtCustomInterfaces/SANSBackgroundCorrectionSettings.h"
namespace MantidQt
{
namespace CustomInterfaces
{
  SANSBackgroundCorrectionWidget::SANSBackgroundCorrectionWidget(QWidget* parent) : QWidget(parent) {
    m_ui.setupUi(this); 
  }

  /**
   * Set the dark run settings for time-based subtractions
   * @param setting: the dark run settings for time-based subtractions, ie when we want 
   */
  void SANSBackgroundCorrectionWidget::setDarkRunSettingForTime(SANSBackgroundCorrectionSettings setting) {
    m_ui.bckgnd_cor_time_det_cbox->setChecked(setting.getUseDet());
    m_ui.bckgnd_cor_time_mean_cbox->setChecked(setting.getUseMean());
    m_ui.bckgnd_cor_time_mon_cbox->setChecked(setting.getUseMon());
    m_ui.bckgnd_cor_time_run_line_edit->setText(setting.getRunNumber());
    m_ui.bckgnd_cor_time_mon_num_line_edit->setText(setting.getMonNumber());
  }

  /**
   * Get the dark run settings for time-based subtractions
   * @returns the dark run settings for time-based subtractions
   */
  SANSBackgroundCorrectionSettings SANSBackgroundCorrectionWidget::getDarkRunSettingForTime() {
    auto useDet = m_ui.bckgnd_cor_time_det_cbox->isChecked();
    auto useMean = m_ui.bckgnd_cor_time_mean_cbox->isChecked();
    auto useMon = m_ui.bckgnd_cor_time_mon_cbox->isChecked();
    auto run = m_ui.bckgnd_cor_time_run_line_edit->text();
    auto monNumbers = m_ui.bckgnd_cor_time_mon_num_line_edit->text();

    return SANSBackgroundCorrectionSettings(run, useMean, useDet, useMon, monNumbers);
  }

  /**
  * Set the dark run settings for uamp-based subtractions
  * @param setting: the dark run settings for uamp-based subtractions, ie when we want
  */
  void SANSBackgroundCorrectionWidget::setDarkRunForUamp(SANSBackgroundCorrectionSettings setting) {
    m_ui.bckgnd_cor_uamp_det_cbox->setChecked(setting.getUseDet());
    m_ui.bckgnd_cor_uamp_mon_cbox->setChecked(setting.getUseMon());
    m_ui.bckgnd_cor_uamp_run_line_edit->setText(setting.getRunNumber());
    m_ui.bckgnd_cor_uamp_mon_num_line_edit->setText(setting.getMonNumber());
  }

  /**
  * Get the dark run settings for uamp-based subtractions
  * @returns the dark run settings for uamp-based subtractions
  */
  SANSBackgroundCorrectionSettings SANSBackgroundCorrectionWidget::getDarkRunSettingForUamp() {
    auto useDet = m_ui.bckgnd_cor_uamp_det_cbox->isChecked();
    auto useMon = m_ui.bckgnd_cor_uamp_mon_cbox->isChecked();
    auto run = m_ui.bckgnd_cor_uamp_run_line_edit->text();
    auto monNumbers = m_ui.bckgnd_cor_uamp_mon_num_line_edit->text();
    auto useMean = false;
    return SANSBackgroundCorrectionSettings(run, useMean, useDet, useMon, monNumbers);
  }
}
}