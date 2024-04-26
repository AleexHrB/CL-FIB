//////////////////////////////////////////////////////////////////////
//
//    CodeGenVisitor - Walk the parser tree to do
//                     the generation of code
//
//    Copyright (C) 2020-2030  Universitat Politecnica de Catalunya
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU General Public License
//    as published by the Free Software Foundation; either version 3
//    of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//    contact: José Miguel Rivero (rivero@cs.upc.edu)
//             Computer Science Department
//             Universitat Politecnica de Catalunya
//             despatx Omega.110 - Campus Nord UPC
//             08034 Barcelona.  SPAIN
//
//////////////////////////////////////////////////////////////////////

#include "CodeGenVisitor.h"
#include "antlr4-runtime.h"

#include "../common/TypesMgr.h"
#include "../common/SymTable.h"
#include "../common/TreeDecoration.h"
#include "../common/code.h"

#include <string>
#include <cstddef>    // std::size_t

// uncomment the following line to enable debugging messages with DEBUG*
// #define DEBUG_BUILD
#include "../common/debug.h"

// using namespace std;


// Constructor
CodeGenVisitor::CodeGenVisitor(TypesMgr       & Types,
                               SymTable       & Symbols,
                               TreeDecoration & Decorations) :
  Types{Types},
  Symbols{Symbols},
  Decorations{Decorations} {
}

// Accessor/Mutator to the attribute currFunctionType
TypesMgr::TypeId CodeGenVisitor::getCurrentFunctionTy() const {
  return currFunctionType;
}

void CodeGenVisitor::setCurrentFunctionTy(TypesMgr::TypeId type) {
  currFunctionType = type;
}

// Methods to visit each kind of node:
//
antlrcpp::Any CodeGenVisitor::visitProgram(AslParser::ProgramContext *ctx) {
  DEBUG_ENTER();
  code my_code;
  SymTable::ScopeId sc = getScopeDecor(ctx);
  Symbols.pushThisScope(sc);
  for (auto ctxFunc : ctx->function()) { 
    subroutine subr = visit(ctxFunc);
    my_code.add_subroutine(subr);
  }
  Symbols.popScope();
  DEBUG_EXIT();
  return my_code;
}

antlrcpp::Any CodeGenVisitor::visitFunction(AslParser::FunctionContext *ctx) {
  DEBUG_ENTER();
  TypesMgr::TypeId t1;
  if (ctx -> type()) t1 = getTypeDecor(ctx -> type());
  else t1 = Types.createVoidTy();

  SymTable::ScopeId sc = getScopeDecor(ctx);
  Symbols.pushThisScope(sc);
  subroutine subr(ctx->ID()->getText());
  codeCounters.reset();
  std::vector<var> && lvars = visit(ctx->declarations());
  for (auto & onevar : lvars) {
    subr.add_var(onevar);
  }



  if (not Types.isVoidTy(t1)) subr.add_param("_result", Types.to_string(t1), Types.isArrayTy(t1)); 

  if (ctx -> parameters()) {
      std::vector<std::pair<var, TypesMgr::TypeId>> && params = visit(ctx->parameters());
      for (auto & onevar : params) {
          subr.add_param(onevar.first.name, onevar.first.type, Types.isArrayTy(onevar.second));
      }
  }

  instructionList && code = visit(ctx->statements());
  code = code || instruction(instruction::RETURN());
  subr.set_instructions(code);
  Symbols.popScope();
  DEBUG_EXIT();
  return subr;
}

antlrcpp::Any CodeGenVisitor::visitDeclarations(AslParser::DeclarationsContext *ctx) {
  DEBUG_ENTER();
  std::vector<var> lvars;
  for (auto & varDeclCtx : ctx->variable_decl()) {
      std::vector<var> && aux = visit(varDeclCtx);
      lvars.insert(lvars.end(), aux.begin(), aux.end());
  }
  DEBUG_EXIT();
  return lvars;
}

antlrcpp::Any CodeGenVisitor::visitParameters(AslParser::ParametersContext *ctx) {
  DEBUG_ENTER();
  std::vector<std::pair<var, TypesMgr::TypeId>> params;
  for (size_t i = 0; i < ctx->type().size(); ++i) {
      TypesMgr::TypeId t = getTypeDecor(ctx->type(i));
      std::size_t size = Types.getSizeOfType(t);
      std::string s = Types.to_string(t);

      if (Types.isArrayTy(t)) s = Types.to_string(Types.getArrayElemType(t));
      params.push_back({var{ctx->ID(i)->getText(), s , size}, t});
  }
  DEBUG_EXIT();
  return params;
}

antlrcpp::Any CodeGenVisitor::visitVariable_decl(AslParser::Variable_declContext *ctx) {
  DEBUG_ENTER();
  TypesMgr::TypeId   t1 = getTypeDecor(ctx->type());
  std::size_t      size = Types.getSizeOfType(t1);
  std::vector<var> lvars;

  /**
   ----------------------------------
   * MAL:
    vars
      x integer
      y boolean
      z array<10,integer> 10
    endvars

    * BIEN:
    vars
      x integer
      y boolean
      z integer 10
    endvars
  */
  if (Types.isArrayTy(t1))
    t1 = Types.getArrayElemType(t1);
  // ----------------------------------

  for (auto& a : ctx -> ID()) lvars.push_back(var{a ->getText(), Types.to_string(t1), size}); 
  DEBUG_EXIT();
  return lvars;
}

antlrcpp::Any CodeGenVisitor::visitStatements(AslParser::StatementsContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  for (auto stCtx : ctx->statement()) {
    instructionList && codeS = visit(stCtx);
    code = code || codeS;
  }
  DEBUG_EXIT();
  return code;
}


antlrcpp::Any CodeGenVisitor::visitAssignStmt(AslParser::AssignStmtContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAtsE1 = visit(ctx->left_expr());
  std::string           addr1 = codAtsE1.addr;
  std::string           offs1 = codAtsE1.offs;
  instructionList &     code1 = codAtsE1.code;
  TypesMgr::TypeId tid1 = getTypeDecor(ctx->left_expr());

  CodeAttribs     && codAtsE2 = visit(ctx->expr());
  std::string           addr2 = codAtsE2.addr;
  // std::string           offs2 = codAtsE2.offs;
  instructionList &     code2 = codAtsE2.code;
  TypesMgr::TypeId tid2 = getTypeDecor(ctx->expr());

  instructionList code = code1 || code2;

  // type coertion int->float
  if (Types.isFloatTy(tid1) and Types.isIntegerTy(tid2)) {
      std::string temp = "%"+codeCounters.newTEMP();
      code = code || instruction::FLOAT(temp, addr2);
      addr2 = temp;
  }
  
  // load
  if (offs1 == "") code = code || instruction::LOAD(addr1, addr2);
  else code = code || instruction::XLOAD(addr1, offs1, addr2);
  
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitIfStmt(AslParser::IfStmtContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  CodeAttribs     && codAtsE = visit(ctx->expr());
  std::string          addr1 = codAtsE.addr;
  instructionList &    code1 = codAtsE.code;
  instructionList &&   code2 = visit(ctx->statements(0));
  std::string label = codeCounters.newLabelIF();
  std::string labelEndIf = "endif"+label;

  if (not ctx -> ELSE()) {
      code = code1 || instruction::FJUMP(addr1, labelEndIf) ||
          code2 || instruction::LABEL(labelEndIf);
  }

  else {
      std::string label2 = codeCounters.newLabelIF();
      std::string labelEndElse = "endelse"+label;
      instructionList &&   code3 = visit(ctx->statements(1));
      code = code1 || instruction::FJUMP(addr1, labelEndIf) ||
          code2 || instruction::UJUMP(labelEndElse) ||instruction::LABEL(labelEndIf) || code3
          || instruction::LABEL(labelEndElse);
  }
  DEBUG_EXIT();
  return code;
}


antlrcpp::Any CodeGenVisitor::visitReadStmt(AslParser::ReadStmtContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAtsE = visit(ctx->left_expr());

  std::string          addr1 = codAtsE.addr;
  std::string          offs1 = codAtsE.offs;

  instructionList &    code1 = codAtsE.code;
  instructionList &     code = code1;
  TypesMgr::TypeId tid1 = getTypeDecor(ctx->left_expr());
  
  std::string temp = "%"+codeCounters.newTEMP();

  if (Types.isIntegerTy(tid1) or Types.isBooleanTy(tid1)) code = code1 || instruction::READI(temp);
  else if (Types.isFloatTy(tid1)) code = code1 || instruction::READF(temp);
  else if (Types.isCharacterTy(tid1)) code = code1 || instruction::READC(temp);
  else std::cout << "Read failed" << std::endl;

  //Visentada
  if (offs1 != "") {
      code = code || instruction::XLOAD(addr1, offs1, temp);
  }

  else code = code || instruction::LOAD(addr1, temp);

  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitWriteExpr(AslParser::WriteExprContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt1 = visit(ctx->expr());
  std::string         addr1 = codAt1.addr;
  // std::string         offs1 = codAt1.offs;
  instructionList &   code1 = codAt1.code;
  instructionList &    code = code1;
  TypesMgr::TypeId tid1 = getTypeDecor(ctx->expr());

  if (Types.isIntegerTy(tid1)) code = code1 || instruction::WRITEI(addr1);
  else if (Types.isCharacterTy(tid1)) code = code1 || instruction::WRITEC(addr1);
  else if (Types.isBooleanTy(tid1)) code = code1 || instruction::WRITEI(addr1);
  else if (Types.isFloatTy(tid1)) code = code1 || instruction::WRITEF(addr1);

  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitWriteString(AslParser::WriteStringContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  std::string s = ctx->STRING()->getText();
  code = code || instruction::WRITES(s);
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitLeftExprIdent(AslParser::LeftExprIdentContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs && codAts = visit(ctx->ident());
  DEBUG_EXIT();
  return codAts;
}

//a[2] + 3
antlrcpp::Any CodeGenVisitor::visitArithmetic(AslParser::ArithmeticContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt1 = visit(ctx->expr(0));
  std::string         addr1 = codAt1.addr;
  instructionList &   code1 = codAt1.code;
  CodeAttribs     && codAt2 = visit(ctx->expr(1));
  std::string         addr2 = codAt2.addr;
  instructionList &   code2 = codAt2.code;
  instructionList &&   code = code1 || code2;
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr(0));
  TypesMgr::TypeId t2 = getTypeDecor(ctx->expr(1));
  TypesMgr::TypeId  t = getTypeDecor(ctx);
  std::string temp = "%"+codeCounters.newTEMP();
  //else if (ctx -> MOD()) code = code || instruction::MUL(temp, addr1, addr2);

  if (Types.isFloatTy(t)) {
      if (Types.isIntegerTy(t1)) {
          std::string temp2 = "%"+codeCounters.newTEMP();
          code = code || instruction::FLOAT(temp2, addr1);
          addr1 = temp2;
      }
      else if (Types.isIntegerTy(t2)) {
          std::string temp2 = "%"+codeCounters.newTEMP();
          code = code || instruction::FLOAT(temp2, addr2);
          addr2 = temp2;
      }

      if (ctx->MUL())
          code = code || instruction::FMUL(temp, addr1, addr2);
      else if (ctx -> DIV())
          code = code || instruction::FDIV(temp, addr1, addr2);
      else if (ctx -> PLUS())
          code = code || instruction::FADD(temp, addr1, addr2);
      else if (ctx -> MINUS())
          code = code || instruction::FSUB(temp, addr1, addr2);
  }

  else {
      if (ctx->MUL())
          code = code || instruction::MUL(temp, addr1, addr2);
      else if (ctx -> DIV())
          code = code || instruction::DIV(temp, addr1, addr2);
      else if (ctx -> PLUS())
          code = code || instruction::ADD(temp, addr1, addr2);
      else if (ctx -> MINUS())
          code = code || instruction::SUB(temp, addr1, addr2);
  }

  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitRelational(AslParser::RelationalContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt1 = visit(ctx->expr(0));
  std::string         addr1 = codAt1.addr;
  instructionList &   code1 = codAt1.code;
  CodeAttribs     && codAt2 = visit(ctx->expr(1));
  std::string         addr2 = codAt2.addr;
  instructionList &   code2 = codAt2.code;
  instructionList &&   code = code1 || code2;
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr(0));
  TypesMgr::TypeId t2 = getTypeDecor(ctx->expr(1));
  //TypesMgr::TypeId  t = getTypeDecor(ctx);
  std::string temp = "%"+codeCounters.newTEMP();

  if (Types.isFloatTy(t1) or Types.isFloatTy(t2)) {

      if (Types.isIntegerTy(t1)) {
          std::string temp2 = "%"+codeCounters.newTEMP();
          code = code || instruction::FLOAT(temp2, addr1);
          addr1 = temp2;
      }
      else if (Types.isIntegerTy(t2)) {
          std::string temp2 = "%"+codeCounters.newTEMP();
          code = code || instruction::FLOAT(temp2, addr2);
          addr2 = temp2;
      }

      if (ctx -> EQUAL())
          code = code || instruction::FEQ(temp, addr1, addr2);
      else if (ctx -> NEQ()) {
          code = code || instruction::FEQ(temp, addr1, addr2);
          code = code || instruction::NOT(temp, temp);
      }
      else if (ctx -> GT()) {
          code = code || instruction::FLE(temp, addr1, addr2);
          code = code || instruction::NOT(temp, temp);
      }
      else if (ctx -> LT())
          code = code || instruction::FLT(temp, addr1, addr2);
      else if (ctx -> GE()) {
          code = code || instruction::FLT(temp, addr1, addr2);
          code = code || instruction::NOT(temp, temp);
      }
      else if (ctx -> LE())
          code = code || instruction::FLE(temp, addr1, addr2);
  }

  else {
      if (ctx -> EQUAL())
          code = code || instruction::EQ(temp, addr1, addr2);
      else if (ctx -> NEQ()) {
          code = code || instruction::EQ(temp, addr1, addr2);
          code = code || instruction::NOT(temp, temp);
      }
      else if (ctx -> GT()) {
          code = code || instruction::LE(temp, addr1, addr2);
          code = code || instruction::NOT(temp, temp);
      }
      else if (ctx -> LT())
          code = code || instruction::LT(temp, addr1, addr2);
      else if (ctx -> GE()) {
          code = code || instruction::LT(temp, addr1, addr2);
          code = code || instruction::NOT(temp, temp);
      }
      else if (ctx -> LE())
          code = code || instruction::LE(temp, addr1, addr2);
  }

  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitValue(AslParser::ValueContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  std::string temp = "%"+codeCounters.newTEMP();
  TypesMgr::TypeId t1 = getTypeDecor(ctx);
  if (Types.isBooleanTy(t1)) code = instruction::ILOAD(temp, ctx->getText() == "true" ? "1" : "0");
  else if (Types.isFloatTy(t1)) code = instruction::FLOAD(temp, ctx->getText());
  else if (Types.isCharacterTy(t1)) code = instruction::CHLOAD(temp, ctx->getText());
  else if (Types.isIntegerTy(t1)) code = instruction::ILOAD(temp, ctx->getText());
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitExprIdent(AslParser::ExprIdentContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs && codAts = visit(ctx->ident());
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitIdent(AslParser::IdentContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs codAts(ctx->ID()->getText(), "", instructionList());
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitParenthesis(AslParser::ParenthesisContext *ctx) {
    DEBUG_ENTER();
    DEBUG_EXIT();
    return visit(ctx -> expr());
}

antlrcpp::Any CodeGenVisitor::visitUnaryLogical(AslParser::UnaryLogicalContext *ctx) {
    DEBUG_ENTER();
    CodeAttribs     && codAt1 = visit(ctx->expr());
    std::string         addr1 = codAt1.addr;
    instructionList & code = codAt1.code;
    std::string temp = "%"+codeCounters.newTEMP();

    if (ctx -> NOT()) code  = codAt1.code || instruction::NOT(temp, addr1);

    CodeAttribs codAts(temp, "", code);
    
    DEBUG_EXIT();

    return codAts;
}

antlrcpp::Any CodeGenVisitor::visitLogical(AslParser::LogicalContext *ctx) {
    DEBUG_ENTER();
    CodeAttribs     && codAt1 = visit(ctx->expr(0));
    std::string         addr1 = codAt1.addr;
    instructionList &   code1 = codAt1.code;
    CodeAttribs     && codAt2 = visit(ctx->expr(1));
    std::string         addr2 = codAt2.addr;
    instructionList &   code2 = codAt2.code;
    instructionList &&   code = code1 || code2;

    std::string temp = "%"+codeCounters.newTEMP();

    if (ctx -> AND()) code = code || instruction::AND(temp, addr1, addr2);
    else if (ctx -> OR()) code = code || instruction::OR(temp, addr1, addr2);

    CodeAttribs codAts(temp, "", code);
    DEBUG_EXIT();
    return codAts;
}

antlrcpp::Any CodeGenVisitor::visitUnaryArithmetic(AslParser::UnaryArithmeticContext *ctx) {
    DEBUG_ENTER();
    CodeAttribs     && codAt1 = visit(ctx->expr());
    std::string         addr1 = codAt1.addr;
    instructionList &   code1 = codAt1.code;
    TypesMgr::TypeId  t = getTypeDecor(ctx);

    if (ctx -> MINUS()) {
        std::string temp = "%"+codeCounters.newTEMP();
        if (Types.isFloatTy(t)) code1 = code1 || instruction::FNEG(temp, addr1);
        else code1 = code1 || instruction::NEG(temp, addr1);
        addr1 = temp;
    }

    CodeAttribs codAts(addr1, "", code1);
    DEBUG_EXIT();
    return codAts;
}


antlrcpp::Any CodeGenVisitor::visitWhileStmt(AslParser::WhileStmtContext *ctx) {
    DEBUG_ENTER();
    std::string count = codeCounters.newLabelWHILE();
    std::string initWhile = "while" + count;
    std::string endWhile = "endWhile" + count;
    instructionList && code1 = instruction::LABEL(initWhile);

    CodeAttribs     && codAt1 = visit(ctx->expr());
    std::string         addr1 = codAt1.addr;

    code1 = code1 || codAt1.code;
    code1 = code1 || instruction::FJUMP(codAt1.addr, endWhile);

    instructionList && code2 = visit(ctx->statements());

    code1 = code1 || code2;
    code1 = code1 || instruction::UJUMP(initWhile);
    code1 = code1 || instruction::LABEL(endWhile);

    DEBUG_EXIT();
    return code1;
}


// Getters for the necessary tree node atributes:
//   Scope and Type
SymTable::ScopeId CodeGenVisitor::getScopeDecor(antlr4::ParserRuleContext *ctx) const {
  return Decorations.getScope(ctx);
}
TypesMgr::TypeId CodeGenVisitor::getTypeDecor(antlr4::ParserRuleContext *ctx) const {
  return Decorations.getType(ctx);
}


// Constructors of the class CodeAttribs:
//
CodeGenVisitor::CodeAttribs::CodeAttribs(const std::string & addr,
                                         const std::string & offs,
                                         instructionList & code) :
  addr{addr}, offs{offs}, code{code} {
}

CodeGenVisitor::CodeAttribs::CodeAttribs(const std::string & addr,
                                         const std::string & offs,
                                         instructionList && code) :
  addr{addr}, offs{offs}, code{code} {
}


antlrcpp::Any CodeGenVisitor::visitReturnStmt(AslParser::ReturnStmtContext *ctx) {
    DEBUG_ENTER();
    instructionList code1;

    if (ctx -> expr()) {
        CodeAttribs     && codAt1 = visit(ctx->expr());
        std::string         addr1 = codAt1.addr;
        code1 = codAt1.code || instruction::LOAD("_result", addr1);
    }

    code1 = code1 || instruction::RETURN();
    DEBUG_EXIT();
    return code1;
}

antlrcpp::Any CodeGenVisitor::visitFuncExpr(AslParser::FuncExprContext *ctx) {
    DEBUG_ENTER();
    instructionList && code = instruction::PUSH(); // push _result

    TypesMgr::TypeId tFunc = getTypeDecor(ctx -> ident());
    const std::vector<TypesMgr::TypeId>& functionParams = Types.getFuncParamsTypes(tFunc);

    for (unsigned int i = 0; i < ctx->expr().size(); ++i) {
        CodeAttribs     && codAt1 = visit(ctx->expr(i));
        std::string         addr1 = codAt1.addr;
        code = code || codAt1.code;

        // type coertion: int -> float
        TypesMgr::TypeId tExpr = getTypeDecor(ctx->expr(i));
        if (Types.isIntegerTy(tExpr) and Types.isFloatTy(functionParams[i])) {
          std::string temp = "%"+codeCounters.newTEMP();
          code = code || instruction::FLOAT(temp, addr1);
          addr1 = temp;
        }

        code = code || instruction::PUSH(addr1);
    }

    code = code || instruction::CALL(ctx -> ident() -> getText());

    for (unsigned int i = 0; i < ctx -> expr().size(); ++i) code = code || instruction::POP();

    std::string temp = "%"+codeCounters.newTEMP();

    code = code || instruction::POP(temp);

    CodeAttribs codAts(temp, "", code);
    
    DEBUG_EXIT();

    return codAts;
}


antlrcpp::Any CodeGenVisitor::visitProcCall(AslParser::ProcCallContext *ctx) {
    DEBUG_ENTER();
    instructionList code;

    TypesMgr::TypeId tFunc = getTypeDecor(ctx -> ident());
    const std::vector<TypesMgr::TypeId>& functionParams = Types.getFuncParamsTypes(tFunc);

    if (not Types.isVoidFunction(tFunc)) code = code || instruction::PUSH();

    for (unsigned int i = 0; i < ctx->expr().size(); ++i) {
        CodeAttribs     && codAt1 = visit(ctx->expr(i));
        std::string         addr1 = codAt1.addr;
        code = code || codAt1.code;

        // type coertion: int -> float
        TypesMgr::TypeId tExpr = getTypeDecor(ctx->expr(i));

        if (Types.isIntegerTy(tExpr) and Types.isFloatTy(functionParams[i])) {
          std::string temp = "%"+codeCounters.newTEMP();
          code = code || instruction::FLOAT(temp, addr1);
          addr1 = temp;
        }

        code = code || instruction::PUSH(addr1);
    }

    code = code || instruction::CALL(ctx -> ident() -> getText());

    for (unsigned int i = 0; i < ctx -> expr().size(); ++i) {
        TypesMgr::TypeId t = getTypeDecor(ctx -> expr(i));

        if (Types.isArrayTy(t)) code = code || instruction::POP();
        else code = code || instruction::POP();
    }

    if (not Types.isVoidFunction(tFunc)) code = code || instruction::POP();

    //std::string temp = "%"+codeCounters.newTEMP();
    //code = code || instruction::POP(temp);
    
    DEBUG_EXIT();

    return code;
}

antlrcpp::Any CodeGenVisitor::visitArrayAccessExpr(AslParser::ArrayAccessExprContext *ctx) {
    DEBUG_ENTER();
    CodeAttribs     && codAt1 = visit(ctx -> expr(0));
    std::string         addr1 = codAt1.addr;

    CodeAttribs     && codAt2 = visit(ctx -> expr(1));
    std::string         addr2 = codAt2.addr;

    instructionList && code = codAt1.code || codAt2.code;
    
    std::string temp = "%"+codeCounters.newTEMP();

    //TypesMgr::TypeId t = getTypeDecor(ctx->expr(0));
    //t = Types.getArrayElemType(t);
    //std::size_t size = Types.getSizeOfType(t);

    //code = code || instruction::MUL(temp, std::to_string(size), addr2);
    code = code || instruction::LOADX(temp, addr1, addr2);

    CodeAttribs codAts(temp, "", code);
    DEBUG_EXIT();
    return codAts;
}


antlrcpp::Any CodeGenVisitor::visitArrayAccessLExpr(AslParser::ArrayAccessLExprContext *ctx) {
    DEBUG_ENTER();
    CodeAttribs     && codAt1 = visit(ctx -> expr(0));
    std::string         addr1 = codAt1.addr;

    CodeAttribs     && codAt2 = visit(ctx -> expr(1));
    std::string         addr2 = codAt2.addr;

    instructionList && code = codAt1.code || codAt2.code;
    
    //std::string temp = "%"+codeCounters.newTEMP();

    //TypesMgr::TypeId t = getTypeDecor(ctx->expr(0));
    //t = Types.getArrayElemType(t);
    //std::size_t size = Types.getSizeOfType(t);
    //code = code || instruction::MUL(temp, std::to_string(size), addr2);

    CodeAttribs codAts(addr1, addr2, code);

    DEBUG_EXIT();
    return codAts;
}
