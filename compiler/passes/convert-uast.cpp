/*
 * Copyright 2020-2021 Hewlett Packard Enterprise Development LP
 * Copyright 2004-2019 Cray Inc.
 * Other additional copyright holders may be indicated within.
 *
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* This file supports conversion of uAST to the older AST used by
   the rest of the compiler. It uses recursive functions to perform
   the conversion.
 */

#include "convert-uast.h"

#include "DoWhileStmt.h"
#include "WhileDoStmt.h"
#include "build.h"
#include "docsDriver.h"

#include "chpl/uast/all-uast.h"
#include "chpl/util/string-escapes.h"

using namespace chpl;

namespace {

struct Converter {
  chpl::Context* context = nullptr;
  bool inTupleDecl = false;
  UniqueString thisStr;

  Converter(chpl::Context* context) : context(context) {
    thisStr = UniqueString::build(context, "this");
  }

  Expr* convertAST(const uast::ASTNode* node);

  Expr* convertExprOrNull(const uast::ASTNode* node) {
    if (node == nullptr)
      return nullptr;

    Expr* ret = convertAST(node);
    INT_ASSERT(ret);
    return ret;
  }

  Expr* visit(const uast::Comment* node) {
    // old ast does not represent comments
    return nullptr;
  }

  Expr* visit(const uast::ErroneousExpression* node) {
    return new CallExpr(PRIM_ERROR);
  }

  UnresolvedSymExpr* visit(const uast::Identifier* node) {
    return new UnresolvedSymExpr(node->name().c_str());
  }

  /// SimpleBlockLikes ///

  BlockStmt* createBlockWithStmts(
      uast::ASTListIteratorPair<uast::Expression> stmts) {
    BlockStmt* block = new BlockStmt();
    for (auto stmt: stmts) {
      Expr* e = convertAST(stmt);
      if (e) {
        block->insertAtTail(e);
      }
    }
    return block;
  }

  BlockStmt* visit(const uast::Begin* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  BlockStmt* visit(const uast::Block* node) {
    return createBlockWithStmts(node->stmts());
  }

  BlockStmt* visit(const uast::Defer* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  BlockStmt* visit(const uast::Local* node) {
    BlockStmt* body = createBlockWithStmts(node->stmts());
    Expr* condition = convertExprOrNull(node->condition());
    if (condition) {
      return buildLocalStmt(condition, body);
    } else {
      return buildLocalStmt(body);
    }
  }

  BlockStmt* visit(const uast::On* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  BlockStmt* visit(const uast::Serial* node) {
    BlockStmt* body = createBlockWithStmts(node->stmts());
    Expr* condition = convertExprOrNull(node->condition());

    if (condition) {
      return buildSerialStmt(condition, body);
    } else {
      return buildSerialStmt(new SymExpr(gTrue), body);
    }
  }

  /// Expressions ///

  CallExpr* visit(const uast::Delete* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  CallExpr* visit(const uast::New* node) {
    Expr* newedType = convertAST(node->typeExpression());
    CallExpr* typeCall = new CallExpr(newedType);

    switch (node->management()) {
      case uast::New::DEFAULT_MANAGEMENT:
        return new CallExpr(PRIM_NEW, typeCall);
      case uast::New::OWNED:
        return new CallExpr(PRIM_NEW, typeCall,
                            new NamedExpr(astr_chpl_manager,
                                          new SymExpr(dtOwned->symbol)));
      case uast::New::SHARED:
        return new CallExpr(PRIM_NEW, typeCall,
                            new NamedExpr(astr_chpl_manager,
                                          new SymExpr(dtShared->symbol)));
      case uast::New::UNMANAGED:
        return new CallExpr(PRIM_NEW, typeCall,
                            new NamedExpr(astr_chpl_manager,
                                          new SymExpr(dtUnmanaged->symbol)));
      case uast::New::BORROWED:
        return new CallExpr(PRIM_NEW, typeCall,
                            new NamedExpr(astr_chpl_manager,
                                          new SymExpr(dtBorrowed->symbol)));
    }
    INT_FATAL("case not handled");
    return nullptr;
  }

  Expr* visit(const uast::As* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  UseStmt* visit(const uast::Use* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  Expr* visit(const uast::UseClause* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  Expr* visit(const uast::WithClause* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  GotoStmt* visit(const uast::Break* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  BlockStmt* visit(const uast::Cobegin* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  Expr* visit(const uast::Conditional* node) {
    Expr* condExpr = toExpr(convertAST(node->condition()));
    BlockStmt* thenBlock = createBlockWithStmts(node->thenStmts());
    BlockStmt* elseBlock = nullptr;

    if (node->hasElseBlock())
      elseBlock = createBlockWithStmts(node->elseStmts());

    return buildIfStmt(condExpr, thenBlock, elseBlock);
  }

  GotoStmt* visit(const uast::Continue* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  DefExpr* visit(const uast::Label* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  CallExpr* visit(const uast::Return* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  BlockStmt* visit(const uast::Sync* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  CallExpr* visit(const uast::Yield* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  /// Loops ///

  BlockStmt* visit(const uast::DoWhile* node) {
    Expr* condExpr = toExpr(convertAST(node->condition()));
    BlockStmt* body = createBlockWithStmts(node->stmts());
    return DoWhileStmt::build(condExpr, body);
  }

  BlockStmt* visit(const uast::While* node) {
    Expr* condExpr = toExpr(convertAST(node->condition()));
    BlockStmt* body = createBlockWithStmts(node->stmts());
    return WhileDoStmt::build(condExpr, body);
  }

  /// IndexableLoops ///

  BlockStmt* visit(const uast::BracketLoop* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  BlockStmt* visit(const uast::Coforall* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  BlockStmt* visit(const uast::For* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  BlockStmt* visit(const uast::Forall* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  BlockStmt* visit(const uast::Foreach* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  /// Literals ///

  /// NumericLiterals ///

  Expr* visit(const uast::ImagLiteral* node) {
    SymExpr* se = buildImagLiteral(node->text().c_str());
    VarSymbol* v = toVarSymbol(se->symbol());
    INT_ASSERT(v && v->immediate && v->immediate->const_kind == NUM_KIND_IMAG);
    INT_ASSERT(v->immediate->real_value() == node->value());
    return se;
  }

  Expr* visit(const uast::IntLiteral* node) {
    SymExpr* se = buildIntLiteral(node->text().c_str());
    VarSymbol* v = toVarSymbol(se->symbol());
    INT_ASSERT(v && v->immediate && v->immediate->const_kind == NUM_KIND_INT);
    INT_ASSERT(v->immediate->int_value() == node->value());
    return se;
  }

  Expr* visit(const uast::RealLiteral* node) {
    SymExpr* se = buildRealLiteral(node->text().c_str());
    VarSymbol* v = toVarSymbol(se->symbol());
    INT_ASSERT(v && v->immediate && v->immediate->const_kind == NUM_KIND_REAL);
    INT_ASSERT(v->immediate->real_value() == node->value());
    return se;
  }

  Expr* visit(const uast::UintLiteral* node) {
    SymExpr* se = buildIntLiteral(node->text().c_str());
    VarSymbol* v = toVarSymbol(se->symbol());
    INT_ASSERT(v && v->immediate && v->immediate->const_kind == NUM_KIND_UINT);
    INT_ASSERT(v->immediate->uint_value() == node->value());
    return se;
  }

  /// StringLikeLiterals ///
  Expr* visit(const uast::BytesLiteral* node) {
    std::string quoted = quoteStringForC(node->str());
    SymExpr* se = buildBytesLiteral(quoted.c_str());
    VarSymbol* v = toVarSymbol(se->symbol());
    INT_ASSERT(v && v->immediate);
    INT_ASSERT(v->immediate->const_kind == CONST_KIND_STRING);
    INT_ASSERT(v->immediate->string_kind == STRING_KIND_BYTES);
    return se;
  }

  Expr* visit(const uast::CStringLiteral* node) {
    std::string quoted = quoteStringForC(node->str());
    SymExpr* se = buildCStringLiteral(quoted.c_str());
    VarSymbol* v = toVarSymbol(se->symbol());
    INT_ASSERT(v && v->immediate);
    INT_ASSERT(v->immediate->const_kind == CONST_KIND_STRING);
    INT_ASSERT(v->immediate->string_kind == STRING_KIND_C_STRING);
    return se;

  }

  Expr* visit(const uast::StringLiteral* node) {
    std::string quoted = quoteStringForC(node->str());
    SymExpr* se = buildStringLiteral(quoted.c_str());
    VarSymbol* v = toVarSymbol(se->symbol());
    INT_ASSERT(v && v->immediate);
    INT_ASSERT(v->immediate->const_kind == CONST_KIND_STRING);
    INT_ASSERT(v->immediate->string_kind == STRING_KIND_STRING);
    return se;
  }

  /// Calls ///

  Expr* visit(const uast::Dot* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  Expr* visit(const uast::FnCall* node) {
    const uast::Expression* calledExpression = node->calledExpression();
    INT_ASSERT(calledExpression);
    Expr* calledExpr = convertAST(calledExpression);
    INT_ASSERT(calledExpr);

    CallExpr* ret = nullptr;
    CallExpr* addArgsTo = nullptr;
    if (calledExpression->isNew()) {
      // we have (call PRIM_NEW (call C) mgmt)
      // and need to add the arguments to the (call C)
      CallExpr* primNew = toCallExpr(calledExpr);
      INT_ASSERT(primNew->isPrimitive(PRIM_NEW));
      CallExpr* typeCall = toCallExpr(primNew->get(1));
      INT_ASSERT(typeCall);
      ret = primNew;
      addArgsTo = typeCall;
    } else {
      ret = new CallExpr(calledExpr);
      addArgsTo = ret;
    }

    int nActuals = node->numActuals();
    for (int i = 0; i < nActuals; i++) {
      Expr* actual = convertAST(node->actual(i));
      INT_ASSERT(actual);
      if (node->isNamedActual(i)) {
        actual = buildNamedActual(node->actualName(i).c_str(), actual);
      }
      addArgsTo->insertAtTail(actual);
    }

    return ret;
  }

  Expr* visit(const uast::OpCall* node) {
    CallExpr* ret = new CallExpr(node->op().c_str());
    int nActuals = node->numActuals();
    for (int i = 0; i < nActuals; i++) {
      Expr* actual = convertAST(node->actual(i));
      INT_ASSERT(actual);
      ret->insertAtTail(actual);
    }

    return ret;
  }

  /*Expr* visit(const uast::PrimCall* node) {
    INT_FATAL("TODO");
    return nullptr;
  }*/

  Expr* visit(const uast::Zip* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  /// Decls ///

  Expr* visit(const uast::MultiDecl* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  Expr* visit(const uast::TupleDecl* node) {
    BlockStmt* block = new BlockStmt(BLOCK_SCOPELESS);

    bool saveInTupleDecl = inTupleDecl;
    inTupleDecl = true;

    for (auto decl : node->decls()) {
      Expr* d = convertAST(decl);
      INT_ASSERT(d);
      if (DefExpr* def = toDefExpr(d)) {
        if (VarSymbol* var = toVarSymbol(def->sym)) {
          if (var->name[0] == '_' && var->name[1] == '\0') {
            var->name = astr("chpl_tuple_blank");
            var->cname = astr("chpl_tuple_blank");
          }
        }
      }
      block->insertAtTail(d);
    }

    Expr* typeExpr = convertExprOrNull(node->typeExpression());
    Expr* initExpr = convertExprOrNull(node->initExpression());

    BlockStmt* ret = buildTupleVarDeclStmt(block, typeExpr, initExpr);

    inTupleDecl = saveInTupleDecl;
    return ret;
  }

  /// NamedDecls ///

  Expr* visit(const uast::EnumElement* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  static RetTag convertRetTag(uast::Function::ReturnIntent returnIntent) {
    switch (returnIntent) {
      case uast::Function::DEFAULT_RETURN_INTENT:
        return RET_VALUE;
      case uast::Function::CONST:
        return RET_VALUE;
      case uast::Function::CONST_REF:
        return RET_CONST_REF;
      case uast::Function::REF:
        return RET_REF;
      case uast::Function::PARAM:
        return RET_PARAM;
      case uast::Function::TYPE:
        return RET_TYPE;
    }

    INT_FATAL("case not handled");
    return RET_VALUE;
  }

  static bool isAssignOp(UniqueString name) {
    return (name == "=" ||
            name == "+=" ||
            name == "-=" ||
            name == "*=" ||
            name == "/=" ||
            name == "%=" ||
            name == "**=" ||
            name == "&=" ||
            name == "|=" ||
            name == "^=" ||
            name == ">>=" ||
            name == "<<=");
  }

  Expr* visit(const uast::Function* node) {
    FnSymbol* fn = new FnSymbol("_");
    if (node->isInline()) {
      fn->addFlag(FLAG_INLINE);
    }
    if (node->isOverride()) {
      fn->addFlag(FLAG_OVERRIDE);
    }
    // TODO: add FLAG_NO_PARENS for parenless functions

    IntentTag thisTag = INTENT_BLANK;
    Expr* receiverType = nullptr;

    // Add the formals
    if (node->numFormals() > 0) {
      for (auto formal : node->formals()) {
        DefExpr* def = visit(formal);
        INT_ASSERT(def);
        if (formal->name() != thisStr) {
          buildFunctionFormal(fn, def); // adds it to the list
        } else if (!node->isPrimaryMethod()) {
          thisTag = convertFormalIntent(formal->intent());
          receiverType = convertExprOrNull(formal->typeExpression());
        }
      }
    }

    UniqueString name = node->name();

    fn = buildFunctionSymbol(fn, name.c_str(), thisTag, receiverType);

    if (isAssignOp(name)) {
      fn->addFlag(FLAG_ASSIGNOP);
    }

    RetTag retTag = convertRetTag(node->returnIntent());


    // TODO: handle specified cname for extern/export functions

    if (node->kind() == uast::Function::ITER) {
      if (fn->hasFlag(FLAG_EXTERN))
        USR_FATAL_CONT(fn, "'iter' is not legal with 'extern'");
      fn->addFlag(FLAG_ITERATOR_FN);
    }

    if (node->kind() == uast::Function::OPERATOR) {
      fn->addFlag(FLAG_OPERATOR);
      if (fn->_this != NULL) {
        updateOpThisTagOrErr(fn);
        setupTypeIntentArg(toArgSymbol(fn->_this));
      }
    }

    Expr* retType = convertExprOrNull(node->returnType());
    Expr* whereClause = convertExprOrNull(node->whereClause());

    Expr* lifetimeConstraints = nullptr;
    if (node->numLifetimeClauses() > 0) {
      for (auto clause : node->lifetimeClauses()) {
        Expr* convertedClause = convertAST(clause);
        INT_ASSERT(convertedClause);

        if (lifetimeConstraints == nullptr) {
          lifetimeConstraints = convertedClause;
        } else {
          lifetimeConstraints =
            new CallExpr(",", lifetimeConstraints, convertedClause);
        }
      }
      INT_ASSERT(lifetimeConstraints);
    }

    BlockStmt* body = createBlockWithStmts(node->stmts());
    BlockStmt* decl = buildFunctionDecl(fn, retTag, retType,
                                        node->throws(), whereClause,
                                        lifetimeConstraints,
                                        body,
                                        /* docs */ nullptr);

    return decl;
  }

  DefExpr* visit(const uast::Module* node) {
    chpl::UniqueString ustr = node->name();
    const char* name = ustr.c_str();
    const char* path = context->filePathForID(node->id()).c_str();
    ModTag tag = MOD_USER; // TODO: distinguish internal/standard/etc
    bool priv = (node->visibility() == uast::Decl::PRIVATE);
    bool prototype = (node->kind() == uast::Module::PROTOTYPE ||
                      node->kind() == uast::Module::IMPLICIT);
    BlockStmt* body = createBlockWithStmts(node->stmts());

    ModuleSymbol* mod = buildModule(name,
                                    tag,
                                    body,
                                    path,
                                    priv,
                                    prototype,
                                    /* docs */ nullptr);

    return new DefExpr(mod);
  }

  /// VarLikeDecls ///

  static IntentTag convertFormalIntent(uast::Formal::Intent intent) {
    switch (intent) {
      case uast::Formal::DEFAULT_INTENT:
        return INTENT_BLANK;
      case uast::Formal::CONST:
        return INTENT_CONST;
      case uast::Formal::CONST_REF:
        return INTENT_CONST_REF;
      case uast::Formal::REF:
        return INTENT_REF;
      case uast::Formal::IN:
        return INTENT_IN;
      case uast::Formal::CONST_IN:
        return INTENT_CONST_IN;
      case uast::Formal::OUT:
        return INTENT_OUT;
      case uast::Formal::INOUT:
        return INTENT_INOUT;
      case uast::Formal::PARAM:
        return INTENT_PARAM;
      case uast::Formal::TYPE:
        return INTENT_TYPE;
    }

    INT_FATAL("case not handled");
    return INTENT_BLANK;
  }

  DefExpr* visit(const uast::Formal* node) {
    IntentTag intentTag = convertFormalIntent(node->intent());

    Expr* typeExpr = convertExprOrNull(node->typeExpression());
    Expr* initExpr = convertExprOrNull(node->initExpression());

    Expr* varargsVariable = nullptr; // TODO: handle varargs

    return buildArgDefExpr(intentTag, node->name().c_str(),
                           typeExpr, initExpr, varargsVariable);
  }

  Expr* visit(const uast::TaskVar* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  const char* tupleVariableName(const char* name) {
    if (inTupleDecl && name[0] == '_' && name[1] == '\0')
      return "chpl__tuple_blank";
    else
      return name;
  }

  Expr* visit(const uast::Variable* node) {
    auto stmts = new BlockStmt(BLOCK_SCOPELESS);

    auto varSym = new VarSymbol(tupleVariableName(node->name().c_str()));
    // Adjust the variable according to its kind
    switch (node->kind()) {
      case uast::Variable::VAR:
        varSym->qual = QUAL_VAL;
        break;
      case uast::Variable::CONST:
        varSym->addFlag(FLAG_CONST);
        varSym->qual = QUAL_CONST;
        break;
      case uast::Variable::CONST_REF:
        varSym->addFlag(FLAG_CONST);
        varSym->addFlag(FLAG_REF_VAR);
        varSym->qual = QUAL_CONST_REF;
        break;
      case uast::Variable::REF:
        varSym->addFlag(FLAG_REF_VAR);
        varSym->qual = QUAL_REF;
        break;
      case uast::Variable::PARAM:
        varSym->addFlag(FLAG_PARAM);
        varSym->qual = QUAL_PARAM;
        break;
      case uast::Variable::TYPE:
        varSym->addFlag(FLAG_TYPE_VARIABLE);
        break;
      case uast::Variable::INDEX:
        break;
    }

    Expr* typeExpr = convertExprOrNull(node->typeExpression());
    Expr* initExpr = convertExprOrNull(node->initExpression());

    auto defExpr = new DefExpr(varSym, initExpr, typeExpr);
    stmts->insertAtTail(defExpr);

    // Add a PRIM_END_OF_STATEMENT.
    if (fDocs == false && inTupleDecl == false) {
      CallExpr* end = new CallExpr(PRIM_END_OF_STATEMENT);
      stmts->insertAtTail(end);
    }

    return stmts;
  }

  /// TypeDecls

  Expr* visit(const uast::Enum* node) {
    INT_FATAL("TODO");
    return nullptr;
  }

  /// AggregateDecls

  Expr* visit(const uast::Class* node) {
    const char* name = node->name().c_str();
    const char* cname = name;
    Expr* inherit = convertExprOrNull(node->parentClass());
    BlockStmt* decls = createBlockWithStmts(node->declOrComments());
    Flag externFlag = FLAG_UNKNOWN;

    return buildClassDefExpr(name, cname, AGGREGATE_CLASS, inherit, decls,
                             externFlag, /* docs */ nullptr);
  }

  Expr* visit(const uast::Record* node) {
    const char* name = node->name().c_str();
    const char* cname = name; // TODO: cname could be set for extern record
    Expr* inherit = nullptr;
    BlockStmt* decls = createBlockWithStmts(node->declOrComments());
    Flag externFlag = FLAG_UNKNOWN; // TODO: add extern record support

    return buildClassDefExpr(name, cname, AGGREGATE_RECORD, inherit, decls,
                             externFlag, /* docs */ nullptr);
  }

  Expr* visit(const uast::Union* node) {
    const char* name = node->name().c_str();
    const char* cname = name; // TODO: cname could be set for extern union
    Expr* inherit = nullptr;
    BlockStmt* decls = createBlockWithStmts(node->declOrComments());
    Flag externFlag = FLAG_UNKNOWN; // TODO: add extern union support

    return buildClassDefExpr(name, cname, AGGREGATE_UNION, inherit, decls,
                             externFlag, /* docs */ nullptr);
  }
};

/// Generic conversion calling the above functions ///
Expr* Converter::convertAST(const uast::ASTNode* node) {
  astlocMarker markAstLoc(node->id());
  return node->dispatch<Expr*>(*this);
}

} // end anonymous namespace

ModuleSymbol* convertToplevelModule(chpl::Context* context,
                                    const chpl::uast::Module* mod) {
  printf("Converting module named %s\n", mod->name().c_str());
  astlocMarker markAstLoc(mod->id());
  Converter c(context);
  DefExpr* def = c.visit(mod);
  ModuleSymbol* ret = toModuleSymbol(def->sym);
  ModuleSymbol::addTopLevelModule(ret);
  return ret;
}
