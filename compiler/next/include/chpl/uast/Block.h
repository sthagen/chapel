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

#ifndef CHPL_UAST_BLOCK_H
#define CHPL_UAST_BLOCK_H

#include "chpl/queries/Location.h"
#include "chpl/uast/Expression.h"

namespace chpl {
namespace uast {


/**
  This class represents a { } block.
 */
class Block final : public Expression {
 private:
  Block(ASTList stmts)
    : Expression(asttags::Block, std::move(stmts)) {

    assert(isExpressionASTList(children_));
  }

  bool contentsMatchInner(const ASTNode* other) const override;
  void markUniqueStringsInner(Context* context) const override;

 public:
  /**
   Create and return a Block containing the passed stmts.
   */
  static owned<Block> build(Builder* builder, Location loc, ASTList stmts);

  /**
    Return a way to iterate over the statements.
   */
  ASTListIteratorPair<Expression> stmts() const {
    return ASTListIteratorPair<Expression>(children_.begin(), children_.end());
  }

  /**
   Return the number of statements in the block.
   */
  int numStmts() const {
    return this->numChildren();
  }
  /**
   Return the i'th statement in the block.
   */
  const Expression* stmt(int i) const {
    const ASTNode* ast = this->child(i);
    assert(ast->isExpression());
    return (const Expression*)ast;
  }
};


} // end namespace uast
} // end namespace chpl

#endif
