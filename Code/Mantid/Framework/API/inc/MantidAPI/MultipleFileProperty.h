#ifndef MANTID_API_MULTIPLEFILEPROPERTY_H_
#define MANTID_API_MULTIPLEFILEPROPERTY_H_

#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/MultiFileNameParser.h"
#include <vector>
#include <set>

namespace Mantid
{
namespace API
{

  /**
    A property to allow a user to specify multiple files to load.

    The current functionality is such that there are two basic forms of syntax.  For the puposes
    of documentation we'll call these the "Long Form" and "Short Form".

    ------------------------------------------------------------------------------------------------------
    [A] Short Form

        These strings are of the format "[dir][inst][under][runs][ext]" where:
    
        [dir]   (Optional) = The OS-specific file directory, e.g. "c:/data/"
        [inst]  (Optional) = The instrument name, e.g. "IRS" or "PG3".
        [under] (Optional) = Some instrument filenames require an underscore.
        [runs]  (Required) = The run numbers, e.g. "0102, 0110-0115, 0120, 0130:0140:2"
        [ext]   (Optional) = The file extension, e.g. ".raw"

        For optional values, defaults or user settings are used where necessary.
    
        For [runs], users specify lists and ranges of runs using comma, plus, minus and colon.  Some examples:

        "TSC0001,0002"       = Runs 1 and 2 of the TOSCA instrument are to be loaded.
        "0003+0004"          = Runs 3 and 4 of the default instrument are to be loaded and added together.
        "0005:0009.raw"      = The raw files containing runs 5 to 9 of the default instrument are to be loaded.
        "c:/data/0010-0014"  = The files in "c:/data/" containing runs 10 to 14 of the default instrument are 
                               to be loaded and added together.
        "IRS0020:0028:2.nxs" = The nexus files containing runs 20, 22, 24, 26 and 28 for IRIS are to be loaded.
        "INST_0030-0038:3"   = Runs 30, 33, and 36 of INST are to be loaded and added together.

    ------------------------------------------------------------------------------------------------------
    [B] Long Form
    
        These strings are of the format "[[short_form][operator]]...[short_form]" where:

        [short_form] = [dir][inst][under][runs][ext], which is the "Short Form" outlined above.
        [operator]   = Either a comma or a plus.

        Some examples:

        "TSC0001,TSC0002+0003"        = Runs 1, 2 and 3 of the TOSCA instrument should be loaded, but 2 and 3 
                                        are added together.
        "TSC0005+TSC0006,TSC0007.raw" = Runs 5 and 6 as well as the raw file containing run 7 of the TOSCA 
                                        instrument should be loaded, but 5 and 6 are added together.

    ------------------------------------------------------------------------------------------------------
    NOTES: 
    
    [1] Presently, we disallow more complex algebra such as "TSC0005,0006+TSC0007". In such a case it is 
        ambiguous whether or not the user wishes to just add run 7 to 6, or add run 7 to both 5 and 6.
    
    [2] The "Short Form" is parsed by the Kernel::MultiFileNameParsing::Parser class, whereas this class is 
        responsible for splitting up the Long Form.

    [3] The functionality of this class is such that all strings are stored only after being converted to
        the Long Form, and all filenames are fully resolved.  For example "0005,0006+0007" is stored as
        "[dir][inst][under]0005[ext],[dir][inst][under]0006[ext]+[dir][inst][under]0007[ext]".

    [4] The default functionality of this Property can be changed to emulate a simple FileProperty - to do
        this, the user must change the properties file.  Disabling multi file loading in this way will allow
        users to use "," and "+" in their filenames, and in this case we use the dummy "" delimiters to call
        toValue and toString.

    ------------------------------------------------------------------------------------------------------

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport MultipleFileProperty : public Kernel::PropertyWithValue< std::vector< std::vector< std::string> > >
  {
  public:
    ///Constructor
    MultipleFileProperty(const std::string & name,const std::vector<std::string> & exts = std::vector<std::string>());

    ~MultipleFileProperty();

    /// 'Virtual copy constructor
    virtual MultipleFileProperty* clone() const { return new MultipleFileProperty(*this); }

    /// Overridden functions to accomodate std::vector<std::vector<std::string>>> structure of this property.
    virtual std::string setValue(const std::string & propValue);
    virtual std::string value() const;
    virtual std::string getDefault() const;

    /// @return the vector of suggested extensions. For use in GUIs showing files.
    std::vector<std::string> getExts() const { return m_exts; }
    
    /// Returns the main file extension that's used 
    std::string getDefaultExt() const {return m_defaultExt;}

    // Unhide the PropertyWithValue assignment operator
    using Kernel::PropertyWithValue< std::vector< std::vector< std::string> > >::operator=;

    /// Return a "flattened" vector with the contents of the given vector of vectors.
    static std::vector<std::string> flattenFileNames(const std::vector<std::vector<std::string> > & fileNames);

  private:
    std::string setValueAsSingleFile(const std::string & propValue);
    std::string setValueAsMultipleFiles(const std::string & propValue);
    /// Whether or not the user has turned on multifile loading.
    bool m_multiFileLoadingEnabled;

    /// Suggested extensions
    std::vector<std::string> m_exts;
    /// Parser used to parse multi-file strings.
    Kernel::MultiFileNameParsing::Parser m_parser;
    ///The default file extension associated with the type of file this property will handle
    std::string m_defaultExt;
  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_MULTIPLEFILEPROPERTY_H_ */
