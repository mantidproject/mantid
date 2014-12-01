#include "MantidQtCustomInterfaces/QtReflOptionsDialog.h"
#include "MantidQtCustomInterfaces/QtReflMainView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** Constructor */
    QtReflOptionsDialog::QtReflOptionsDialog(ReflMainView* view, boost::shared_ptr<IReflPresenter> presenter) :
      QDialog(dynamic_cast<QtReflMainView*>(view)),
      m_presenter(presenter)
    {
      initLayout();
      initBindings();
      loadOptions();
    }

    /** Destructor */
    QtReflOptionsDialog::~QtReflOptionsDialog()
    {
    }

    /** Initialise the ui */
    void QtReflOptionsDialog::initLayout()
    {
      ui.setupUi(this);
      connect(ui.buttonBox->button(QDialogButtonBox::Ok),    SIGNAL(clicked()), this, SLOT(saveOptions()));
    }

    /** Bind options to their widgets */
    void QtReflOptionsDialog::initBindings()
    {
      m_bindings.clear();

      //Check all the widgets for the "reflOptionName" property.
      //If it exists, bind the named option to that widget.
      QList<QWidget*> widgets = findChildren<QWidget*>();
      for(auto it = widgets.begin(); it != widgets.end(); ++it)
      {
        QVariant binding = (*it)->property("reflOptionName");
        if(binding.isValid())
          m_bindings[binding.toString().toStdString()] = (*it)->objectName();
      }
    }

    /** This slot saves the currently configured options to the presenter */
    void QtReflOptionsDialog::saveOptions()
    {
      std::map<std::string,QVariant> options = m_presenter->options();

      //Iterate through all our bound widgets, pushing their value into the options map
      for(auto it = m_bindings.begin(); it != m_bindings.end(); ++it)
      {
        QString widgetName = it->second;
        if(widgetName.isEmpty())
          continue;

        QCheckBox* checkbox = findChild<QCheckBox*>(widgetName);
        if(checkbox)
        {
          options[it->first] = checkbox->isChecked();
          continue;
        }

        QSpinBox* spinbox = findChild<QSpinBox*>(widgetName);
        if(spinbox)
        {
          options[it->first] = spinbox->value();
          continue;
        }
      }

      //Update the presenter's options
      m_presenter->setOptions(options);
    }

    /** This slot sets the ui to match the presenter's options */
    void QtReflOptionsDialog::loadOptions()
    {
      std::map<std::string,QVariant> options = m_presenter->options();

      //Set the values from the options
      for(auto it = options.begin(); it != options.end(); ++it)
      {
        QString widgetName = m_bindings[it->first];
        if(widgetName.isEmpty())
          continue;

        QCheckBox* checkbox = findChild<QCheckBox*>(widgetName);
        if(checkbox)
        {
          checkbox->setChecked(it->second.toBool());
          continue;
        }

        QSpinBox* spinbox = findChild<QSpinBox*>(widgetName);
        if(spinbox)
        {
          spinbox->setValue(it->second.toInt());
          continue;
        }
      }
    }

  } //CustomInterfaces
} //MantidQt

