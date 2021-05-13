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

#ifndef CHPL_UAST_SYM_H
#define CHPL_UAST_SYM_H

#include "chpl/uast/ASTNode.h"
#include "chpl/queries/UniqueString.h"

namespace chpl {
namespace uast {


/**
  This is an abstract base class for Symbols
 */
class Sym : public ASTNode {

 public:
  enum Visibility {
    DEFAULT_VISIBILITY,
    PUBLIC,
    PRIVATE,
  };

 private:
  UniqueString name_;
  Visibility visibility_;

 protected:
 Sym(ASTTag tag,
     UniqueString name, Sym::Visibility visibility)
   : ASTNode(tag), name_(name), visibility_(visibility) {
  }

 Sym(ASTTag tag, ASTList children,
     UniqueString name, Sym::Visibility visibility)
   : ASTNode(tag, std::move(children)),
    name_(name), visibility_(visibility) {
  }

  bool symContentsMatchInner(const Sym* other) const {
    return this->name_ == other->name_ &&
           this->visibility_ == other->visibility_;
  }
  void symMarkUniqueStringsInner(Context* context) const {
    name_.mark(context);
  }

 public:
  virtual ~Sym() = 0; // this is an abstract base class

  UniqueString name() const { return name_; }
  Visibility visibility() const { return visibility_; }
};


} // end namespace uast
} // end namespace chpl

#endif
