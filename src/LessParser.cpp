/*
 * Copyright 2012 Bram van der Kroef
 *
 * This file is part of LESS CSS Compiler.
 *
 * LESS CSS Compiler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LESS CSS Compiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LESS CSS Compiler.  If not, see <http://www.gnu.org/licenses/>. 
 *
 * Author: Bram van der Kroef <bram@vanderkroef.net>
 */

#include "LessParser.h"

bool LessParser::parseStatement(Stylesheet* stylesheet) {
  return parseRuleset(stylesheet) || parseAtRuleOrVariable(stylesheet);
}

bool LessParser::parseAtRuleOrVariable (Stylesheet* stylesheet) {
  string keyword, import;
  TokenList* rule;
  AtRule* atrule = NULL;
  
  if (tokenizer->getTokenType() != Token::ATKEYWORD) 
    return false;

  keyword = tokenizer->getToken()->str;
  tokenizer->readNextToken();
  skipWhitespace();

  if (!parseVariable(keyword)) {
    rule = new TokenList();
    while(parseAny(rule)) {};
  
    if (!parseBlock(rule)) {
      if (tokenizer->getTokenType() != Token::DELIMITER) {
        throw new ParseException(tokenizer->getToken()->str,
                                 "delimiter (';') at end of @-rule");
      }
      tokenizer->readNextToken();
      skipWhitespace();
    }
    // parse import
    if (keyword == "@import") {
      if (rule->size() != 1 ||
          rule->front()->type != Token::STRING)
        throw new ParseException(*rule->toString(), "A string with the \
file path");
      import = rule->front()->str;
      if (import.size() < 5 ||
          import.substr(import.size() - 5, 4) != ".css") {
        if (import.size() < 6 || import.substr(import.size() - 6, 5) != ".less")
          import.insert(import.size() - 1, ".less");
        
        importFile(import.substr(1, import.size() - 2), stylesheet);
        return true;
      }
    } 
    atrule = new AtRule(new string(keyword));
    atrule->setRule(rule);
    stylesheet->addAtRule(atrule);
  }
  return true;
}

bool LessParser::parseVariable (string keyword = "") {
  TokenList* rule;
  
  if (keyword == "") {
    if (tokenizer->getTokenType() != Token::ATKEYWORD) 
      return false;
    keyword = tokenizer->getToken()->str;
    tokenizer->readNextToken();
    skipWhitespace();

    if (tokenizer->getTokenType() != Token::COLON)
      throw new ParseException(tokenizer->getToken()->str,
                               "colon (':') following @keyword in \
variable declaration.");
  } else if (tokenizer->getTokenType() != Token::COLON)
    return false;
  
  tokenizer->readNextToken();
  skipWhitespace();
    
  rule = parseValue();

  if (rule == NULL || rule->size() == 0) {
    throw new ParseException(tokenizer->getToken()->str,
                             "value for variable");
  }
  if (tokenizer->getTokenType() != Token::DELIMITER) {
    throw new ParseException(tokenizer->getToken()->str,
                             "delimiter (';') at end of @-rule");
  }
  valueProcessor->putVariable(keyword, rule);
  tokenizer->readNextToken();
  skipWhitespace();
  return true;
}

bool LessParser::parseRuleset (Stylesheet* stylesheet,
                               Selector* selector) {
  Ruleset* ruleset = NULL;
  ParameterRuleset* pruleset;
  list<string> parameters;
  list<string>::iterator pit;
  
  if (selector == NULL)
    selector = parseSelector();
  
  if (tokenizer->getTokenType() != Token::BRACKET_OPEN) {
    if (selector == NULL) 
      return false;
    else {
      throw new ParseException(tokenizer->getToken()->str,
                               "a declaration block ('{...}')");
    }
  }
  tokenizer->readNextToken();

  // add new scope for the ruleset
  valueProcessor->pushScope();
  
  if (selector->back()->type != Token::PAREN_CLOSED) {
    ruleset = new Ruleset(selector);
    stylesheet->addRuleset(ruleset);
  } else {
    ruleset = pruleset = new ParameterRuleset(selector);
    parameterRulesets.push_back(pruleset);

    // Add a NULL value to the local scope for each parameter so they
    // don't get replaced in the ruleset. For example this statement:
    // @x: 5; .class (@x: 0) {left: @x }
    // 'left: @x' would be replaced with 'left: 5' if we didn't do this
    parameters = pruleset->getKeywords();
    for(pit = parameters.begin(); pit != parameters.end(); pit++) {
      valueProcessor->putVariable(*pit, NULL);
    }
  }    

  skipWhitespace();
  parseRulesetStatement(stylesheet, ruleset);

  // remove scope
  valueProcessor->popScope();
  
  if (tokenizer->getTokenType() != Token::BRACKET_CLOSED) {
    throw new ParseException(tokenizer->getToken()->str,
                             "end of declaration block ('}')");
  } 
  tokenizer->readNextToken();
  skipWhitespace();
  
  return true;
}

bool LessParser::parseRulesetStatement (Stylesheet* stylesheet,
                                        Ruleset* ruleset) {
  Declaration* declaration;
  Selector* selector = parseSelector();
  TokenList* value;

  if (selector == NULL) {
    if (parseVariable()) {
      parseRulesetStatement(stylesheet, ruleset);
      return true;
    }
    return false;
  }

    // a selector followed by a ruleset is a nested rule
  if (parseNestedRule(selector, ruleset, stylesheet)) {
    parseRulesetStatement(stylesheet, ruleset);
    return true;
      
    // a selector by itself might be a mixin.
  } else if (parseMixin(selector, ruleset, stylesheet)) {
    delete selector;
    if (tokenizer->getTokenType() == Token::DELIMITER) {
      tokenizer->readNextToken();
      skipWhitespace();
      parseRulesetStatement(stylesheet, ruleset);
    }
    return true;
    
    // if we can parse a property and the next token is a COLON then the
    // statement is a declaration.
  } else if (selector->size() > 1 &&
        selector->front()->type == Token::IDENTIFIER &&
        selector->at(1)->type == Token::COLON) {
    
      declaration = new Declaration(new string(selector->front()->str));
      delete selector->shift();
      delete selector->shift();
      // parse any leftover value parts.
      value = CssParser::parseValue();
      if (value != NULL) {
        selector->push(value);
        delete value;
      }
      valueProcessor->processValue(selector);
      declaration->setValue(selector);
    
      ruleset->addDeclaration(declaration);
      if (tokenizer->getTokenType() == Token::DELIMITER) {
        tokenizer->readNextToken();
        skipWhitespace();
        parseRulesetStatement(stylesheet, ruleset);
      }
    
      return true;

  } else {
    throw new ParseException(*selector->toString(),
                             "a mixin that has been defined");
  }
}

bool LessParser::parseNestedRule(Selector* selector, Ruleset*
                                 ruleset, Stylesheet* stylesheet) {
  
  if (tokenizer->getTokenType() != Token::BRACKET_OPEN)
    return false;

  selector->addPrefix(ruleset->getSelector());
  parseRuleset(stylesheet, selector);
  return true;
}

Declaration* LessParser::parseDeclaration (string* property) {
  Declaration* declaration;
  
  skipWhitespace();
  if (tokenizer->getTokenType() != Token::COLON) 
    return NULL;
  tokenizer->readNextToken();
  skipWhitespace();
      
  declaration = new Declaration(property);
      
  TokenList* value = parseValue();
  if (value == NULL) {
    throw new ParseException(tokenizer->getToken()->str,
                             "value for property");
  }
  
  declaration->setValue(value);
  return declaration;
}

TokenList* LessParser::parseValue () {
  TokenList* value = CssParser::parseValue();
  if (value != NULL) 
    valueProcessor->processValue(value);

  return value;
}


bool LessParser::parseMixin(Selector* selector, Ruleset* ruleset,
                            Stylesheet* stylesheet) {
  Ruleset* mixin;
  vector<Declaration*>* declarations;
  vector<Declaration*>::iterator it;
  bool ret;

  ret = processParameterMixin(selector, ruleset);

  if ((mixin = stylesheet->getRuleset(selector)) != NULL) {
    declarations = mixin->getDeclarations();  
    for (it = declarations->begin(); it < declarations->end(); it++) {
      ruleset->addDeclaration((*it)->clone());
    }
    
    return true;
  } 
  return ret;
}

bool LessParser::processParameterMixin(Selector* selector, Ruleset* parent) {
  TokenList key;
  list<TokenList*>* arguments = new list<TokenList*>();

  TokenList* argument;  
  TokenListIterator* tli = selector->iterator();

  vector<ParameterRuleset*>::iterator pri;
  Token* current;

  ParameterRuleset* mixin;

  bool ret = false;

  while (tli->hasNext()) {
    current = tli->next();
    if (current->type == Token::PAREN_OPEN)
      break;
    key.push(current->clone());
  }

  // delete trailing whitespace
  while (key.back()->type == Token::WHITESPACE) {
    delete key.pop();
  }

  while (tli->hasNext()) {
    argument = new TokenList();

    while (tli->hasNext()) {
      current = tli->next();
      if (current->str == "," ||
          current->str == ";" ||
          current->type == Token::PAREN_CLOSED)
        break;
      argument->push(current->clone());
    }
    valueProcessor->processValue(argument);
    arguments->push_back(argument);
  }
  
  delete tli;

  for (pri = parameterRulesets.begin(); pri < parameterRulesets.end();
       pri++) {
    mixin = *pri;
    
    if (mixin->getSelector()->equals(&key) &&
        mixin->matchArguments(arguments)) {
      // new scope
      valueProcessor->pushScope();
      
      if (mixin->putArguments(valueProcessor, arguments) &&
          mixin->matchConditions(valueProcessor)) {
        mixin->addDeclarations(valueProcessor, parent);
        ret = true;
      }
      
      valueProcessor->popScope();
    }
  }
  return ret;
}


void LessParser::importFile(string filename, Stylesheet* stylesheet) {
  ifstream* in = new ifstream(filename.c_str());
  if (in->fail() || in->bad())
    throw new ParseException(filename, "existing file");
  
  LessTokenizer* tokenizer = new LessTokenizer(in);
  LessParser* parser = new LessParser(tokenizer);
  parser->parseStylesheet(stylesheet);
  in->close();
  delete parser;
  delete tokenizer;
  delete in;
}
