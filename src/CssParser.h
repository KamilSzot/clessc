#ifndef __CssParser_h__
#define __CssParser_h__

#include "CssTokenizer.h"
#include "Stylesheet.h"
#include "Token.h"
#include <string>
#include <vector>

using namespace std;

/**
 * Parses CSS code according to the syntax spec at
 * http://www.w3.org/TR/CSS2/syndata.html.
 * Excerpt:
 * 
 *   stylesheet  : [ CDO | CDC | S | statement ]*;
 *   statement   : ruleset | at-rule;
 *   at-rule     : ATKEYWORD S* any* [ block | ';' S* ];
 *   block       : '{' S* [ any | block | ATKEYWORD S* | ';' S* ]* '}' S*;
 *   ruleset     : selector? '{' S* declaration? [ ';' S* declaration? ]* '}' S*;
 *   selector    : any+;
 *   declaration : property S* ':' S* value;
 *   property    : IDENT;
 *   value       : [ any | block | ATKEYWORD S* ]+;
 *   any         : [ IDENT | NUMBER | PERCENTAGE | DIMENSION | STRING
 *                  | DELIM | URI | HASH | UNICODE-RANGE | INCLUDES
 *                  | DASHMATCH | ':' | FUNCTION S* [any|unused]* ')'
 *                  | '(' S* [any|unused]* ')' | '[' S* [any|unused]* ']'
 *                 ] S*;
 *   unused      : block | ATKEYWORD S* | ';' S* | CDO S* | CDC S*;
 *
 * The CDO ('<!--') and CDC ('-->') are not supported by the parser as
 * they are not expected in a .css file.
 */

class CssParser{
protected:
  CssTokenizer* tokenizer;

  /**
   * Parse whitespace tokens and comments and skip them.
   */
  void skipWhitespace();

  /**
   * Parse whitespace tokens and add them to the tokens
   * argument. Comments are skipped.
   */
  bool parseWhitespace(vector<Token*>* tokens);
  
  /**
   * Parses a Ruleset or an AtRule and adds it to the stylesheet.
   *
   * @return true if a Ruleset or AtRule could be parsed, false if
   *         neither were found.
   */
  bool parseStatement(Stylesheet* stylesheet);

  /**
   * Parses an keyword that begins with '@' and the rule following the keyword.
   *
   * @return an AtRule object or NULL if none was found.
   */
  AtRule* parseAtRule ();
  
  /**
   * Parse tokens inside brackets ('{', '}'). The tokens are added to
   * the vector.
   *
   * @return true if a block was parsed or false if the next token was
   *    not '{'.
   */
  bool parseBlock (vector<Token*>* tokens);

  /**
   * Parses an optional selector and a decleration block.
   *
   * @return a Ruleset object or NULL if none was found.
   */
  Ruleset* parseRuleset ();

  /**
   * Parses a selector into a vector of tokens.
   *
   * @return a vector containing the list of tokens or NULL if no
   *    selector was found.
   */
  vector<Token*>* parseSelector();

  /**
   * Parses a 'property: value' pair.
   *
   * @return a Declaration* object or NULL if none was found.
   */
  Declaration* parseDeclaration ();

  /**
   * Parses an identifier token that represents a property name.
   *
   * @return the string of the token or NULL if none was found.
   */
  string* parseProperty ();

  /**
   * Parses a list of tokens that represent a value.
   *
   * @return a vector containing a list of tokens or NULL if none
   *         could be found.
   */
  vector<Token*>* parseValue ();

  /**
   * Parses a token and adds it to the tokens argument. The token can
   * be:
   *  - Number, Percentage or Dimension
   *  - String
   *  - Url ('url(example.com)')
   *  - Hash (identifier that starts with a '#')
   *  - Unicode range (u+123456-789ABC)
   *  - Includes (~=)
   *  - Dashmatch (|=)
   *  - Colon (:)
   *  - Parenthesis (parses additional tokens between parenthesis.)
   *  - Identifier (optionally followed by parethesis)
   *  - Braces ('[]', parses additional tokens between braces.)
   *  - Any token that the Tokenizer defines as OTHER.
   *
   *  @return true if the token found was one of the tokens in the
   *          above list.
   */
  bool parseAny (vector<Token*>* tokens);


  /**
   * Parses a token and adds it to the tokens argument. The token can
   * be:
   *  - A block (see parseBlock()).
   *  - An AtKeyword: a keyword that begins with '@'.
   *  - A delimiter (';').
   *
   *  @return true if a block, atkeyword or delimiter could be parsed.
   */
  bool parseUnused(vector<Token*>* tokens);
  
public:

  /**
   * Initializes the CssParser with the given CssTokenizer argument.
   */
  CssParser(CssTokenizer* tokenizer);
  
  /**
   * Parses a stylesheet from the tokenizer. After parsing the
   * stylesheet all of the input should be parsed so this function
   * should only need to be called once.
   *
   * @throws ParseException if the parser comes across a mistake in
   *   the input, such as unterminated strings or parentheses.
   */
  Stylesheet* parseStylesheet();
  
  
};

#endif
