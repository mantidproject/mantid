#include "MantidQtAPI/PropertyWidget.h"
#include "MantidKernel/System.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidAPI/IWorkspaceProperty.h"

#include <boost/optional.hpp>
#include <boost/assign.hpp>

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
   *
   * @param s :: the string to convert.
   * @returns :: the converted number, wrapped in an optional.  Else an empty optional
   *             if conversion was unsuccesful.
   */
  boost::optional<double> stringToNumber(const std::string & s)
  {
    // Using std::istringstream is a nice way to do string-to-double conversion
    // in this situation as it rounds numbers for us at the same time.  Unfortunately,
    // commas seem to confuse it ("0,0,0" is converted to "0" without any warning).
    const bool containsComma = s.find(",") != std::string::npos;
    if( containsComma )
      return boost::optional<double>();
    
    std::istringstream i(s);
    double result;
    
    if (!(i >> result))
      return boost::optional<double>();
    
    return boost::optional<double>(result);
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

    const auto number = stringToNumber(value);
    if( !number )
      return false;
    
    static const std::vector<double> EMPTY_NUM_MACROS = boost::assign::list_of
      (EMPTY_DBL()) (-DBL_MAX) (DBL_MAX)
      (static_cast<double>(EMPTY_INT()))
      (static_cast<double>(EMPTY_LONG()))
      (static_cast<double>(-INT_MAX))
      (static_cast<double>(-LONG_MAX));

    return std::find(EMPTY_NUM_MACROS.begin(), EMPTY_NUM_MACROS.end(), *number) != EMPTY_NUM_MACROS.end();
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

    const auto number = stringToNumber(defaultValue);
    if( !number )
      return defaultValue;

    // We'd like to round off any instances of "2.7999999999999998", "0.050000000000000003",
    // or similar, but we want to keep the decimal point in values like "0.0" or "1.0" since
    // they can be a visual clue that a double is expected.
    if( defaultValue.length() > 5 )
    {
      std::stringstream roundedValue;
      const double rounded = *number;
      roundedValue << rounded;
      return roundedValue.str();
    }

    return defaultValue;
  }
} // anonymous namespace

namespace MantidQt
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PropertyWidget::PropertyWidget(Mantid::Kernel::Property * prop, QWidget * parent, QGridLayout * layout, int row)
  : QWidget(parent),
    m_prop(prop), m_gridLayout(layout), m_row(row),
    m_replaceWSButton(NULL)
  {
    if (!prop)
      throw std::runtime_error("NULL Property passed to the PropertyWidget constructor.");

    if (!m_gridLayout)
    {
      // Create a LOCAL grid layout
      m_gridLayout = new QGridLayout(this, 1, 5);
      m_gridLayout->setSpacing(5);
      this->setLayout(m_gridLayout);
      // Will always go to row 0
      m_row = 0;
      // And the parent is THIS widget
      m_parent = this;
      //this->setStyleSheet(          "QWidget {  background-color: yellow;  }"          );
    }
    else
    {
      // Use the parent of the provided QGridLayout when adding widgets
      m_parent = parent;
    }

    // Create the validator label (that red star)
    m_validLbl = new QLabel("*");
    QPalette pal = m_validLbl->palette();
    pal.setColor(QPalette::WindowText, Qt::darkRed);
    m_validLbl->setPalette(pal);
    // Start off hidden
    m_validLbl->setVisible(false);
    // Put it in the 4th column.
    m_gridLayout->addWidget(m_validLbl, m_row, 4);

    /// Save the documentation tooltip
    m_doc = QString::fromStdString(prop->documentation());


  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PropertyWidget::~PropertyWidget()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Slot called when someone clicks the "replace ws button" */
  void PropertyWidget::replaceWSButtonClicked()
  {
    emit replaceWorkspaceName(QString::fromStdString(m_prop->name()));
  }

  //----------------------------------------------------------------------------------------------
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
      m_widgets.push_back(m_replaceWSButton);
      // Place in the grid on column 2.
      m_gridLayout->addWidget(m_replaceWSButton, m_row, 2);
      m_replaceWSButton->setVisible(true);
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Externally set an error string to display in the validator
   *
   * @param error :: string to show in the star, empty means no error
   */
  void PropertyWidget::setError(const QString & error)
  {
    m_error = error.trimmed();

    // Show the invalid star if there was an error
    if (m_error.isEmpty())
      m_validLbl->setVisible(false);
    else
    {
      m_validLbl->setVisible(true);
      m_validLbl->setToolTip(m_error);
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Slot called when the value of the property had been changed.
   * This performs validation of the value and shows/hides that validator
   * star.
   *
   * It then emits a signal that the value of the property was changed.
   * */
  void PropertyWidget::valueChangedSlot()
  {
    // Try to set the value
    QString value = this->getValue().trimmed();
    // Use the default if empty
    if( value.isEmpty() )
      value = QString::fromStdString(m_prop->getDefault());

    std::string error("");
    try
    {
      error = m_prop->setValue(value.toStdString());
    }
    catch(std::exception & err_details)
    {
      error = err_details.what();
    }
    this->setError(QString::fromStdString(error).trimmed());

    // This will be caught by the GenericDialog.
    emit valueChanged( QString::fromStdString(m_prop->name()) ) ;
  }

  //----------------------------------------------------------------------------------------------
  /** Sets all widgets contained within to Enabled
   * @param val :: enabled or not   */
  void PropertyWidget::setEnabled(bool val)
  {
    for (int i=0; i < m_widgets.size(); i++)
      m_widgets[i]->setEnabled(val);
    QWidget::setEnabled(val);
  }

  //----------------------------------------------------------------------------------------------
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
