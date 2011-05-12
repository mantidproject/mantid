#ifndef MANTID_API_EXPRESSION_H
#define MANTID_API_EXPRESSION_H

#include "MantidKernel/System.h"

#include <boost/shared_ptr.hpp>
#include <string.h>
#include <vector>
#include <map>
#include <set>

namespace Mantid
{
  namespace API
  {
    /** 
    
    This class represents an expression made up of names, binary operators and brackets.
    The input for an Expression is a text string. If an Expression is a function (sum, product,
    sine, etc) it has arguments. Each argument is an Expression itself. So Expression is a tree structure
    with functions in its nodes and the branches are the arguments.

    @author Roman Tolchenov, Tessella plc
    @date 2/02/2010

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport Expression
    {
    public:
      /// Default contructor
      Expression();
      /// contructor
      Expression(const std::vector<std::string>& ops);
      /// contructor
      Expression(const std::vector<std::string>& binary,const std::set<std::string>& unary);
      /// copy contructor
      Expression(const Expression& expr);
      /// Assignment operator
      Expression& operator=(const Expression& expr);
      /**
       * Parse a string and create an expression.
       * @param str :: The input string.
       */
      void parse(const std::string& str);
      /**
       * Print the expression into std::cerr to show its structure
       * @param pads :: Padding to make indentation
       */
      void logPrint(const std::string& pads = "")const;
      /// Returns this expression as a string. It does not simply returns the input string but recreates it.
      std::string str()const;
      /// Returns true if the expression is a function (i.e. has arguments)
      bool isFunct()const{return m_terms.size() > 0;}
      /// Returns the name of the expression which is a function or variable name.
      std::string name()const{return m_funct;}
      /// Returns the the expression's binary operator on its left. Can be an empty string.
      std::string operator_name()const{return m_op;}
      /// Returns the top level terms of the expression (function arguments). For a variable it empty.
      const std::vector<Expression>& terms()const{return m_terms;}
      /// Returns the number of argumens
      int size()const{return static_cast<int>(m_terms.size());}
      /// Const Iterator tpyedef
      typedef std::vector<Expression>::const_iterator iterator;

      /// An iterator pointing to the start of the expressions
      iterator begin()const{return m_terms.begin();}
      /// An iterator pointing to the end of the expressions
      iterator end()const{return m_terms.end();}
      /**
       * Gets the Expression at the specified index
       * @param i :: the index
       * @return Expression at the given index
       */
      const Expression& operator[](int i)const{return m_terms.at(i);}
      /// If the expression has 1 argument and empty function name it means it is wrapped in brackets
      /// This method returns first sub-expression without brackets
      const Expression& bracketsRemoved()const;
      /// Return a list of all variable names in this expression
      std::set<std::string> getVariables()const;
      /**
       * Rename all variables with a given name 
       * @param oldName :: The old name
       * @param newName :: The new name
       */
      void renameAll(const std::string& oldName,const std::string& newName);
      /**
       * Rename this expression 
       * @param newName :: The new name
       */
      void rename(const std::string& newName);
    private:

      /// copy contructor
      Expression(const Expression* pexpr);
      /** 
       * This is a struct to mark a token in a string expression.
       * Tokens in an expressions are separated by operators.
       * A token is either a symbolic name (not containing operators and empty spaces)
       * or another expression.
       */
      struct Token
      {
        /**
         * Constructor.
         * @param i :: The index of the first symbol of the token.
         * @param j :: The index of the last symbol of the token.
         * @param k :: The index of the first symbol of the next token. The substring
         *    between j and k contains the operator connecting the next token to this one.
         * @param p :: The precedence of the connecting operator.
         */
        Token(size_t i, size_t j, size_t k,size_t p)
          :is(i),ie(j),is1(k),prec(p){}
        /**
         * The copy constructor.
         */
        Token(const Token& t)
          :is(t.is),ie(t.ie),is1(t.is1),prec(t.prec){}
        size_t is;  ///< The index of the first symbol of the token.
        size_t ie;  ///< The index of the last symbol of the token.
        size_t is1; ///< The index of the first symbol of the next token.
        size_t prec;      ///< The precedence of the connecting operator.
      };
      /// The container type
      typedef std::vector< Token > Tokens;
      /// Get i-th token
      std::string GetToken(size_t i);
      /// Get the operator connecting i-th token
      std::string GetOp(size_t i);
      /// Analyze the string in m_expr and find all top level tokens.
      /// Result: filled in m_tokens structure.
      void tokenize();
      /**
       * Set the function name of this expression. It is also called when
       * name cannot be split into tokens. 
       * @param name :: The name of the function
       */
      void setFunct(const std::string& name);
      /** 
       * Remove leading and ending empty spaces from a string
       * @param str :: The string.
       */
      static void trim(std::string& str);

      Tokens m_tokens;     ///< The container for the token markers
      std::string m_expr;  ///< Saved expression string

      std::string m_funct; ///< Function name
      std::string m_op;    ///< Operator connecting this expression to its sibling on the left
      std::vector<Expression> m_terms;///< Child expressions (function arguments)

      /**
       * Keeps operator that can be used in an expression
       */
      struct Operators
      {
        std::vector<std::string> binary;          ///< Binary operators in reverse precedence order
        std::set<std::string> unary;           ///< Unary operators
        std::map<std::string, size_t> precedence; ///< Map of the operator precedence order
        std::set<char> symbols;                   ///< All the symbols that are used in the binary operators
        std::map<std::string, char> op_number;    ///< map of operators
      };

      /**
       * Returns the precedence of operator op
       * @param op :: The operator
       * @return The precedence
       */
      size_t op_prec(const std::string& op)const;
      /**
       * Adds new binary operators to the expression
       * @param ops :: A vector with operators in reverse order of precedence
       */
      void add_operators(const std::vector<std::string>& ops);
      /**
       * Adds new unary operators to the expression
       * @param ops :: A vector with unary operators
       */
      void add_unary(const std::set<std::string>& ops);
      /**
       * Check if a string is a unary operator
       * @param op :: The string to check
       * @return True if the argument is a unary operator
       */
      bool is_unary(const std::string& op)const;
      /**
       * Check if a character is a part of an operator
       * @param c :: The character to check
       * @return True if it is true
       */
      bool is_op_symbol(const char c)const;

      boost::shared_ptr<Operators> m_operators; ///< pointer ot the operators

    };

  }// API
}// Mantid

#endif /* EXPRESSION_H */
