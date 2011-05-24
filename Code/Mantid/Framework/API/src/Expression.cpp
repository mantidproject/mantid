#include "MantidAPI/Expression.h"

#include <Poco/StringTokenizer.h>

#include <sstream>
#include <iostream>
#include <locale>

namespace Mantid
{
namespace API
{

typedef Poco::StringTokenizer tokenizer;

Expression::Expression()
{

  m_operators.reset(new Operators());
  // Define binary operators. Put them in the reverse precedence order (from lower to higher prec.)
  std::vector<std::string> ops;
  ops.push_back(";");
  ops.push_back(",");
  ops.push_back("=");
  ops.push_back("== != > < <= >=");
  ops.push_back("&& || ^^");
  ops.push_back("+ -");
  ops.push_back("* /");
  ops.push_back("^");

  add_operators(ops);

  // Define unary operators
  std::set<std::string> unary;
  unary.insert("+");
  unary.insert("-");

  add_unary(unary);
}

/// contructor
Expression::Expression(const std::vector<std::string>& ops)
{
  m_operators.reset(new Operators());
  add_operators(ops);
}

/// contructor
Expression::Expression(const std::vector<std::string>& binary,const std::set<std::string>& unary)
{
  m_operators.reset(new Operators());
  add_operators(binary);
  add_unary(unary);
}

Expression::Expression(const Expression& expr)
:
//m_tokens(expr.m_tokens),
//m_expr(expr.m_expr),
m_funct(expr.m_funct),
m_op(expr.m_op),
m_terms(expr.m_terms),
m_operators(expr.m_operators)
{
}
Expression::Expression(const Expression* pexpr)
:m_operators(pexpr->m_operators)
{
}

/// Assignment operator
Expression& Expression::operator=(const Expression& expr)
{
  m_operators = expr.m_operators;
  m_funct = expr.m_funct;
  m_op = expr.m_op;
  m_terms = expr.m_terms;
  //m_expr = expr.m_expr;
  //m_tokens = expr.m_tokens;
  return *this;
}

void Expression::add_operators(const std::vector<std::string>& ops)
{
  m_operators->binary = ops;
  // Fill in the precedence table (m_op_precedence)
  for(size_t i=0;i<m_operators->binary.size();i++)
  {
    char j = 0;
    tokenizer tkz(m_operators->binary[i], " ", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
    for(tokenizer::Iterator it = tkz.begin();it != tkz.end(); it++ )
    {
      m_operators->precedence[*it] = i + 1;
      m_operators->op_number[*it] = j++;
    }
  }

  for(size_t i=0;i<ops.size();i++)
  {
    std::string str = ops[i];
    for(size_t j=0;j<str.size();j++)
    {
      char c = str[j];
      if ( c == ' ' ) continue;
      m_operators->symbols.insert(c);
    }
  }

}

void Expression::add_unary(const std::set<std::string>& ops)
{
  m_operators->unary = ops;
  for(std::set<std::string>::const_iterator it=ops.begin();it!=ops.end();it++)
  {
    for(std::string::const_iterator c=it->begin();c!=it->end();c++)
    {
      m_operators->symbols.insert(*c);
    }
  }
}

size_t Expression::op_prec(const std::string& op)const
{
  std::map<std::string, size_t>::const_iterator i = m_operators->precedence.find(op);
  if (i == m_operators->precedence.end()) return 0;
  return i->second;
}

bool Expression::is_unary(const std::string& op)const
{
  return m_operators->unary.count(op) != 0;
}

bool Expression::is_op_symbol(const char c)const
{
  return m_operators->symbols.count(c) != 0;
}

void Expression::trim(std::string& str)
{
  size_t i = str.find_first_not_of(" \t\n\r");
  size_t j = str.find_last_not_of(" \t\n\r");
  if (i == std::string::npos || j == std::string::npos || j < i)
  {
    str = "";
  }
  else
  {
    str = str.substr(i,j-i+1);
  }
}

void Expression::parse(const std::string &str)
{
  m_expr = str;
  trim(m_expr);

  if (m_expr.size() > 1 && m_expr[0] == '(' && m_expr[m_expr.size()-1] == ')')
  {
    if (m_expr.find('(',1) == std::string::npos)
    {
      m_expr.erase(0,1);
      m_expr.erase(m_expr.size()-1,1);
      trim(m_expr);
    }
  }

  tokenize();

  if (m_tokens.size() == 0)
  {
    setFunct(m_expr);
    return;
  }

  std::string op = GetOp(0);
  //size_t prec = m_operators->precedence[op];
  size_t prec = op_prec(op);
  tokenizer tkz(m_operators->binary[prec-1], " ", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);

  setFunct(*tkz.begin());

  for(size_t i=0;i<=m_tokens.size();i++)
  {
    m_terms.push_back(Expression(this));
    Expression& t = m_terms.back();
    if (i)
      t.m_op = GetOp(i-1);
    t.parse(GetToken(i));
  }
  m_expr = "";
  m_tokens.clear();
}

void Expression::tokenize()
{
  m_tokens.clear();

  size_t min_prec = 1000;
  size_t is = 0;
  size_t is1 = 0;
  unsigned int lvl = 0;
  size_t last = m_expr.size() - 1;
  bool inString = false;
  int skip = 0;
  bool canBeBinary = false;
  bool isNumber = false; // if parser is inside a number (important case is 123.45e+67)
  bool canDotBeAdded = false;
  bool canEBeAdded = false;
  bool canPlusBeAdded = false;
  Tokens tokens;
  for(size_t i=0;i<m_expr.size();i++)
  {
    char c = m_expr[i];


    if (!inString && skip == 0)
    {
      if (isNumber)
      {
        if (c == '.')
        {
          if (canDotBeAdded)
          {
            canDotBeAdded = false;
          }
          else
          {
            isNumber = false;
          }
        }
        else if (c == 'e' || c == 'E')
        {
          if (canEBeAdded)
          {
            canEBeAdded = false;
            canDotBeAdded = false;
            canPlusBeAdded = true;
          }
          else
          {
            isNumber = false;
          }
        }
        else if (c == '+' || c == '-')
        {
          if (canPlusBeAdded)
          {
            canPlusBeAdded = false;
            canEBeAdded = false;
            canDotBeAdded = false;
          }
          else
          {
            isNumber = false;
          }
        }
        else if (!isdigit(c))
        {
          isNumber = false;
        }
      }
      else if (isdigit(c))
      {
        isNumber = true;
        canDotBeAdded = true;
        canEBeAdded = true;
        canPlusBeAdded = false;
      }
      if (lvl == 0 && !isNumber && is_op_symbol(c))// insert new token
      {
        if (i == last) 
        {
          break;
          //throw std::runtime_error("Expression: syntax error");
        }

        if (is_op_symbol(m_expr[i+1]))
        {
          is1 = i + 2;
        }
        else
        {
          is1 = i + 1;
        }

        if (is1 > last) 
        {
          throw std::runtime_error("Expression: syntax error");
        }

        std::string op = m_expr.substr(i,is1-i);
        size_t prec = canBeBinary? m_operators->precedence[op] : 0;
        if (!prec) // operator does not exist
        {
          std::ostringstream mess;
          bool error = true;
          // check if it's a binary and a unary operators together
          if (op.size() == 2)
          {
            if (is_unary(op))
            {
              is1 -= 2;
              skip = 2;
              prec = min_prec + 1; // do not add token
              error = false;
            }
            else
            {
              is1 -= 1;
              std::string uop = op.substr(1,1);
              op = op[0];
              if (is_op_symbol(m_expr[is1+1]))
              {
                uop += m_expr[is1+1];
                if (is1 + 2 > last) 
                {
                  mess << "Expression: syntax error at " << is1+1;
                  throw std::runtime_error(mess.str());
                }
              }
              if (is_unary(uop))
              {
                prec = m_operators->precedence[op];
                if (prec)
                {// we don't want to create a new token with unary operator. it is processed in SetFunct()
                  skip = 1;
                  error = false;
                }
              }
            }
          }// op.size == 2
          else if (op.size() == 1)
          {
            //skip = 1;
            prec = min_prec + 1; // do not add token
            error = false;
          }
          if (error)
          {
            mess << "Expression: unrecognized operator " << op;
            throw std::runtime_error(mess.str());
          }
        }

        if (prec <= min_prec)
        {
          if (prec < min_prec)
	    min_prec = prec;
          Token tok(is,i-1,is1,prec);
          tokens.push_back(tok);
          is = is1;
        }

        i = is1 - 1;

        canBeBinary = false;

      }// insert new token
      else if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
      {
        canBeBinary = true;
      }

      if ( c == '(') lvl++;
      if ( c == ')')
      {
        if (lvl) lvl--;
        else
        {
          throw std::runtime_error("Unmatched brackets");
        }
      }
    } // !inString || skip
    else if (skip > 0)
    {
      skip --;
    }

    if (c == '"')
    {
      if ( !inString )
      {
        inString = true;
      }
      else
      {
        inString = false;
      }
    }


  } // for i

  if (tokens.size())
  {
    // remove operators of higher prec
    m_tokens.push_back(Token(tokens[0]));
    for(size_t i=0;i<tokens.size();i++)
    {
      Token& tok = tokens[i];
      std::string op = m_expr.substr(tok.ie+1,tok.is1-tok.ie-1);//?
      if (m_operators->precedence[op] == min_prec)
      {
        Token& last_tok = m_tokens.back();
        last_tok.ie = tok.ie;
        last_tok.is1= tok.is1;
        if (i != tokens.size()-1)
          m_tokens.push_back(Token(tokens[i+1]));
      }
    }
  }


}

std::string Expression::GetToken(size_t i)
{
  if (m_tokens.size() == 0) return m_expr;

  if (i < m_tokens.size())
  {
    Token& tok = m_tokens[i];
    return m_expr.substr(tok.is,tok.ie-tok.is+1);
  }

  if (i == m_tokens.size())
  {
    Token& tok = m_tokens[i-1];
    return m_expr.substr(tok.is1);
  }

  return "";

}

std::string Expression::GetOp(size_t i)
{
  if (m_tokens.size() == 0 || i >= m_tokens.size()) return "";

  Token& tok = m_tokens[i];
  return m_expr.substr(tok.ie+1,tok.is1-tok.ie-1);
}

void Expression::logPrint(const std::string& pads)const
{
  std::string myPads = pads + "   ";
  if (m_terms.size())
  {
    std::cerr<<myPads<<m_op<<'['<<m_funct<<']'<<"("<<'\n';
    for(size_t i=0;i<m_terms.size();i++)
      m_terms[i].logPrint(myPads);
    std::cerr<<myPads<<")"<<'\n';
  }
  else
    std::cerr<<myPads<<m_op<<m_funct<<'\n';
}

void Expression::setFunct(const std::string& name)
{
  if ( !op_prec(name))
  {
    std::string op = "";
    if (name.size() > 1 && is_op_symbol(name[0]))
    {
      op = name.substr(0,1);
      if (name.size() > 2 && is_op_symbol(name[1]))
      {
        op += name[1];
      }
    }
    if (! op.empty() && is_unary(op) )
    {
      m_funct = op;
      Expression tmp(this);
      tmp.parse(name.substr(op.size()));
      m_terms.push_back(tmp);
      return;
    }
  }

  m_funct = name;
  trim(m_funct);
  if (m_funct.empty())
  {
    throw std::runtime_error("Expression: Syntax error");
  }

  // Check if the function has arguments
  std::string::size_type i = std::string::npos;

  bool inQuotes = false;
  for(std::string::const_iterator c = name.begin(); c != name.end(); c++)
  {
    if (*c == '"')
    {
      if (inQuotes) inQuotes = false;
      else
        inQuotes = true;
      continue;
    }

    if ( !inQuotes && *c == '(')
    {
      i = c - name.begin();
      break;
    }
  }

  if (i != std::string::npos)
  {
    std::string::size_type j = name.find_last_of(')');
    if (j == std::string::npos || j < i)
    {
      throw std::runtime_error("Unmatched brackets");
    }

    if (j > i + 1)// nonzero argument list
    {
      std::string args = name.substr(i+1,j-i-1);//?
      trim(args);
      std::string f = name.substr(0,i);
      Expression tmp(this);
      tmp.parse(args);
      if ( !tmp.isFunct() || tmp.name() != ",")
      {
        m_terms.push_back(tmp);
      }
      else
      {
        std::string my_op = m_op;
        *this = tmp;
        m_op = my_op;
      }
      m_funct = f;
    }

  }
}

std::string Expression::str()const
{
  bool brackets = false;
  std::ostringstream res;
  size_t prec = op_prec(m_funct);
  if (size() == 1 && is_unary(m_funct))
  {// unary operator
    res << m_funct;
    if (op_prec(m_terms[0].m_funct) > 0)
    {
      brackets = true;
    }
  }
  else if (!prec)
  {// function with a name
    res << m_funct;
    brackets = true;
  }

  if (m_terms.size())
  {
    if (brackets) res << '(' ;
    for(size_t i=0;i<m_terms.size();i++)
    {
      res << m_terms[i].operator_name();
      size_t prec1 = op_prec(m_terms[i].m_funct);
      bool isItUnary = false;
      if (m_terms[i].size() == 1 && is_unary(m_terms[i].m_funct))
      {
        prec1 = 0; // unary operator
        isItUnary = true;
      }
      bool bk = prec > 0 && prec1 > 0  &&  static_cast<int>(prec) > prec1;
      if (bk) res << '(' ;
      if (isItUnary) res << ' ';
      res << m_terms[i].str();
      if (bk) res <<')';
    }
    if (brackets) res << ')' ;
  }
  return res.str();
}

const Expression& Expression::bracketsRemoved()const
{
  const Expression* e = this;
  while(e->name().empty() && e->size() == 1)
  {
    e = &e->m_terms[0];
  }
  return *e;
}

/**
 * Return a list of all variable names in this expression
 */
std::set<std::string> Expression::getVariables()const
{
  std::set<std::string> out;
  if ( !isFunct() )
  {
    std::string s = name();
    if ( !s.empty() && !isdigit(s[0]) )
    {
      out.insert(s);
    }
  }
  else
  {
    for(iterator e = begin(); e != end(); ++e)
    {
      if ( e->isFunct() )
      {
        std::set<std::string> tout = e->getVariables();
        out.insert(tout.begin(),tout.end());
      }
      else
      {
        std::string s = e->name();
        if ( !s.empty() && !isdigit(s[0]) )
        {
          out.insert(s);
        }
      }
    }
  }
  return out;
}

void Expression::rename(const std::string& newName)
{
  m_funct = newName;
}

void Expression::renameAll(const std::string& oldName,const std::string& newName)
{
  if ( !isFunct() && name() == oldName)
  {
    rename(newName);
  }
  else
  {
    std::vector<Expression>::iterator e = m_terms.begin();
    for(; e != m_terms.end(); ++e)
    {
      e->renameAll(oldName,newName);
    }
  }
}


}//API
}//Mantid
