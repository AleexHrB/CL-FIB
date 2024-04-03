
// Generated from Asl.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"




class  AslLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, ASSIGN = 7, 
    EQUAL = 8, NEQ = 9, GE = 10, LE = 11, GT = 12, LT = 13, PLUS = 14, MINUS = 15, 
    MUL = 16, MOD = 17, DIV = 18, LPAR = 19, RPAR = 20, VAR = 21, INT = 22, 
    FLOAT = 23, CHAR = 24, BOOL = 25, AND = 26, NOT = 27, OR = 28, RETURN = 29, 
    WHILE = 30, ENDWHILE = 31, IF = 32, THEN = 33, ELSE = 34, ENDIF = 35, 
    FUNC = 36, ENDFUNC = 37, READ = 38, WRITE = 39, ARRAY = 40, DO = 41, 
    INTVAL = 42, FLOATVAL = 43, BOOLVAL = 44, CHARVAL = 45, ID = 46, STRING = 47, 
    COMMENT = 48, WS = 49
  };

  explicit AslLexer(antlr4::CharStream *input);
  ~AslLexer();

  virtual std::string getGrammarFileName() const override;
  virtual const std::vector<std::string>& getRuleNames() const override;

  virtual const std::vector<std::string>& getChannelNames() const override;
  virtual const std::vector<std::string>& getModeNames() const override;
  virtual const std::vector<std::string>& getTokenNames() const override; // deprecated, use vocabulary instead
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;

  virtual const std::vector<uint16_t> getSerializedATN() const override;
  virtual const antlr4::atn::ATN& getATN() const override;

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;
  static std::vector<std::string> _channelNames;
  static std::vector<std::string> _modeNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

