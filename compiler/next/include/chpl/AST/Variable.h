/*
 * Copyright 2021 Hewlett Packard Enterprise Development LP
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

#ifndef CHPL_AST_VARIABLE_H
#define CHPL_AST_VARIABLE_H

#include "chpl/AST/Symbol.h"
#include "chpl/AST/Location.h"

namespace chpl {
namespace ast {

class Builder;

/**
  This class represents a variable. For example:

  \rst
  .. code-block:: chapel

      var a = 1;
      ref b = a;
      const c = 2;
      const ref d = c;
      param e = "hi";
  \endrst

  each of these is a VariableDecl that refers to a Variable Symbol.
 */
class Variable final : public Symbol {
 friend class Builder;
 friend class VariableDecl;

 public:
  enum Tag {
    VAR,
    CONST,
    CONST_REF,
    REF,
    PARAM
  };

 private:
  Tag tag_;
  int8_t typeExprChildNum;
  int8_t initExprChildNum;

  Variable(ASTList children,
           UniqueString name, Symbol::Visibility vis, Tag tag,
           int8_t typeExprChildNum, int8_t initExprChildNum);
  bool contentsMatchInner(const BaseAST* other) const override;

 public:
  ~Variable() override = default;
  const Tag tag() const { return this->tag_; }
  /**
    Returns the type expression used in the variable's declaration, or nullptr
    if there wasn't one.
    */
  const Expr* typeExpr() const {
    if (typeExprChildNum >= 0) {
      const BaseAST* ast = this->child(typeExprChildNum);
      assert(ast->isExpr());
      return (const Expr*)ast;
    } else {
      return nullptr;
    }
  }
  /**
    Returns the init expression used in the variable's declaration, or nullptr
    if there wasn't one.
    */
  const Expr* initExpr() const {
    if (initExprChildNum >= 0) {
      const BaseAST* ast = this->child(initExprChildNum);
      assert(ast->isExpr());
      return (const Expr*)ast;
    } else {
      return nullptr;
    }
  }
};

} // end namespace ast
} // end namespace chpl

#endif
