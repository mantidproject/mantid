#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidKernel/FileProperty.h"
#include <QStringList>
#include <QFileDialog>
#include <QFileInfo>
#include "boost/lexical_cast.hpp"
#include "Poco/StringTokenizer.h"

using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

static const int MAX_FILE_NOT_FOUND_DISP = 3;

const QString MWRunFiles::DEFAULT_FILTER = "Files (*.RAW *.raw *.NXS *.nxs);;All Files (*.*)";

MWRunFiles::MWRunFiles(QWidget *parent, const QString prevSettingsGr, const QComboBox * const instrum, const QString label, const QString toolTip, const QString exts) :
  MantidWidget(parent), m_toolTipOrig(toolTip), m_filter(exts)
{
  // allows saving and loading the values the user entered on to the form
  m_prevSets.beginGroup(prevSettingsGr);

  if(instrum)
  {// this function also adds some information to the tooltip
    instrumentChange(instrum->currentText());
  }
  else
  {
    setToolTip(m_toolTipOrig + "\n(no intrument selected)");
  }

  m_designedWidg.setupUi(this);
  
  m_designedWidg.lbDiscrip->setText(label);

  // setup the label on the form that contains a star to look like a validator star
  QPalette pal = m_designedWidg.valid->palette();
  pal.setColor(QPalette::WindowText, Qt::darkRed);
  m_designedWidg.valid->setPalette(pal);
  m_designedWidg.valid->hide();
  
  connect(m_designedWidg.leFiles, SIGNAL(editingFinished()), this, SLOT(readEntries()));
  connect(m_designedWidg.pbBrowse, SIGNAL(clicked()), this, SLOT(browseClicked()));

  connect(instrum, SIGNAL(currentIndexChanged(const QString &)),
    this, SLOT(instrumentChange(const QString &)));

}
/** Returns the user entered filenames as a coma seperated list (integers are expanded to
*  filenames). If one of the files couldn't be found it then no filenames are returned
*/
const std::vector<std::string>& MWRunFiles::getFileNames() const
{
  return m_files;
}
/** Safer than using getRunFiles()[0] in the situation were there are no files
*  @return an empty string is returned if no input files have been defined or one of the files can't be found, otherwise it's the name of the first input file
*/
QString MWRunFiles::getFile1() const
{
  if ( m_files.begin() != m_files.end() )
  {
    return QString::fromStdString(*m_files.begin());
  }
  return "";
}

QString MWRunFiles::openFileDia()
{
  QStringList filenames;
  filenames = QFileDialog::getOpenFileNames(this, "Open file",
    m_prevSets.value("load file dir", "").toString(), m_filter);
  if(filenames.isEmpty())
  {
	return "";
  }
  m_prevSets.setValue("load file dir", 
	                    QFileInfo(filenames.front()).absoluteDir().path());
  // turns the QStringList into a coma separated list inside a QString
  return filenames.join(", ");
}
/// Convert integers from the LineEdit box into filenames but leaves all non-integer values untouched
void MWRunFiles::readRunNumAndRanges()
{
  readComasAndHyphens(m_designedWidg.leFiles->text().toStdString(), m_files);
  // only show the validator if errors are found below
  m_designedWidg.valid->hide();
  QStringList errors;

  // set up the object we use for validation
  std::vector<std::string> exts;
  exts.push_back("raw"); exts.push_back("RAW");
  exts.push_back("NXS"); exts.push_back("nxs");
  FileProperty loadData("Filename", "", FileProperty::Load, exts);

  std::vector<std::string>::iterator i = m_files.begin(), end = m_files.end();
  for ( ; i != end; ++i )
  {
    try
	{
	  const unsigned int runNumber = boost::lexical_cast<unsigned int>(*i);
	  *i = m_instrument.toStdString() + //instrument code
	    boost::lexical_cast<std::string>(runNumber) +
		".raw";                                      //only raw files are supported at the moment //??STEVES change this?
	}
	catch ( boost::bad_lexical_cast )
	{// the entry doesn't read as a run number
	}// the alternative is that they entered a filename and we leave that as it is
    if ( errors.size() < MAX_FILE_NOT_FOUND_DISP )
	{
	  std::string problem = loadData.setValue(*i);
	  if ( ! problem.empty() )
	  {
	    errors << QString::fromStdString(problem);
	  }
	}
  }
  if ( errors.size() > 0 )
  {
    m_designedWidg.valid->setToolTip(errors.join(", "));
	m_designedWidg.valid->show();
	m_files.clear();
  }
}
void MWRunFiles::readComasAndHyphens(const std::string &in, std::vector<std::string> &out)
{
  if ( in.empty() )
  {// it is not an error to have an empty line but it would cause problems with an error check a the end of this function
    return;
  }
  
  Poco::StringTokenizer ranges(in, "-");
  Poco::StringTokenizer::Iterator aList = ranges.begin();

  out.clear();
  do
  {
    Poco::StringTokenizer preHyp(*aList, ",", Poco::StringTokenizer::TOK_TRIM);
    Poco::StringTokenizer::Iterator readPos = preHyp.begin();
    if ( readPos == preHyp.end() )
    {// can only mean that there was only an empty string or white space the '-'
      throw std::invalid_argument("'-' found at the start of a list, can't interpret range specification");
    }
	out.reserve(out.size()+preHyp.count());
    for ( ; readPos != preHyp.end(); ++readPos )
    {
      out.push_back(*readPos);
    }
    if (aList == ranges.end()-1)
    {// there is no more input
      break;
    }

    Poco::StringTokenizer postHy(*(aList+1),",",Poco::StringTokenizer::TOK_TRIM);
    readPos = postHy.begin();
    if ( readPos == postHy.end() )
    {
      throw std::invalid_argument("A '-' follows straight after another '-', can't interpret range specification");
    }
	
    try
    {//we've got the point in the string where there was a hyphen, first get the stuff that is after it
      // the tokenizer will always return at least on string
      const int rangeEnd = boost::lexical_cast<int>(*readPos);
      // the last string before the hyphen should be an int, here we get the it
      const int rangeStart = boost::lexical_cast<int>(out.back()); 
      // counting down isn't supported
      if ( rangeStart > rangeEnd )
      {
        throw std::invalid_argument("A range where the first integer is larger than the second is not allowed");
      }
      // expand the range
      for ( int j = rangeStart+1; j < rangeEnd; j++ )
      {
        out.push_back(boost::lexical_cast<std::string>(j));
      }
	}
	catch (boost::bad_lexical_cast)
	{// the hyphen wasn't between two numbers, don't intepret it as a range instaed reconstruct the hyphenated string and add it to the list
	  out.back() += "-" + *readPos;
	}
  }
  while( ++aList != ranges.end() );

  if ( *(in.end()-1) == '-' )
  {
    throw std::invalid_argument("'-' found at the end of a list, can't interpret range specification");
  }
}
/** This slot opens a file browser, then adds any selected file to the
*  leFiles LineEdit and emits fileChanged()
*/
void MWRunFiles::browseClicked()
{
  QString uFile = openFileDia();
  if( uFile.trimmed().isEmpty() ) return;

  if ( ! m_designedWidg.leFiles->text().isEmpty() )
  {
    m_designedWidg.leFiles->setText(
	  m_designedWidg.leFiles->text()+", " + uFile);
  }
  else m_designedWidg.leFiles->setText(uFile);
  
  readEntries();
}
void MWRunFiles::instrumentChange(const QString &newInstr)
{
  // the tooltip will tell users which instrument, if any, if associated with this control
  if (newInstr.isEmpty())
  {
    setToolTip(m_toolTipOrig + "\n(no intrument selected)");
  }
  else
  {
    static const QString TIP_SEC_1 = "\n(selected intrument = ", TIP_SEC_2 = ")";
    if ( toolTip() == m_toolTipOrig+TIP_SEC_1+m_instrument+TIP_SEC_2 )
    {// this checked if the tool tip was modified outside the widget, it wasn't and so we'll update it
      setToolTip(m_toolTipOrig+TIP_SEC_1+newInstr+TIP_SEC_2);
    }
  }
  
  m_instrument = newInstr;
  
  readEntries();
}
void MWRunFiles::readEntries()
{
  readRunNumAndRanges();
  //??STEVES?? do the validation, then conditionally do the next line
  emit fileChanged();
}
QString MWRunFile::openFileDia()
{
  QString filename;
  filename = QFileDialog::getOpenFileName(this, "Open file",
    m_prevSets.value("load file dir", "").toString(), m_filter);
  if( ! filename.isEmpty() )
  {
	m_prevSets.setValue("load file dir", 
	                    QFileInfo(filename).absoluteDir().path());
  }
  return filename;
}
/** Slot opens a file browser, writing the selected file to leFiles
*  LineEdit and emits fileChanged()
*/
void MWRunFile::browseClicked()
{
  QString uFile = openFileDia();
  if( uFile.trimmed().isEmpty() ) return;

  m_designedWidg.leFiles->setText(uFile);
  
  readEntries();  
  emit fileChanged();
}