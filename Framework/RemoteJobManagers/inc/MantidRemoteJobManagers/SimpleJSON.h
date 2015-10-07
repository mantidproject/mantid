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

#ifndef SIMPLEJSON_H
#define SIMPLEJSON_H

#include <map>
#include <string>
#include <vector>
#include <istream>
#include <ostream>

class JSONValue;
typedef std::map<std::string, JSONValue> JSONObject;
typedef std::vector<JSONValue> JSONArray;
// Note: according to the JSON spec, an array is a type of value.
// That isn't strictly true in the C++ sense here (ie: JSONArray
// doesn't inherit from JSONValue), but I think we'll be all right.

// This is the "public" initialization function.  Since JSONObject
// is just a typedef, there's no way to make it a constructor.
void initFromStream(JSONObject &obj, std::istream &istr);

// A "public" function for formatted output.  It's sort of assumed
// that ostr will actually be std::cout or std::cerr, but it can
// be any output stream.  This function mostly exists for debugging
// purposes.
void prettyPrint(const JSONObject &obj, std::ostream &ostr,
                 unsigned indentLevel);

class JSONException;

class JSONValue {
public:
  enum VALUE_TYPE { NULLTYPE, BOOL, NUMBER, STRING, ARRAY, OBJECT };

  JSONValue();       // Initialize with the NULL value
  JSONValue(bool v); // Initialize w/ true or false
  JSONValue(double v);
  JSONValue(const std::string &v);
  JSONValue(const JSONArray &v);  // Initialize w/ an array
  JSONValue(const JSONObject &v); // Initialize w/ another JSON object

  JSONValue(std::istream &istr); // Initialize from a stream (and presumably
                                 // create a whole hierarchy)

  // Formatted output to a stream - presumably cout.  (Mostly for debugging
  // purposes)
  void prettyPrint(std::ostream &ostr, unsigned indentLevel) const;

  // Destructor, copy constructor and assignment operator
  ~JSONValue();
  JSONValue(const JSONValue &v);
  JSONValue &operator=(const JSONValue &v);

  // Accessors...
  VALUE_TYPE getType() const { return m_type; }

  bool getValue(bool &v) const;
  bool getValue(double &v) const;
  bool getValue(std::string &v) const;
  bool getValue(JSONArray &v) const;
  bool getValue(JSONObject &v) const;
  // If the object does not contain the requested type, then the accessor
  // functions
  // return false and leave v unchanged.

private:
  void assignmentOpHelper(); // Used by operator= (and by no-one else)

  VALUE_TYPE m_type;

  // This is where the actual value is stored
  union {
    bool m_bool;
    double m_num;
    std::string *mp_string;
    JSONArray *mp_array;
    JSONObject *mp_object;
  };
};

class JSONException : public std::exception {
public:
  JSONException(const std::string &msg) : m_msg(msg) {}
  const std::string &getMsg() const { return m_msg; }

  // Default constructor, copy constructor & assignment operator are fine

  virtual ~JSONException() throw() {}

private:
  std::string m_msg;
};

class JSONCopyException : public JSONException {
public:
  JSONCopyException(const std::string &msg) : JSONException(msg) {}
};

class JSONAssignmentException : public JSONException {
public:
  JSONAssignmentException(const std::string &msg) : JSONException(msg) {}
};

class JSONParseException : public JSONException {
public:
  JSONParseException(const std::string &msg) : JSONException(msg) {}
};

#endif
