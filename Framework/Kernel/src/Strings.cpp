// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Strings.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/UnitLabel.h"

#include <Poco/Path.h>

#include <boost/algorithm/string.hpp>
#include <memory>

#include <fstream>

using std::size_t;

namespace Mantid::Kernel::Strings {

//------------------------------------------------------------------------------------------------
/** Loads the entire contents of a text file into a string
 *
 * @param filename :: full path to file
 * @return string contents of text file
 */
std::string loadFile(const std::string &filename) {
  std::string retVal;
  std::string str;
  std::ifstream in;
  in.open(filename.c_str());
  getline(in, str);
  while (in) {
    retVal += str + "\n";
    getline(in, str);
  }
  in.close();
  return retVal;
}

// ------------------------------------------------------------------------------------------------
/** Return a string shortened with the center replace by " ... "
 * If the string is already short enough then the original string will be
 *returned.
 * If the max length or input string length is smaller than the ellipsis, the
 *input will be returned.
 *
 * @param input :: input string
 * @param max_length :: The maximum length of the return string (0 = return full
 *string)
 * @return the modified string.
 */
std::string shorten(const std::string &input, const size_t max_length) {
  const std::string ellipsis = " ... ";
  const size_t ellipsisSize = ellipsis.size();
  // limit too small or input too small, return input string
  if ((max_length == 0) || (input.size() < ellipsisSize + 2) || (input.size() <= max_length))
    return input;

  const size_t end_length = (max_length - ellipsisSize) / 2;
  std::string retVal = input.substr(0, end_length) + ellipsis + input.substr(input.size() - end_length, end_length);
  return retVal;
}

//------------------------------------------------------------------------------------------------
/** Return a string with all matching occurence-strings
 *
 * @param input :: input string
 * @param find_what :: will search for all occurences of this string
 * @param replace_with :: ... and replace them with this.
 * @return the modified string.
 */
std::string replace(const std::string &input, const std::string &find_what, const std::string &replace_with) {
  std::string output = input;
  std::string::size_type pos = 0;
  while ((pos = output.find(find_what, pos)) != std::string::npos) {
    output.erase(pos, find_what.length());
    output.insert(pos, replace_with);
    pos += replace_with.length();
  }
  return output;
}

/**
 * Return a string with all occurrences of the characters in the input replaced
 * by the replace string
 * @param input :: The input string to perform the replacement on
 * @param charStr :: Each occurrence of ANY character in this string within the
 * input string will be replaced by substitute
 * @param substitute :: A substitute string
 * @return A new string with the characters replaced
 */
MANTID_KERNEL_DLL std::string replaceAll(const std::string &input, const std::string &charStr,
                                         const std::string &substitute) {
  std::string replaced;
  replaced.reserve(input.size());
  std::string::const_iterator iend = input.end();
  for (std::string::const_iterator itr = input.begin(); itr != iend; ++itr) {
    char inputChar = (*itr);
    if (charStr.find_first_of(inputChar) == std::string::npos) // Input string
                                                               // char is not
                                                               // one of those
                                                               // to be replaced
    {
      replaced.push_back(inputChar);
    } else {
      replaced.append(substitute);
    }
  }
  return replaced;
}

/** Convert all characters in string to lowercase
 */
MANTID_KERNEL_DLL std::string toLower(const std::string &input) {
  std::string output(input);
  std::transform(output.begin(), output.end(), output.begin(), ::tolower);
  return output;
}

/** Convert all characters in string to uppercase
 */
MANTID_KERNEL_DLL std::string toUpper(const std::string &input) {
  std::string output(input);
  std::transform(output.begin(), output.end(), output.begin(), ::toupper);
  return output;
}

/** Checks if string ends with a suffix
 */
MANTID_KERNEL_DLL bool endsWith(std::string const &str, std::string const &suffix) {
  if (str.size() >= suffix.size()) {
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
  }
  return false;
}

//------------------------------------------------------------------------------------------------
/**
 * Function to convert a number into hex
 * output (and leave the stream un-changed)
 * @param OFS :: Output stream
 * @param n :: Integer to convert
 * \todo Change this to a stream operator
 */
void printHex(std::ostream &OFS, const int n) {
  std::ios_base::fmtflags PrevFlags = OFS.flags();
  OFS << "Ox";
  OFS.width(8);
  OFS.fill('0');
  hex(OFS);
  OFS << n;
  OFS.flags(PrevFlags);
}

//------------------------------------------------------------------------------------------------
/**
 * Removes the multiple spaces in the line
 * @param Line :: Line to process
 * @return String with single space components
 */
std::string stripMultSpc(const std::string &Line) {
  std::string Out;
  int spc(1);
  int lastReal(-1);
  for (unsigned int i = 0; i < Line.length(); i++) {
    if (Line[i] != ' ' && Line[i] != '\t' && Line[i] != '\r' && Line[i] != '\n') {
      lastReal = i;
      spc = 0;
      Out += Line[i];
    } else if (!spc) {
      spc = 1;
      Out += ' ';
    }
  }
  lastReal++;
  if (lastReal < static_cast<int>(Out.length()))
    Out.erase(lastReal);
  return Out;
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
/**
 *  Checks that as least cnt letters of
 *  works is part of the string. It is currently
 *  case sensitive. It removes the Word if found
 *  @param Line :: Line to process
 *  @param Word :: Word to use
 * @param cnt :: Length of Word for significants [default =4]
 *  @retval 1 on success (and changed Line)
 *  @retval 0 on failure
 */
int extractWord(std::string &Line, const std::string &Word, const int cnt) {
  if (Word.empty())
    return 0;

  size_t minSize(cnt > static_cast<int>(Word.size()) ? Word.size() : cnt);
  std::string::size_type pos = Line.find(Word.substr(0, minSize));
  if (pos == std::string::npos)
    return 0;
  // Pos == Start of find
  size_t LinePt = minSize + pos;
  for (; minSize < Word.size() && LinePt < Line.size() && Word[minSize] == Line[LinePt]; LinePt++, minSize++) {
  }

  Line.erase(pos, LinePt - (pos - 1));
  return 1;
}

//------------------------------------------------------------------------------------------------
/** If a word ends with a number representing a positive integer, return
 * the value of that int.
 *
 * @param word :: string possibly ending in a number
 * @return the number, or -1 if it does not end in a number
 */
int endsWithInt(const std::string &word) {
  if (word.empty())
    return -1;
  int out = -1;
  // Find the index of the first number in the string (if any)
  auto firstNumber = int(word.size());
  for (int i = int(word.size()) - 1; i >= 0; i--) {
    char c = word[i];
    if ((c > '9') || (c < '0'))
      break;
    firstNumber = i;
  }
  // Convert the string of decimals to an int
  if (firstNumber < int(word.size())) {
    std::string part = word.substr(firstNumber, word.size() - firstNumber);
    if (!convert(part, out))
      return -1;
  }
  return out;
}

//------------------------------------------------------------------------------------------------
/**
 *  Check to see if S is the same as the
 *  first part of a phrase. (case insensitive)
 *  @param S :: string to check
 *  @param fullPhrase :: complete phrase
 *  @return 1 on success
 */
int confirmStr(const std::string &S, const std::string &fullPhrase) {
  const size_t nS(S.length());
  const size_t nC(fullPhrase.length());
  if (nS > nC || nS == 0)
    return 0;
  for (size_t i = 0; i < nS; i++)
    if (S[i] != fullPhrase[i])
      return 0;
  return 1;
}

//------------------------------------------------------------------------------------------------
/**
 *  Gets a line and determine if there is addition component to add
 *  in the case of a very long line.
 *  @param fh :: input stream to get line
 *  @param Out :: string up to last 'tab' or ' '
 *  @param Excess :: string after 'tab or ' '
 *  @param spc :: number of char to try to read
 *  @retval 1 :: more line to be found
 *  @retval -1 :: Error with file
 *  @retval 0  :: line finished.
 */
int getPartLine(std::istream &fh, std::string &Out, std::string &Excess, const int spc) {
  // std::string Line;
  if (fh.good()) {
    auto ss = new char[spc + 1];
    const auto clen = static_cast<int>(spc - Out.length());
    fh.getline(ss, clen, '\n');
    ss[clen + 1] = 0; // incase line failed to read completely
    Out += static_cast<std::string>(ss);
    delete[] ss;
    // remove trailing comments
    std::string::size_type pos = Out.find_first_of("#!");
    if (pos != std::string::npos) {
      Out.erase(pos);
      return 0;
    }
    if (fh.gcount() == clen - 1) // cont line
    {
      pos = Out.find_last_of("\t ");
      if (pos != std::string::npos) {
        Excess = Out.substr(pos, std::string::npos);
        Out.erase(pos);
      } else
        Excess.erase(0, std::string::npos);
      fh.clear();
      return 1;
    }
    return 0;
  }
  return -1;
}

//------------------------------------------------------------------------------------------------
/**
 *  Removes all spaces from a string
 * except those with in the form '\ '
 *  @param CLine :: Line to strip
 *  @return String without space
 */
std::string removeSpace(const std::string &CLine) {
  std::string Out;
  char prev = 'x';
  for (char character : CLine) {
    if (!isspace(character) || prev == '\\') {
      Out += character;
      prev = character;
    }
  }
  return Out;
}

//------------------------------------------------------------------------------------------------
/**
 *  Reads a line from the stream of max length spc.
 *  Trailing comments are removed. (with # or ! character)
 *  @param fh :: already open file handle
 *  @return String read.
 */
std::string getLine(std::istream &fh) {
  std::string line;
  getLine(fh, line);
  return line;
}

//------------------------------------------------------------------------------------------------
/**
 *  Reads a line from the stream of max length spc.
 *  Trailing comments are removed. (with # or ! character)
 *  @param fh :: already open file handle
 *  @param Line :: string read
 */
void getLine(std::istream &fh, std::string &Line) {
  if (std::getline(fh, Line)) {
    // remove trailing comments
    auto pos = Line.find_first_of("#!");
    if (pos != std::string::npos)
      Line.erase(pos);
  }
}

/**
 * Peek at a line without extracting it from the stream
 */
std::string peekLine(std::istream &fh) {
  std::string str;
  std::streampos pos = fh.tellg();
  getline(fh, str);
  fh.seekg(pos);

  return strip(str);
}

//------------------------------------------------------------------------------------------------
/**
 *  Determines if a string is only spaces
 *  @param A :: string to check
 *  @return 1 on an empty string , 0 on failure
 */
int isEmpty(const std::string &A) {
  std::string::size_type pos = A.find_first_not_of(" \t");
  return (pos != std::string::npos) ? 0 : 1;
}

//------------------------------------------------------------------------------------------------
/**
 *  removes the string after the comment type of
 *  '$ ' or '!' or '#  '
 *  @param A :: String to process
 */
void stripComment(std::string &A) {
  std::string::size_type posA = A.find("$ ");
  std::string::size_type posB = A.find("# ");
  std::string::size_type posC = A.find('!');
  if (posA > posB)
    posA = posB;
  if (posA > posC)
    posA = posC;
  if (posA != std::string::npos)
    A.erase(posA, std::string::npos);
}

//------------------------------------------------------------------------------------------------
/**
 *  Returns the string from the first non-space to the
 *  last non-space
 * @param A :: string to process
 * @return shortened string
 */
std::string fullBlock(const std::string &A) { return strip(A); }

//------------------------------------------------------------------------------------------------
/** Returns the string from the first non-space to the
 *  last non-space
 *  @param A :: string to process
 *  @return shortened string
 */
std::string strip(const std::string &A) {
  std::string result(A);
  boost::trim(result);
  return result;
}

/**
 * Return true if the line is to be skipped (starts with #).
 * @param line :: The line to be checked
 * @return True if the line should be skipped
 */
bool skipLine(const std::string &line) {
  // Empty or comment
  return (line.empty() || boost::starts_with(line, "#"));
}

//------------------------------------------------------------------------------------------------
/**
 *  Write out the line in the limited form for MCNPX
 *  ie initial line from 0->72 after that 8 to 72
 *  (split on a space or comma)
 *  @param Line :: full MCNPX line
 *  @param OX :: ostream to write to
 */
void writeMCNPX(const std::string &Line, std::ostream &OX) {
  const int MaxLine(72);
  std::string::size_type pos(0);
  std::string X = Line.substr(0, MaxLine);
  std::string::size_type posB = X.find_last_of(" ,");
  int spc(0);
  while (posB != std::string::npos && static_cast<int>(X.length()) >= MaxLine - spc) {
    pos += posB + 1;
    if (!isspace(X[posB]))
      posB++;
    const std::string Out = X.substr(0, posB);
    if (!isEmpty(Out)) {
      if (spc)
        OX << std::string(spc, ' ');
      OX << X.substr(0, posB) << '\n';
    }
    spc = 8;
    X = Line.substr(pos, MaxLine - spc);
    posB = X.find_last_of(" ,");
  }
  if (!isEmpty(X)) {
    if (spc)
      OX << std::string(spc, ' ');
    OX << X << '\n';
  }
}

//------------------------------------------------------------------------------------------------
/**
 *  Splits the sting into parts that are space delminated.
 *  @param Ln :: line component to strip
 *  @return vector of components
 */
std::vector<std::string> StrParts(const std::string &Ln) {
  auto tokenizer = Mantid::Kernel::StringTokenizer(
      Ln, " ", Mantid::Kernel::StringTokenizer::TOK_TRIM | Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  return tokenizer.asVector();
}

/**
 * Splits a string into key value pairs and returns them as a map. Whitespace
 * between separators is ignored
 * @param input :: The string containing the key/values
 * @param keyValSep :: The separator that splits a key and value [default: "="]
 * @param listSep :: The separator that splits elements of the list [default:
 * ","]
 * @returns A map of keys->values
 */
std::map<std::string, std::string> splitToKeyValues(const std::string &input, const std::string &keyValSep,
                                                    const std::string &listSep) {
  std::map<std::string, std::string> keyValues;
  const int splitOptions =
      Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY + Mantid::Kernel::StringTokenizer::TOK_TRIM;
  Mantid::Kernel::StringTokenizer listSplitter(input, listSep);
  for (const auto &iter : listSplitter) {
    Mantid::Kernel::StringTokenizer keyValSplitter(iter, keyValSep, splitOptions);
    if (keyValSplitter.count() == 2) {
      keyValues[keyValSplitter[0]] = keyValSplitter[1];
    }
  }

  return keyValues;
}

//------------------------------------------------------------------------------------------------
/**
 *  Converts a vax number into a standard unix number
 *  @param A :: float number as read from a VAX file
 *  @return float A in IEEE little eindian format
 */
float getVAXnum(const float A) {
  union {
    // char a[4];
    float f;
    int ival;
  } Bd;

  int sign, expt, fmask;
  float frac;
  double onum;

  Bd.f = A;
  sign = (Bd.ival & 0x8000) ? -1 : 1;
  expt = ((Bd.ival & 0x7f80) >> 7); // reveresed ?
  if (!expt)
    return 0.0;

  fmask = ((Bd.ival & 0x7f) << 16) | ((Bd.ival & 0xffff0000) >> 16);
  expt -= 128;
  fmask |= 0x800000;
  frac = static_cast<float>(fmask) / 0x1000000;
  onum = frac * static_cast<float>(sign) * pow(2.0, expt);
  return static_cast<float>(onum);
}

//------------------------------------------------------------------------------------------------
/**
 *  Takes a character string and evaluates
 *  the first [typename T] object. The string is then
 *  erase upt to the end of number.
 *  The diffierence between this and section is that
 *  it allows trailing characters after the number.
 *  @param out :: place for output
 *  @param A :: string to process
 *  @return 1 on success 0 on failure
 */
template <typename T> int sectPartNum(std::string &A, T &out) {
  if (A.empty())
    return 0;

  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx >> retval;
  const std::streamoff xpt = cx.tellg();
  if (xpt < 0)
    return 0;
  A.erase(0, static_cast<unsigned int>(xpt));
  out = retval;
  return 1;
}

//------------------------------------------------------------------------------------------------
/**
 *  Takes a character string and evaluates
 *  the first [typename T] object. The string is then filled with
 *  spaces upto the end of the [typename T] object
 *  @param out :: place for output
 *  @param cA :: char array for input and output.
 *  @return 1 on success 0 on failure
 */
template <typename T> int section(char *cA, T &out) {
  if (!cA)
    return 0;
  std::string sA(cA);
  const int item(section(sA, out));
  if (item) {
    strcpy(cA, sA.c_str());
    return 1;
  }
  return 0;
}

/**
 *  takes a character string and evaluates
 *  the first \<T\> object. The string is then filled with
 *  spaces upto the end of the \<T\> object
 *  @param out :: place for output
 *  @param A :: string for input and output.
 *  @return 1 on success 0 on failure
 */
template <typename T> int section(std::string &A, T &out) {
  if (A.empty())
    return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx >> retval;
  if (cx.fail())
    return 0;
  const std::streamoff xpt = cx.tellg();
  const auto xc = static_cast<char>(cx.get());
  if (!cx.fail() && !isspace(xc))
    return 0;
  A.erase(0, static_cast<unsigned int>(xpt));
  out = retval;
  return 1;
}

/**
 *  Takes a character string and evaluates
 *  the first [T] object. The string is then filled with
 *  spaces upto the end of the [T] object.
 *  This version deals with MCNPX numbers. Those
 *  are numbers that are crushed together like
 *  - 5.4938e+04-3.32923e-6
 *  @param out :: place for output
 *  @param A :: string for input and output.
 *  @return 1 on success 0 on failure
 */
template <typename T> int sectionMCNPX(std::string &A, T &out) {
  if (A.empty())
    return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx >> retval;
  if (!cx.fail()) {
    const std::streamoff xpt = cx.tellg();
    if (xpt < 0) {
      return 0;
    }
    const auto xc = static_cast<char>(cx.get());
    if (!cx.fail() && !isspace(xc) && (xc != '-' || xpt < 5)) {
      return 0;
    }
    A.erase(0, static_cast<unsigned int>(xpt));
    out = retval;
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------------------------
/**
 *  Takes a character string and evaluates
 *  the first [typename T] object. The string is then
 *  erase upto the end of number.
 *  The diffierence between this and convert is that
 *  it allows trailing characters after the number.
 *  @param out :: place for output
 *  @param A :: string to process
 *  @retval number of char read on success
 *  @retval 0 on failure
 */
template <typename T> int convPartNum(const std::string &A, T &out) {
  if (A.empty())
    return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx >> retval;
  // If we have reached the end of the stream, then we need to clear the error
  // as
  // it will cause the tellg() call to return -1.  Not pretty but works for now.
  cx.clear();
  const std::streamoff xpt = cx.tellg();
  if (xpt < 0)
    return 0;
  out = retval;
  return static_cast<int>(xpt);
}

//------------------------------------------------------------------------------------------------
/**
 *  Convert a string into a value
 *  @param A :: string to pass
 *  @param out :: value if found
 *  @return 0 on failure 1 on success
 */
template <typename T> int convert(const std::string &A, T &out) {
  if (A.empty())
    return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx >> retval;
  if (cx.fail())
    return 0;
  const auto clast = static_cast<char>(cx.get());
  if (!cx.fail() && !isspace(clast))
    return 0;
  out = retval;
  return 1;
}

//------------------------------------------------------------------------------------------------
/**
 *  Convert a string into a value
 *  @param A :: string to pass
 *  @param out :: value if found
 *  @return 0 on failure 1 on success
 */
template <typename T> int convert(const char *A, T &out) {
  // No string, no conversion
  if (!A)
    return 0;
  std::string Cx = A;
  return convert(Cx, out);
}

//------------------------------------------------------------------------------------------------
/** Convert a number or other value to a string
 *
 * @param value :: templated value (e.g. a double) to convert
 * @return a string
 */
template <typename T> std::string toString(const T &value) {
  std::ostringstream mess;
  mess << value;
  return mess.str();
}

/**
 * This assumes that the vector is sorted.
 *
 * @param value :: templated value (only works for integer types) to convert.
 * @return A reduced string representation.
 */
template <typename T> std::string toString(const std::vector<T> &value) {
  std::ostringstream mess;
  auto it = value.begin();
  auto last = value.end();
  T start;
  T stop;
  for (; it != last; ++it) {
    start = *(it);
    stop = start;
    for (; it != last; ++it) {
      if (it + 1 == last)
        break;
      else if ((stop + static_cast<T>(1)) == *(it + 1))
        stop = *(it + 1);
      else
        break;
    }
    mess << start;
    if (start != stop)
      mess << "-" << stop;
    if (it + 1 != last)
      mess << ",";
  }
  return mess.str();
}

template <typename T> std::string toString(const std::set<T> &value) {
  return toString(std::vector<T>(value.begin(), value.end()));
}

template <> MANTID_KERNEL_DLL std::string toString(const UnitLabel &value) { return value; }

/// Template overload for a vector of strings.
/// @param value :: A value to convert to a string.
/// @return :: A string with comma separated items of the value vector.
template <> MANTID_KERNEL_DLL std::string toString(const std::vector<std::string> &value) {
  return join(value.begin(), value.end(), ",");
}

//------------------------------------------------------------------------------------------------
/**
 *  Write out the three vectors into a file of type dc 9
 *  @param step :: parameter to control x-step (starts from zero)
 * @param Y :: Y column
 *  @param Fname :: Name of the file
 *  @return 0 on success and -ve on failure
 */
template <template <typename T, typename A> class V, typename T, typename A>
int writeFile(const std::string &Fname, const T &step, const V<T, A> &Y) {
  V<T, A> Ex; // Empty vector
  V<T, A> X;  // Empty vector
  for (unsigned int i = 0; i < Y.size(); i++)
    X.emplace_back(i * step);

  return writeFile(Fname, X, Y, Ex);
}

//------------------------------------------------------------------------------------------------
/**
 *  Write out the three vectors into a file of type dc 9
 *  @param X :: X column
 *  @param Y :: Y column
 *  @param Fname :: Name of the file
 *  @return 0 on success and -ve on failure
 */
template <template <typename T, typename A> class V, typename T, typename A>
int writeFile(const std::string &Fname, const V<T, A> &X, const V<T, A> &Y) {
  V<T, A> Ex;                        // Empty vector/list
  return writeFile(Fname, X, Y, Ex); // don't need to specific ??
}

//------------------------------------------------------------------------------------------------
/**
 *  Write out the three container into a file with
 *  column free-formated data in the form :
 *   - X  Y Err
 *   If Err does not exist (or is short) 0.0 is substituted.
 *  @param X :: X column
 *  @param Y :: Y column
 *  @param Err :: Err column
 *  @param Fname :: Name of the file
 *  @return 0 on success and -ve on failure
 */
template <template <typename T, typename A> class V, typename T, typename A>
int writeFile(const std::string &Fname, const V<T, A> &X, const V<T, A> &Y, const V<T, A> &Err) {
  const size_t Npts(X.size() > Y.size() ? Y.size() : X.size());
  const size_t Epts(Npts > Err.size() ? Err.size() : Npts);

  std::ofstream FX;

  FX.open(Fname.c_str());
  if (!FX.good())
    return -1;

  FX << "# " << Npts << " " << Epts << '\n';
  FX.precision(10);
  FX.setf(std::ios::scientific, std::ios::floatfield);
  auto xPt = X.cbegin();
  auto yPt = Y.cbegin();
  auto ePt = (Epts ? Err.cbegin() : Y.cbegin());

  // Double loop to include/exclude a short error stack
  size_t eCount = 0;
  for (; eCount < Epts; eCount++) {
    FX << (*xPt) << " " << (*yPt) << " " << (*ePt) << '\n';
    ++xPt;
    ++yPt;
    ++ePt;
  }
  for (; eCount < Npts; eCount++) {
    FX << (*xPt) << " " << (*yPt) << " 0.0\n";
    ++xPt;
    ++yPt;
  }
  FX.close();
  return 0;
}

//------------------------------------------------------------------------------------------------
/**
 *  Call to read in various values in position x1,x2,x3 from the
 *  line. Note to avoid the dependency on crossSort this needs
 *  to be call IN ORDER
 *  @param Line :: string to read
 *  @param Index :: Indexes to read
 *  @param Out :: OutValues [unchanged if not read]
 *  @retval 0 :: success
 *  @retval -ve on failure.
 */
template <typename T> int setValues(const std::string &Line, const std::vector<int> &Index, std::vector<T> &Out) {
  if (Index.empty())
    return 0;

  if (Out.size() != Index.size())
    return -1;
  //    throw ColErr::MisMatch<int>(Index.size(),Out.size(),
  //        "Mantid::Kernel::Strings::setValues");

  std::string modLine = Line;
  std::vector<int> sIndex(Index); // Copy for sorting
  std::vector<int> OPt(Index.size());
  for (unsigned int i = 0; i < Index.size(); i++)
    OPt[i] = i;

  //  mathFunc::crossSort(sIndex,OPt);

  using iVecIter = std::vector<int>::const_iterator;
  std::vector<int>::const_iterator sc = sIndex.begin();
  std::vector<int>::const_iterator oc = OPt.begin();
  int cnt(0);
  T value;
  std::string dump;
  while (sc != sIndex.end() && *sc < 0) {
    ++sc;
    ++oc;
  }

  while (sc != sIndex.end()) {
    if (*sc == cnt) {
      if (!section(modLine, value))
        return static_cast<int>(-1 - distance(static_cast<iVecIter>(sIndex.begin()), sc));
      // this loop handles repeat units
      do {
        Out[*oc] = value;
        ++sc;
        ++oc;
      } while (sc != sIndex.end() && *sc == cnt);
    } else {
      if (!section(modLine, dump))
        return static_cast<int>(-1 - distance(static_cast<iVecIter>(sIndex.begin()), sc));
    }
    cnt++; // Add only to cnt [sc/oc in while loop]
  }
  // Success since loop only gets here if sc is exhaused.
  return 0;
}

//-----------------------------------------------------------------------------------------------
/** Get a word from a line and strips spaces
 *
 * @param in :: stream input
 * @param consumeEOL :: set to true to remove the new lines at the end of the
 *line
 * @return a string with the word read in
 */
std::string getWord(std::istream &in, bool consumeEOL) {
  std::string ret;
  char nextch = 0;

  // Skip leading spaces
  do {
    nextch = static_cast<char>(in.get());
  } while (nextch == ' ');

  // Return an empty string on EOL; optionally consume it
  if (nextch == '\n' || nextch == '\r') {
    if (!consumeEOL) {
      in.unget();
    } else if ((nextch == '\n' && in.peek() == '\r') || (nextch == '\r' && in.peek() == '\n')) {
      // Handle CRLF and LFCR on Unix by consuming both
      in.ignore();
    }

    return ret;
  } else {      // Non-EOL and non-space character
    in.unget(); // Put it back on stream
  }

  // Get next word if stream is still valid
  if (in.good())
    in >> ret;

  // Optionally consume EOL character
  if (consumeEOL) {
    nextch = static_cast<char>(in.get());

    // Handle CRLF and LFCR on Unix by consuming both
    if (nextch == '\n' || nextch == '\r') {
      if ((nextch == '\n' && in.peek() == '\r') || (nextch == '\r' && in.peek() == '\n')) {
        in.ignore();
      }
    } else {
      in.unget();
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------------------------
/** Read up to the eol
 *
 * @param in :: stream input
 * @param ConsumeEOL :: set to true to remove the new lines at the end of the
 *line
 */
void readToEndOfLine(std::istream &in, bool ConsumeEOL) {
  while (in.good() && getWord(in, false).length() > 0)
    getWord(in, false);
  if (!ConsumeEOL)
    return;
  getWord(in, true);
}

/**
 * Function parses a path, placed into input string "path" and returns vector of
 * the folders contributed into the path
 *  @param path :: the string containing input path, found in path string, if
 * they are separated by \ or / symbols.
 *  Treats special symbols, if defined in the input string as path-es
 *  returns 0 for empty input string
 *  @param path_components :: holder for the individual folders in the path
 *  used to generate path in hdf file, so the resulting path has to obey hdf
 * constrains;
 */
size_t split_path(const std::string &path, std::vector<std::string> &path_components) {
  if (path.empty()) {
    path_components.resize(0);
    return 0;
  }
  // convert Windows path into the unix one
  std::string working_path(path);
  for (size_t i = 0; i < path.size(); i++) {
    if (working_path[i] < 0x20 || working_path[i] > 0x7E)
      working_path[i] = '_';
    if (working_path[i] == '\\')
      working_path[i] = '/';
    if (working_path[i] == ' ')
      working_path[i] = '_';
  }

  // path start with relative character, and we need to convert it into full
  // path
  if (path[0] == '.') {
    // get absolute path using working directory as base;
    Poco::Path absol;
    absol = absol.absolute();
    working_path = absol.toString(Poco::Path::PATH_UNIX) + working_path;
  }
  // as poco splt using regular expressions is doing some rubbish, we need to do
  // split manually
  // code below implements perl split(/\\//,string) commamd. (\\ has been
  // converted to / above)
  std::list<int64_t> split_pos;
  split_pos.emplace_back(-1);
  size_t path_size = working_path.size();
  for (size_t i = 0; i < path_size; i++) {
    if (working_path[i] == '/') {
      split_pos.emplace_back(i);
    }
  }
  split_pos.emplace_back(path_size);
  // allocate target vector to keep folder structure and fill it in
  size_t n_folders = split_pos.size() - 1;
  path_components.resize(n_folders);
  auto it1 = split_pos.begin();
  auto it2 = it1;
  ++it2;

  int64_t ic(0);
  for (; it2 != split_pos.end(); ++it2) {
    std::string folder = working_path.substr(*it1 + 1, *it2 - *it1 - 1);
    if (folder.empty() || (folder.size() == 1 && folder == ".")) { // skip self-references and double slashes;
      it1 = it2;
      continue;
    }
    // reprocess up-references;
    if (folder == "..") {
      ic--;
      if (ic < 0)
        throw(std::invalid_argument("path contains relative references to a "
                                    "folder outside of the seach tree"));
      it1 = it2;
      continue;
    }
    path_components[ic] = folder;
    ic++;
    it1 = it2;
  }

  n_folders = size_t(ic);
  path_components.resize(n_folders);
  return n_folders;
}

/**
 * Function checks if the candidate is the member of the group
 * @param group :: vector of string to check
 * @param candidate :: the string which has to be checked against the group
 * @returns :: number of the candidate in the input vector of strings if the
 candidate belongs to the group
               or -1 if it does not.
               Returns the number of the first maching entry in the group if
 there are duplicated entries in the group
 */
int isMember(const std::vector<std::string> &group, const std::string &candidate) {
  int num(-1);
  for (size_t i = 0; i < group.size(); i++) {
    if (candidate == group[i]) {
      num = int(i);
      return num;
    }
  }
  return num;
}

/**
 * Parses a number range, e.g. "1,4-9,54-111,3,10", to the vector containing all
 * the elements
 * within the range.
 * @param str String to parse
 * @param elemSep String with characters used to separate elements (',')
 * @param rangeSep String with characters used to separate range start and end
 * ('-')
 * @return A vector with all the elements from the range
 */
std::vector<int> parseRange(const std::string &str, const std::string &elemSep, const std::string &rangeSep) {
  using Tokenizer = Mantid::Kernel::StringTokenizer;

  Tokenizer elements;

  if (elemSep.find(' ') != std::string::npos) {
    // If element separator contains space character it's a special case,
    // because in that case
    // it is allowed to have element separator inside a range, e.g. "4 - 5", but
    // not "4,-5"
    Tokenizer ranges(str, rangeSep, Tokenizer::TOK_TRIM);
    std::string new_str = join(ranges.begin(), ranges.end(), rangeSep.substr(0, 1));
    elements = Tokenizer(new_str, elemSep, Tokenizer::TOK_IGNORE_EMPTY | Tokenizer::TOK_TRIM);
  } else {
    elements = Tokenizer(str, elemSep, Tokenizer::TOK_IGNORE_EMPTY | Tokenizer::TOK_TRIM);
  }

  std::vector<int> result;

  // Estimation of the resulting number of elements
  result.reserve(elements.count());

  for (const auto &elementString : elements) {
    // See above for the reason space is added
    Tokenizer rangeElements(elementString, rangeSep, Tokenizer::TOK_TRIM);

    size_t noOfRangeElements = rangeElements.count();

    // A single element
    if (noOfRangeElements == 1) {
      int element;
      if (convert(rangeElements[0], element) != 1)
        throw std::invalid_argument("Invalid element: " + elementString);
      result.emplace_back(element);
    }
    // A pair
    else if (noOfRangeElements == 2) {
      int start, end;

      if (convert(rangeElements[0], start) != 1 || convert(rangeElements[1], end) != 1)
        throw std::invalid_argument("Invalid range: " + elementString);

      if (start >= end)
        throw std::invalid_argument("Range boundaries are reversed: " + elementString);

      for (int i = start; i <= end; i++)
        result.emplace_back(i);
    }
    // Error - e.g. "--""
    else {
      throw std::invalid_argument("Multiple range separators: " + elementString);
    }
  }

  return result;
}

/**
 * Extract a string until an EOL character is reached. There are 3 scenarios
 * that we need to deal with 1) Windows-style  - CRLF ('\\r\\n'); 2) Unix-style
 * - LF ('\\n'); 3) Old MAC style  - CR ('\\r'). This function will give the
 * string preceding any of these sequences
 * @param is :: The input stream to read from
 * @param str :: The output string to use to accumulate the line
 * @returns A reference to the input stream
 */
std::istream &extractToEOL(std::istream &is, std::string &str) {
  // Empty the string
  str = "";
  char c('\0');
  while (is.get(c)) {
    if (c == '\r') {
      c = static_cast<char>(is.peek());
      if (c == '\n') {
        // Extract this as well
        is.get();
      }
      break;
    } else if (c == '\n') {
      break;
    } else {
      // Accumulate the string
      str += c;
    }
  }
  return is;
}

//------------------------------------------------------------------------------------------------
std::string randomString(size_t len) {
  static const std::string alphabet = "0123456789abcdefghijklmnopqrstuvwxyz";

  std::string result;
  result.reserve(len);

  while (result.size() != len) {
    size_t randPos = ((rand() % (alphabet.size() - 1)));
    result.push_back(alphabet[randPos]);
  }

  return result;
}

/// \cond TEMPLATE
template MANTID_KERNEL_DLL int section(std::string &, double &);
template MANTID_KERNEL_DLL int section(std::string &, float &);
template MANTID_KERNEL_DLL int section(std::string &, int &);
template MANTID_KERNEL_DLL int section(std::string &, std::string &);

template MANTID_KERNEL_DLL int sectPartNum(std::string &, double &);
template MANTID_KERNEL_DLL int sectPartNum(std::string &, int &);
template MANTID_KERNEL_DLL int sectionMCNPX(std::string &, double &);

template MANTID_KERNEL_DLL int convert(const std::string &, double &);
template MANTID_KERNEL_DLL int convert(const std::string &, float &);
template MANTID_KERNEL_DLL int convert(const std::string &, std::string &);
template MANTID_KERNEL_DLL int convert(const std::string &, int &);
template MANTID_KERNEL_DLL int convert(const std::string &, std::size_t &);
template MANTID_KERNEL_DLL int convert(const std::string &, bool &);
template MANTID_KERNEL_DLL int convert(const char *, std::string &);
template MANTID_KERNEL_DLL int convert(const char *, double &);
template MANTID_KERNEL_DLL int convert(const char *, int &);
template MANTID_KERNEL_DLL int convert(const char *, std::size_t &);
template MANTID_KERNEL_DLL int convert(const char *, bool &);

template MANTID_KERNEL_DLL std::string toString(const double &value);
template MANTID_KERNEL_DLL std::string toString(const float &value);
template MANTID_KERNEL_DLL std::string toString(const int &value);
template MANTID_KERNEL_DLL std::string toString(const uint16_t &value);
template MANTID_KERNEL_DLL std::string toString(const size_t &value); // Matches uint64_t on Linux 64 & Win 64
#if defined(__APPLE__) || (defined(_WIN32) && !defined(_WIN64)) ||                                                     \
    (defined(__GNUC__) && !defined(__LP64__)) // Mac or 32-bit compiler
template MANTID_KERNEL_DLL std::string toString(const uint64_t &value);
#endif
template MANTID_KERNEL_DLL std::string toString(const std::string &value);

template MANTID_KERNEL_DLL std::string toString(const std::vector<int> &value);
template MANTID_KERNEL_DLL std::string toString(const std::vector<size_t> &value);

// this block should generate the vector ones as well
template MANTID_KERNEL_DLL std::string toString(const std::set<int> &value);
template MANTID_KERNEL_DLL std::string toString(const std::set<int16_t> &value);
template MANTID_KERNEL_DLL std::string toString(const std::set<size_t> &value); // Matches uint64_t on Linux 64 & Win 64
#if defined(__APPLE__) || (defined(_WIN32) && !defined(_WIN64)) ||                                                     \
    (defined(__GNUC__) && !defined(__LP64__)) // Mac or 32-bit compiler
template MANTID_KERNEL_DLL std::string toString(const std::set<uint64_t> &value);
#endif

template MANTID_KERNEL_DLL int convPartNum(const std::string &, double &);
template MANTID_KERNEL_DLL int convPartNum(const std::string &, int &);

template MANTID_KERNEL_DLL int setValues(const std::string &, const std::vector<int> &, std::vector<double> &);

template MANTID_KERNEL_DLL int writeFile(const std::string &, const double &, const std::vector<double> &);
template MANTID_KERNEL_DLL int writeFile(const std::string &, const std::vector<double> &, const std::vector<double> &,
                                         const std::vector<double> &);
template MANTID_KERNEL_DLL int writeFile(const std::string &, const std::vector<double> &, const std::vector<double> &);
template MANTID_KERNEL_DLL int writeFile(const std::string &, const std::vector<float> &, const std::vector<float> &);
template MANTID_KERNEL_DLL int writeFile(const std::string &, const std::vector<float> &, const std::vector<float> &,
                                         const std::vector<float> &);
/// \endcond TEMPLATE

} // namespace Mantid::Kernel::Strings
