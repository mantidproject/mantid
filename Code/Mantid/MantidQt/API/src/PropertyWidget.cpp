#include "MantidQtAPI/PropertyWidget.h"
#include "MantidKernel/System.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/AnalysisDataService.h"

#include <boost/assign.hpp>
#include <boost/foreach.hpp>

#include <cmath>
#include <climits>
#include <cfloat>

#include <algorithm>

#include <QLineEdit>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::API::IWorkspaceProperty;

namespace // anonymous
{
  /**
   * Attempts to convert the given string into a double representation of a number.
   * Rounding will occur so that "0.0200000000001" will be output as 0.02, for example.
   *
   * @param s :: the string to convert.
   * @returns :: the converted number
   *
   * @throws std::runtime_error :: if conversion was unsuccesful.
   */
  double stringToRoundedNumber(const std::string & s)
  {
    // Using std::istringstream is a nice way to do string-to-double conversion
    // in this situation as it rounds numbers for us at the same time.  Unfortunately,
    // commas seem to confuse it ("0,0,0" is converted to "0" without any warning).
    const bool containsComma = s.find(",") != std::string::npos;
    if( containsComma )
      throw std::runtime_error("");
    
    std::istringstream i(s);
    double roundedNumber;
    
    if (!(i >> roundedNumber))
      throw std::runtime_error("");
    
    return roundedNumber;
  } 

  /**
   * Tests whether or not the given string value is "valid" for the given property.
   *
   * @param prop  :: the property to test against
   * @param value :: the string to test with
   * @returns     :: true if the value is valid, else false.
   */
  bool isValidPropertyValue(Mantid::Kernel::Property * prop, const std::string & value)
  {
    const auto guineaPig = boost::shared_ptr<Property>(prop->clone());
    return guineaPig->setValue(value).empty();
  }

  /**
   * Checks whether or not the given string value is one of our known "EMPTY_*" macros,
   * which are used by some algorithms to denote an empty default number value.  We
   * class the other *_MAX macros as "empty" macros, too.
   *
   * @param value :: the string value to check
   * @returns     :: true if the value is one of the macros, else false
   */
  bool isEmptyNumMacro(const std::string & value)
  {
    using namespace Mantid;

    // Catch instances of Python's "sys.maxint" which otherwise seem to fall through our net.
    if( value == "2.14748e+09" )
      return true;

    double roundedNumber;
    try
    {
      roundedNumber = stringToRoundedNumber(value);
    }
    catch( std::runtime_error & )
    {
      return false;
    }
    
    static const std::vector<double> EMPTY_NUM_MACROS = boost::assign::list_of
      (EMPTY_DBL()) (-DBL_MAX) (DBL_MAX)
      (static_cast<double>(EMPTY_INT()))
      (static_cast<double>(EMPTY_LONG()))
      (static_cast<double>(-INT_MAX))
      (static_cast<double>(-LONG_MAX));

    return std::find(EMPTY_NUM_MACROS.begin(), EMPTY_NUM_MACROS.end(), roundedNumber) != EMPTY_NUM_MACROS.end();
  }

  /**
   * Checks whether or not the given property can optionally be left blank by
   * an algorithm user.
   *
   * @param prop :: the property to check
   * @returns    :: true if can be left blank, else false
   */
  bool isOptionalProperty(Mantid::Kernel::Property * prop)
  {
    return isValidPropertyValue(prop, "") ||
           isValidPropertyValue(prop, prop->getDefault());
  }

  /**
   * For a given property, will create a placeholder text string.
   *
   * This will act in a similar way to the function used for the same purpose in the
   * WikiMaker script, except that we won't ever display "optional".
   *
   * @param prop :: the property for which to create placeholder text
   * @returns    :: the placeholder text
   */
  std::string createFieldPlaceholderText(Mantid::Kernel::Property * prop)
  {
    const std::string defaultValue = prop->getDefault();
    if( defaultValue.empty() )
      return "";

    if( !isValidPropertyValue(prop, defaultValue) || isEmptyNumMacro(prop->getDefault()))
      return "";

    // It seems likely that any instance of "-0" or "-0.0" should be replaced with an appropriate
    // EMPTY_* macro, but for now just display them as they appear in the Wiki.
    if( defaultValue == "-0" || defaultValue == "-0.0" )
      return "0";

    double roundedNumber;
    try
    {
      roundedNumber = stringToRoundedNumber(defaultValue);
    }
    catch( std::runtime_error & )
    {
      return defaultValue;
    }

    // We'd like to round off any instances of "2.7999999999999998", "0.050000000000000003",
    // or similar, but we want to keep the decimal point in values like "0.0" or "1.0" since
    // they can be a visual clue that a double is expected.
    static const std::size_t STRING_ROUNDING_LENGTH = 15;
    if( defaultValue.length() >= STRING_ROUNDING_LENGTH )
    {
      std::stringstream roundedValue;
      roundedValue << roundedNumber;
      return roundedValue.str();
    }

    return defaultValue;
  }
} // anonymous namespace

namespace MantidQt
{
namespace API
{
  /**
   * Constructor.
   *
   * @param parent :: the parent of this label.
   */
  ClickableLabel::ClickableLabel(QWidget * parent) : QLabel(parent) {}
  /**
   * Destructor.
   */
  ClickableLabel::~ClickableLabel() {}

  /**
   * Catches the mouse press event and emits the signal.
   *
   * @param event :: the QMouseEvent generated by a user clicking the label.
   */
  void ClickableLabel::mousePressEvent(QMouseEvent * event)
  {
    UNUSED_ARG(event);
    emit clicked();
  }

  /** Constructor
   */
  PropertyWidget::PropertyWidget(Mantid::Kernel::Property * prop, QWidget * parent, QGridLayout * layout, int row)
  : QWidget(parent),
    m_prop(prop), m_gridLayout(layout), m_parent(NULL), m_row(row),// m_info(NULL),
    m_doc(), m_replaceWSButton(NULL), m_widgets(), m_error(), m_isOutputWsProp(false),
    m_previousValue(), m_enteredValue(), m_icons(), m_useHistory(true)
  {
    if (!prop)
      throw std::runtime_error("NULL Property passed to the PropertyWidget constructor.");

    if (!m_gridLayout)
    {
      // Create a LOCAL grid layout
      m_gridLayout = new QGridLayout(this, 1, 5);
      m_gridLayout->setSpacing(5);
      this->setLayout(m_gridLayout);
      m_row = 0;
      m_parent = this;
    }
    else
    {
      // Use the parent of the provided QGridLayout when adding widgets
      m_parent = parent;
    }

    QWidget * infoWidget = new QWidget();
    infoWidget->setLayout(new QHBoxLayout(this));
    infoWidget->layout()->setSpacing(1);
    infoWidget->layout()->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->addWidget(infoWidget, m_row, 4);
    
    QMap<Info, QPair<QString, QString>> pathsAndToolTips;
    pathsAndToolTips[RESTORE] = QPair<QString, QString>(":/history.png", "This property had a previously-entered value.  Click to toggle it off and on.");
    pathsAndToolTips[REPLACE] = QPair<QString, QString>(":/replace.png", "A workspace with this name already exists and so will be overwritten.");
    pathsAndToolTips[INVALID] = QPair<QString, QString>(":/invalid.png", "");

    std::vector<Info> labelOrder = boost::assign::list_of(RESTORE)(REPLACE)(INVALID);

    BOOST_FOREACH( const Info info, labelOrder )
    {
      const QString iconPath = pathsAndToolTips[info].first;
      const QString toolTip  = pathsAndToolTips[info].second;

      auto icon = new ClickableLabel(this);
      icon->setPixmap(QPixmap(iconPath).scaledToHeight(15));
      icon->setVisible(false);
      icon->setToolTip(toolTip);

      infoWidget->layout()->addWidget(icon);
      m_icons[info] = icon;
    }

    connect(m_icons[RESTORE], SIGNAL(clicked()), this, SLOT(toggleUseHistory()));

    /// Save the documentation tooltip
    m_doc = QString::fromStdString(prop->briefDocumentation());

    if( !isOptionalProperty(prop) )
    {
      if(!m_doc.isEmpty()) m_doc += ".\n\n";
      m_doc += "This property is required.";
    }
    
    if( prop->direction() == Direction::Output && dynamic_cast<IWorkspaceProperty*>(prop) )
      m_isOutputWsProp = true;
  }

  /** Destructor
   */
  PropertyWidget::~PropertyWidget()
  {
  }

  /**
   * Set this widget's value.
   *
   * @param value :: the value to set
   */
  void PropertyWidget::setValue(const QString & value)
  {
    setValueImpl(value);
    valueChangedSlot();
    updateIconVisibility();
  }

  /**
   * Set this widget's value as a previously-entered value.
   *
   * @param previousValue :: the previous value of this widget
   */
  void PropertyWidget::setPreviousValue(const QString & previousValue)
  {
    m_previousValue = previousValue;
    setValue(m_previousValue);

    // Handle input workspace options that have been set to a workspace previously,
    // but where the workspace no longer exists.
    const auto currentValue = getValue().toStdString();
    if( getValue() != previousValue )
    {
      m_previousValue = "";
      setValue(m_previousValue);
    }
    
    // Once we've made the history icon visible, it will stay that way for
    // the lifetime of the property widget.
    if( m_previousValue.toStdString() != m_prop->getDefault() &&
        !m_previousValue.isEmpty() )
      m_icons[RESTORE]->setVisible(true);
  }

  /**
   * Update which icons should be shown.
   * @param error An optional error string. If empty the property is revalidated by calling prop->setValue and
   * the error is pulled from here
   */
  void PropertyWidget::updateIconVisibility(const QString &error)
  {
    QString userError(error);
    if(userError.isEmpty())
    {
      // Show "*" icon if the value is invalid for this property.
      QString value = this->getValue().trimmed();
      // Use the default if empty
      if( value.isEmpty() )
        value = QString::fromStdString(m_prop->getDefault());

      try
      {
        userError = QString::fromStdString(m_prop->setValue(value.toStdString()));
      }
      catch(std::exception & err_details)
      {
        userError = QString::fromAscii(err_details.what());
      }
    }
    this->setError(userError.trimmed());

    m_icons[INVALID]->setVisible(!m_error.isEmpty());
    m_icons[INVALID]->setToolTip(m_error);
    // Show "!" icon if a workspace would be overwritten.
    if( m_isOutputWsProp )
    {
      const bool wsExists = Mantid::API::AnalysisDataService::Instance().doesExist(getValue().toStdString());
      m_icons[REPLACE]->setVisible(wsExists);
    }
  }

  /** Slot called when someone clicks the "replace ws button" */
  void PropertyWidget::replaceWSButtonClicked()
  {
    emit replaceWorkspaceName(QString::fromStdString(m_prop->name()));
  }

  /**
   * Emits a signal that the value of the property was changed.
   */
  void PropertyWidget::valueChangedSlot()
  {
    // This will be caught by the GenericDialog.
    emit valueChanged( QString::fromStdString(m_prop->name()) ) ;
  }

  /**
   * To be called when a user edits a property, as opposed to one being set programmatically.
   */
  void PropertyWidget::userEditedProperty()
  {
    setUseHistoryIcon(getValue() == m_previousValue);
    if( getValue() != m_previousValue )
      m_enteredValue = getValue();
    updateIconVisibility();
    valueChangedSlot();
  }
  
  /**
   * Toggle whether or not to use the previously-entered value.
   */
  void PropertyWidget::toggleUseHistory()
  {
    setUseHistoryIcon(!m_useHistory);
    if( m_useHistory )
      setValue(m_previousValue);
    else
      setValue(m_enteredValue);
  }
  
  /**
   * Sets the history on/off icons.
   *
   * @param useHistory :: when true, will show the history on image, when false will show the history off image.
   */
  void PropertyWidget::setUseHistoryIcon(bool useHistory)
  {
    if( m_useHistory != useHistory )
    {
      m_useHistory = useHistory;
      const QString iconPath = useHistory ? ":/history.png" : ":/history_off.png";
      m_icons[RESTORE]->setPixmap(QPixmap(iconPath).scaledToHeight(15));
    }
  }

  /** Create and show the "Replace WS" button.
   *
   * This only has an effect for Output WorkspaceProperty's.
   *
   */
  void PropertyWidget::addReplaceWSButton()
  {
    // Don't re-create it if it already exists
    if (m_replaceWSButton)
      return;

    IWorkspaceProperty * wsProp = dynamic_cast<IWorkspaceProperty*>(m_prop);
    if (wsProp && (m_prop->direction() == Direction::Output) )
    {
      m_replaceWSButton = new QPushButton(QIcon(":/data_replace.png"), "", m_parent);
      // MG: There is no way with the QIcon class to actually ask what size it is so I had to hard
      // code this number here to get it to a sensible size
      m_replaceWSButton->setMaximumWidth(32);
      //m_wsbtn_tracker[btn ] = 1;
      m_replaceWSButton->setToolTip("Replace input workspace");
      connect(m_replaceWSButton, SIGNAL(clicked()), this, SLOT(replaceWSButtonClicked()));
      connect(m_replaceWSButton, SIGNAL(clicked()), this, SLOT(valueChangedSlot()));
      m_widgets.push_back(m_replaceWSButton);
      // Place in the grid on column 2.
      m_gridLayout->addWidget(m_replaceWSButton, m_row, 2);
      m_replaceWSButton->setVisible(true);
    }
  }

  /** Externally set an error string to display in the validator
   *
   * @param error :: string to show in the star, empty means no error
   */
  void PropertyWidget::setError(const QString & error)
  {
    m_error = error.trimmed();
  }

  /** Sets all widgets contained within to Enabled
   * @param val :: enabled or not   */
  void PropertyWidget::setEnabled(bool val)
  {
    for (int i=0; i < m_widgets.size(); i++)
      m_widgets[i]->setEnabled(val);
    QWidget::setEnabled(val);
  }

  /** Sets all widgets contained within to Visible
   * @param val :: Visible or not   */
  void PropertyWidget::setVisible(bool val)
  {
    for (int i=0; i < m_widgets.size(); i++)
      m_widgets[i]->setVisible(val);
    QWidget::setVisible(val);
  }

  /**
   * Given a property and its associated label, will make font adjustments to the label
   * based on whether or not the property is mandatory.
   *
   * @param prop  :: property to which the label belongs
   * @param label :: widget containing the label
   */
  void PropertyWidget::setLabelFont(Mantid::Kernel::Property * prop, QWidget * label)
  {
    if( !isOptionalProperty(prop) )
    {
      auto font = label->font();
      font.setBold(true);
      label->setFont(font);
    }
  }

  /**
   * Given a property and its associated QLineEdit text field, will set the field's placeholder
   * text based on the default value of the property.
   *
   * @param prop  :: the property who's default value to use
   * @param field :: the associated text field to set the placeholder text of
   */
  void PropertyWidget::setFieldPlaceholderText(Mantid::Kernel::Property * prop, QLineEdit * field)
  {
    field->setPlaceholderText(QString::fromStdString(createFieldPlaceholderText(prop)));
  }

} // namespace MantidQt
} // namespace API
