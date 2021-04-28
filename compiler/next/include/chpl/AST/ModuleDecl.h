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

#ifndef CHPL_AST_MODULEDECL_H
#define CHPL_AST_MODULEDECL_H

#include "chpl/AST/Decl.h"
#include "chpl/AST/Location.h"
#include "chpl/AST/Module.h"

namespace chpl {
namespace ast {

class Builder;

/**
  This class represents a module declaration.

  \rst
  .. code-block:: chapel

      module M { }
  \endrst

  contains a module declaration referring to the symbol for module M.
 */
class ModuleDecl final : public Decl {
 friend class Builder;

 private:
  ModuleDecl(owned<Module> module);
  bool contentsMatchInner(const BaseAST* other) const override;

 public:
  ~ModuleDecl() override = default;
  static owned<ModuleDecl> build(Builder* builder, Location loc,
                                 UniqueString name, Symbol::Visibility vis,
                                 Module::Tag tag, ASTList stmts);
  const Module* module() const {
    assert(this->symbol()->isModule());
    return (Module*)this->symbol();
  }
  const Module::Tag tag() const { return this->module()->tag(); }
};

} // end namespace ast
} // end namespace chpl

#endif
