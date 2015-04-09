/*******************************************************************
  A cross-platform JSON parser that uses nothing more than C++ and
  STL templates.  It's probably slower than other JSON parsers, but
  it's a heck of a lot smaller and simpler and works on Linux, MacOS
  and Windows.

  I think it completely implements the JSON spec, but all I'm really
  concerned with is whether it can parse the output from Moab Web
  Services.

  RGM - 23 July 2012
  ******************************************************************/

#include "MantidRemoteJobManagers/SimpleJSON.h"

#include <algorithm> // for transform() function
#include <sstream>
#include <map>
using namespace std;

JSONValue::JSONValue() : m_type(JSONValue::NULLTYPE) {}
JSONValue::JSONValue(bool v) : m_type(JSONValue::BOOL), m_bool(v) {}
JSONValue::JSONValue(double v) : m_type(JSONValue::NUMBER), m_num(v) {}

JSONValue::JSONValue(const string &v) : m_type(JSONValue::STRING) {
  mp_string = new string(v);
}

JSONValue::JSONValue(const JSONArray &v) : m_type(JSONValue::ARRAY) {
  mp_array = new JSONArray(v);
}

JSONValue::JSONValue(const JSONObject &v) : m_type(JSONValue::OBJECT) {
  mp_object = new JSONObject(v);
}

JSONValue::~JSONValue() {
  switch (m_type) {
  case JSONValue::STRING:
    delete mp_string;
    break;

  case JSONValue::ARRAY:
    delete mp_array;
    break;

  case JSONValue::OBJECT:
    delete mp_object;
    break;

  default:
    // Do nothing
    break;
  }
}

JSONValue::JSONValue(const JSONValue &v) {
  m_type = v.getType();

  switch (m_type) {
  case JSONValue::NULLTYPE:
    break; // nothing to do
  case JSONValue::BOOL:
    if (!v.getValue(m_bool))
      throw(JSONCopyException("Failed to copy boolean"));
    break;
  case JSONValue::NUMBER:
    if (!v.getValue(m_num))
      throw(JSONCopyException("Failed to copy float"));
    break;
  case JSONValue::STRING:
    mp_string = new string;
    if (!v.getValue(*mp_string)) {
      delete mp_string; // Make sure we don't leak memory in the event of a
                        // failure
      throw(JSONCopyException("Failed to copy string"));
    }
    break;
  case JSONValue::ARRAY:
    mp_array = new JSONArray;
    if (!v.getValue(*mp_array)) {
      delete mp_array;
      throw(JSONCopyException("Failed to copy array"));
    }
    break;
  case JSONValue::OBJECT:
    mp_object = new JSONObject;
    if (!v.getValue(*mp_object)) {
      delete mp_object;
      throw(JSONCopyException("Failed to copy object"));
    }
    break;
  default:
    // Should never hit this!
    throw(JSONCopyException("Unknown JSON type!!"));
  }
}

JSONValue &JSONValue::operator=(const JSONValue &v) {
  // This gets a little tricky:  In the case where v's type is different from
  // our own, we need to make sure we don't stomp on any of the existing
  // pointers
  // until we're sure the getValue() call has succeeded.  Once getValue() does
  // succeeed, we may need to delete the existing memory.
  // Note: This also allows the code to handle the case of self-assignment.

  JSONValue::VALUE_TYPE newType = v.getType();

  // Temporary values (only one of which will actually get used, but gcc
  // complains if new variables are declared down inside a case statement)
  bool tBool;
  double tFloat;
  std::string *tpString;
  JSONArray *tpArray;
  JSONObject *tpObject;

  switch (newType) {
  case JSONValue::NULLTYPE:
    assignmentOpHelper();
    m_type = newType;
    break;

  case JSONValue::BOOL:
    if (v.getValue(tBool)) {
      assignmentOpHelper();
      m_bool = tBool;
      m_type = newType;
    } else
      throw(JSONAssignmentException("Failed to assign boolean"));
    break;

  case JSONValue::NUMBER:
    if (v.getValue(tFloat)) {
      assignmentOpHelper();
      m_num = tFloat;
      m_type = newType;
    } else
      throw(JSONAssignmentException("Failed to assign float"));
    break;

  case JSONValue::STRING:
    tpString = new string;
    if (v.getValue(*tpString)) {
      assignmentOpHelper();
      mp_string = tpString;
      m_type = newType;
    } else {
      delete tpString; // Make sure we don't leak memory in the event of a
                       // failure
      throw(JSONAssignmentException("Failed to assign string"));
    }
    break;

  case JSONValue::ARRAY:
    tpArray = new JSONArray;
    if (v.getValue(*tpArray)) {
      assignmentOpHelper();
      mp_array = tpArray;
      m_type = newType;
    } else {
      delete tpArray;
      throw(JSONAssignmentException("Failed to assign array"));
    }
    break;

  case JSONValue::OBJECT:
    tpObject = new JSONObject;
    if (v.getValue(*tpObject)) {
      assignmentOpHelper();
      mp_object = tpObject;
      m_type = newType;
    } else {
      delete tpObject;
      throw(JSONAssignmentException("Failed to assign object"));
    }
    break;

  default:
    // Should never hit this!
    throw(JSONAssignmentException("Unknown JSON type!!"));
  }

  return *this;
}

// A helper function for the assignment operator
// Its job is to delete memory holding the current value (in cases  where it is
// a
// pointer, ie: strings, arrays & objects);
// It assumed this will be called after the call to getValue() succeeds, but
// before
// the new value is copied in to the appropriate slot in the union.
void JSONValue::assignmentOpHelper() {
  switch (m_type) {
  case JSONValue::STRING:
    delete mp_string;
    break;
  case JSONValue::ARRAY:
    delete mp_array;
    break;
  case JSONValue::OBJECT:
    delete mp_object;
    break;
  default:
    break; // do nothing
  }
}

bool JSONValue::getValue(bool &v) const {
  if (m_type != JSONValue::BOOL)
    return false;

  v = m_bool;
  return true;
}

bool JSONValue::getValue(double &v) const {
  if (m_type != JSONValue::NUMBER)
    return false;

  v = m_num;
  return true;
}

bool JSONValue::getValue(std::string &v) const {
  // Since booleans and numbers can be easily converted to strings,
  // we'll make this function a little smarter and have it do the
  // conversion if necessary (instead of just returning false)
  bool rv = true; // assume success
  std::ostringstream convert;
  switch (m_type) {
  case JSONValue::STRING:
    v = *mp_string;
    break;

  case JSONValue::NUMBER:
    convert << m_num << std::flush;
    v = convert.str();
    break;

  case JSONValue::BOOL:
    if (m_bool)
      v = "true";
    else
      v = "false";
    break;

  default:
    rv = false;
  }

  return rv;
}

bool JSONValue::getValue(JSONArray &v) const {
  if (m_type != JSONValue::ARRAY)
    return false;

  v = *mp_array;
  return true;
}

bool JSONValue::getValue(JSONObject &v) const {
  if (m_type != JSONValue::OBJECT)
    return false;

  v = *mp_object;
  return true;
}

// Formatted output to a stream (presumably cout)
void JSONValue::prettyPrint(ostream &ostr, unsigned indentLevel) const {
  JSONArray::const_iterator it;

  switch (m_type) {
  case NULLTYPE:
    ostr << "NULL";
    break;

  case BOOL:
    ostr << (m_bool ? "TRUE" : "FALSE");
    break;

  case NUMBER:
    ostr << m_num;
    break;

  case STRING:
    ostr << "\"" << *mp_string << "\"";
    break;

  case ARRAY:

    if (mp_array->size() <= 1) {
      // Special handling for small arrays - print on one line
      ostr << "[ ";
      it = mp_array->begin();
      if (it != mp_array->end()) {
        it->prettyPrint(ostr, indentLevel + 1);
      }
      ostr << " ]";
    } else {
      ostr << "[" << endl;

      it = mp_array->begin();
      while (it != mp_array->end()) {
        for (unsigned i = 0; i < indentLevel + 1; i++) {
          ostr << "\t";
        }
        it->prettyPrint(ostr, indentLevel + 1);
        ostr << endl;
        ++it;
      }

      for (unsigned i = 0; i < indentLevel + 1; i++) {
        ostr << "\t";
      }
      ostr << "]";
    }
    break;

  case OBJECT:
    if (mp_object->size() <= 1) {
      // special handling for small objects - print on one line
      ostr << "{ ";
      ::prettyPrint(*mp_object, ostr, 0);
      ostr << " }";

    } else {
      ostr << "{" << endl;
      ::prettyPrint(*mp_object, ostr, indentLevel + 1);

      for (unsigned i = 0; i < indentLevel + 1; i++) {
        ostr << "\t";
      }
      ostr << "}";
    }
    break;

  default:
    ostr << "<UNKNOWN TYPE!! (This should never happen.)>";
  }
}

// ----- End of JSONValue member definitions ----------

// Prototypes for some 'private' helper functions that initFromStream()
// may use (either directly or indirectly)
void skipWhiteSpace(istream &istr);
void checkChar(char found, char expected);
string readString(istream &istr);
string readUntilCloseChar(istream &istr);
void initArrayFromStream(JSONArray &arr, istream &istr);
JSONValue initValueFromStream(istream &istr);

// Initialize a JSON object from a stream (presumably creating a whole
// hierarchy)
//
// This is the big one. :)  The expectation is that the first character
// will be a '{' and the function will run until if finds a matching '}'
// char.  Along the way, it may create nested objects and/or arrays
// (which means it may be called recursively - by way of
// initValueFromStream())
// Note: The function will consume the closing brace from the stream
void initFromStream(JSONObject &obj, istream &istr) {
  char nextChar;
  istr >> nextChar;
  checkChar(nextChar, '{'); // sanity check
  skipWhiteSpace(istr);

  // Check for empty object (and make sure we consume the })
  nextChar = (char)istr.peek();
  if (nextChar == '}') {
    istr.ignore();
  }

  while (nextChar != '}') // process the stream
  {
    // Quick sanity check
    if (istr.eof()) {
      throw JSONParseException("Unexpected end of data stream");
    }

    // We expect to start the loop with the stream pointing to the opening quote
    // of the key
    nextChar = (char)istr.peek();
    checkChar(nextChar, '"');

    string key = readString(istr);
    istr >> nextChar;         // >> operator automatically skips white space
    checkChar(nextChar, ':'); // the separator between the key and the value

    skipWhiteSpace(istr);

    // Now. we're at the start of the value.
    // Add the key and value to our object
    obj[key] = initValueFromStream(istr);
    istr >> nextChar;
    // nextChar is guaranteed to be either a comma, close brace or close
    // bracket.
    //(If it was anything else, initValueFromStream() would have thrown an
    // exception.)
    // A bracket is an error, a brace means the object is done (and will be
    // checked at
    // the start of the while loop) and a comma needs to be thrown out (along
    // with any
    // following whitespace) to position us for the next key/value pair
    if (nextChar == ']')
      throw JSONParseException(
          "Invalid closing bracket while initializing object");
    else if (nextChar == ',') {
      skipWhiteSpace(istr);
      // Check to see if another key/value pair really follows the comma
      // (because if one doesn't, the parser will get screwed up and may not
      // actually detect the problem).
      if (istr.peek() != '"') {
        throw JSONParseException(
            "Invalid comma (no key/value pair following it)");
      }
    }
  }
}

// Initialize a JSON array from a stream
// This is similar to initFromStream() above and may also be called
// recursively by way of initValueFromStream()
// The expectation is that the first character will be a '[' and it
// will run until if finds a matching ']' char.  Along the way it
// may create nested objects and/or arrays.
// Note: It will consume the closing bracket from the stream
void initArrayFromStream(JSONArray &arr, istream &istr) {
  char nextChar;
  istr >> nextChar;
  checkChar(nextChar, '['); // sanity check
  skipWhiteSpace(istr);

  // Check for empty array (and make sure we consume the ])
  nextChar = (char)istr.peek();
  if (nextChar == ']') {
    istr.ignore();
  }

  while (nextChar != ']') // process the stream
  {
    // Quick sanity check
    if (istr.eof()) {
      throw JSONParseException("Unexpected end of data stream");
    }

    // We expect to start the loop with the stream pointing to the
    // first character of the value
    // Add the value to our array
    arr.push_back(initValueFromStream(istr));

    istr >> nextChar;
    // nextChar is guaranteed to be either a comma, close brace or close
    // bracket.
    //(If it was anything else, initValueFromStream() would have thrown an
    // exception.)
    // A brace is an error, a bracket means the array is done (and will be
    // checked at
    // the start of the while loop) and a comma needs to be thrown out (along
    // with any
    // following whitespace) to position us for the next value
    if (nextChar == '}')
      throw JSONParseException(
          "Invalid closing brace while initializing array");
    else if (nextChar == ',') {
      skipWhiteSpace(istr);
      // Check to see if another key/value pair really follows the comma
      // (because if one doesn't, the parser will get screwed up and may not
      // actually detect the problem).
      if (istr.peek() == ']') {
        throw JSONParseException(
            "Invalid comma (array ended with no further values)");
      }
    }
  }
}

// Initialize a single JSONValue from an input stream.  Note that JSONObject
// and JSONArray are both valid values, and this function may call
// initFromStream()
// and initArrayFromStream().
// Function expects the stream to be pointing at the first character in the
// value
JSONValue initValueFromStream(istream &istr) {
  JSONValue value;
  // We expect the stream to be at the start of the value.

  // Need to determine what kind of value it is.
  char nextChar = (char)istr.peek();
  if (nextChar == '"') // value is a string
  {
    // Read until we get the closing '"'
    value = JSONValue(readString(istr));
  } else if (nextChar == '[') // value is an array of stuff
  {
    JSONArray newArray;
    initArrayFromStream(newArray, istr); // Initialize the array...
    value = JSONValue(newArray);
  } else if (nextChar == '{') // value is another JSON object
  {
    JSONObject newJsonObj;
    initFromStream(newJsonObj, istr); // Initialize the object...
    value = JSONValue(newJsonObj);
  } else {
    // Now it gets a little trickier.  It's either a number or the special
    // values
    // true, false or null (case insensitive)
    // Read until we find the comma, closing bracket or closing brace
    string val = readUntilCloseChar(istr);
    std::transform(val.begin(), val.end(), val.begin(),
                   ::tolower); // convert to lower case for easy comparing
    if (val == "false")
      value = JSONValue(false);
    else if (val == "true")
      value = JSONValue(true);
    else if (val == "null")
      value = JSONValue(); // actually, this assignment isn't necessary - value
                           // is already null
    else {
      // At this point, the only valid option is a number of some kind...
      istringstream numericValue(val);
      double theNumber;
      numericValue >> theNumber;
      // check to see if there are any characters let in numbericValue.
      // If there are, it was an invalid value
      if (!numericValue.eof()) {
        string msg = "Invalid characters in a numeric value: ";
        msg += numericValue.str();
        throw JSONParseException(msg);
      }

      value = JSONValue(theNumber);
    }
  }

  // Done processing the value. Verify that it ends properly (ie, we
  // get a comma or a closing brace)
  skipWhiteSpace(istr);
  nextChar = (char)istr.peek();
  if ((nextChar != ',') && (nextChar != '}') && (nextChar != ']')) {
    string message = "Improperly terminated key/value pair.  Expected comma or "
                     "closing brace.  Received: ";
    message += nextChar;
    message.append("\n");
    char tempBuf[64];
    istr.read(tempBuf, 63);
    tempBuf[63] = '\0';
    message.append("Remaining stream: ");
    message.append(tempBuf);
    throw(JSONParseException(message));
  }

  return value;
}

// Consume whitespace characters from stream.  Leaves the stream
// pointing at the next non-whitespace char (or possibly eof());
void skipWhiteSpace(istream &istr) {

  char next;
  istr >> next; // take advantage of the fact that >> uses whitespace as a field
                // separator
  if (!istr.eof()) {
    // If eof() is set, it means we didn't find any tokens after the whitespace.
    // In this case
    // there's nother to put back, and doing so would unset the eof bit.
    istr.putback(next);
  }
}

void checkChar(char found, char expected) {
  if (found != expected) {
    string msg = "Was expecting ";
    msg += expected;
    msg += " char, but received ";
    msg += found;
    throw(JSONParseException(msg));
  }
}

// Expects istr to be pointing at the first " of a string (either a key or
// a value of type string).  Reads until the closing " and returns the
// characters between as a string. It consumes the closing " and leaves the
// stream pointing at the character that follows it.
string readString(istream &istr) {
  string str;
  char next;
  istr.get(next);
  checkChar(next, '"');
  // Note: can't use operator>> here, becuase whitespace is significant
  istr.get(next);
  while (next != '"') {
    // Check for escaped chars...
    if (next != '\\') {
      str += next; // character isn't escaped, so just append to the string
    } else {
      istr.get(next);
      switch (next) {
      case 't':
        str += '\t';
        break;

      case 'n':
        str += '\n';
        break;

      case 'r':
        str += '\r';
        break;

      case 'b':
        str += '\b';
        break;

      case 'f':
        str += '\f';
        break;

      case '\\':
        str += '\\';
        break;

      case '/':
        str += '/';
        break;

      case '"':
        str += '"';
        break;

      case 'u':
        throw JSONParseException("Parser cannot handle the \\u<hex> notation");
        break;

      default:
        throw JSONParseException(string("Unknown escape value: \\") + next);
      }
    }

    istr.get(next);
    if (istr.eof()) {
      throw JSONParseException(
          "Stream unexpectedly ended without a closing quote.");
    }
  }
  return str;
}

// reads chars from the stream until one of the closing chars is found
// (either a comma, closing bracket or closing brace).  The closing char
// is NOT consumed.  Function assumes the stream is pointing at the
// first character of the value.
// Note: This function is not used for strings.  See readString() for that.
string readUntilCloseChar(istream &istr) {
  string value;
  char next = (char)istr.peek();
  while ((next != ',') && (next != '}') && (next != ']')) {
    if (istr.eof()) {
      throw JSONParseException(
          "Stream unexpectedly ended without a closing char.");
    }

    if ((value.size() > 0) ||
        (!isspace(
             next))) // don't add white space to the start of the value string
    {
      value += next;
    }
    istr.get(); // consume the char from the stream
    next = (char)istr.peek();
  }

  // Strip any whitespace off the end of the value string
  while (isspace(value[value.size() - 1])) {
    value.resize(value.size() - 1);
  }

  return value;
}

void prettyPrint(const JSONObject &obj, std::ostream &ostr,
                 unsigned indentLevel) {
  // Prints keys/value pairs.  One pair per line  (Does not print opening or
  // closing braces...)
  JSONObject::const_iterator it = obj.begin();
  while (it != obj.end()) {
    for (unsigned i = 0; i < indentLevel; i++) {
      ostr << "\t";
    }
    ostr << (*it).first << " : ";
    (*it).second.prettyPrint(ostr, indentLevel);
    if (obj.size() > 1) {
      // if there's only one key/value pair in the object, then don't print
      // a trailing newline.  (The rationale being that such small objects
      // will be printed with their key, value and braces all on one line.)
      ostr << endl;
    }
    ++it;
  }
  ostr.flush();
}
