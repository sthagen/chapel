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

#ifndef CHPL_QUERIES_CONTEXT_H
#define CHPL_QUERIES_CONTEXT_H

#include "chpl/AST/UniqueString.h"
#include "chpl/AST/ID.h"
#include "chpl/Queries/ContextDetail.h"
#include "chpl/Util/memory.h"

#include <unordered_map>

namespace chpl {

/**

\rst

This class stores the compilation-wide context. Another name for this
compilation-wide context is *program database*. It handles unique'd strings and
also stores the results of queries (so that they are are memoized). It tracks
dependencies of queries in order to update them appropriately when a dependency
changes.

Queries are functions that are written in a stylized manner to interact with the
context (aka program database). For example, a ``parse`` query might accept as
an argument a ``UniqueString path`` and return a vector of owned AST nodes.
Another example is a query to determine the location of an AST node; it would
accept as an argument a ``BaseAST*`` and it would return a Location.

When running a query, the query system will manage:
 * checking to see if the query result is already saved and available for reuse
 * recording the queries called by that query as dependencies

To write a query, create a function that uses the ``QUERY_`` macros defined in
QueryImpl.h. The arguments to the function need to be efficient to copy (so
``UniqueString``, ``ID``, ``Location``, and pointers are OK, but e.g.
``std::vector`` is not).  The function will return a result, which need not be
POD and can include AST pointers (but see below). The function needs to be
written in a stylized way to interact with the context.

For example, here is a query that computes MyResultType from myArg1 and
myArg2:

.. code-block:: c++

    #include "chpl/Queries/QueryImpl.h"

    const MyResultType& myQueryFunction(Context* context,
                                        MyArgType MyArg1,
                                        MyOtherArgType MyArg2) {
      QUERY_BEGIN(context, MyResultType, myKey1, myKey2)
      if (QUERY_USE_SAVED()) {
        return QUERY_GET_SAVED();
      }
      // do steps to compute the result
      MyResultType result = ...;
      // if an error is encountered, it can be saved with QUERY_ERROR(error)

      return QUERY_END(result);
    }


To call the query, just write e.g. ``myQueryFunction(context, key1, key2)``.

The query function will check for a result stored already in the program
database that can be reused and the first return accounts for that case.  After
that, the query proceeds to compute the result. When doing so, any queries
called will be automatically recorded as dependencies. It will then compare the
computed result with the saved result, if any, and in some cases combine the
results. Finally, the saved result (which might have been updated) is returned.

Note that a query can return a value or a pointer in addition to a const
reference.

There are some requirements on query argument/key types and on result types.

Since the argument/key types are stored in a hashtable, we need
``std::hash<KeyType>`` and ``std::equal_to<KeyType>`` to be implemented, e.g.

.. code-block:: c++

    namespace std {
      template<> struct hash<chpl::MyPodKeyType> {
        size_t operator()(const chpl::ast::UniqueString key) const {
          return doSomethingToComputeHash...;
        }
      };
      template<> struct equal_to<chpl::MyPodKeyType> {
        bool operator()(const chpl::MyPodKeyType lhs,
                        const chpl::MyPodKeyType rhs) const {
          return doSomethingToCheckIfEqual...;
        }
      };
    }

The process of computing a query and checking to see if it matches a saved
result requires that the result type implement ``chpl::update``:

.. code-block:: c++

    namespace chpl {
      template<> struct update<MyResultType> {
        bool operator()(chpl::ast::UniqueString& keep,
                        chpl::ast::UniqueString& addin) const {
          return doSomethingToCombine...;
        }
      };

On entry to the ``update`` function, ``keep`` is the current value in the
program database and ``addin`` is the newly computed value. The ``update``
function needs to:

  * store the current, updated result in ``keep``
  * store the unused result in ``addin``
  * return ``false`` if ``keep`` matched ``addin`` -- that is, ``keep`` did not
    need to be updated; and ``true`` otherwise.

For most result types, ``return defaultCombine(keep, addin);`` should be
sufficient. In the event that a result is actually a collection of results
that *owns* the elements (for example, when parsing, the result is
conceptually a vector of top-level symbol), the ``combine`` function
should try to update only those elements of ``keep`` that changed by swapping
in the appropriate elements from ``addin``. This strategy allows later queries
that depend on such a result to use pointers to the owned elements and to
avoid updating everything if just one element changed.

Queries *can* return results that contain non-owning pointers to results from
dependent queries. In that event, the update function should not rely on the
contents of these pointers. The system will make sure that they refer to valid
memory but they might be a combination of old results. Additionally, the system
will ensure that any old results being replaced will remain allocated until the
garbage collection runs outside of any query.

For example, a ``parse`` query might result in a list of ``owned`` AST element
pointers. A follow-on ``listSymbols`` result in something containing these AST
element pointers, but not owning them. The ``listSymbols`` query needs to use a
``update`` function that does not look into these queries.

\endrst

 */
class Context {
 private:
  // map that supports uniqueCString / UniqueString
  using UniqueStringsTableType = std::unordered_map<const char*, char*, chpl::detail::UniqueStrHash, chpl::detail::UniqueStrEqual>;
  UniqueStringsTableType uniqueStringsTable;

  // map from a query name to appropriate QueryMap object.
  // maps to an 'owned' heap-allocated thing to manage having subclasses
  // without slicing.
  std::unordered_map<UniqueString, owned<chpl::querydetail::QueryMapBase>> queryDB;

  // Since IDs include module names but not file paths, use this
  // map to go from module name to file path.
  // (If this proves too restrictive for some reason, we could
  //  start including file path in IDs).
  std::unordered_map<UniqueString, UniqueString> modNameToFilepath;

  struct QueryDepsEntry {
    UniqueString queryName;
    chpl::querydetail::QueryDependencyVec dependencies;
    std::vector<ErrorMessage> errors;
    QueryDepsEntry(UniqueString queryName)
      : queryName(queryName), dependencies(), errors() {
    }
  };

  // this is used to compute the dependencies
  std::vector<QueryDepsEntry> queryDeps;

  chpl::querydetail::RevisionNumber currentRevisionNumber;
  chpl::querydetail::RevisionNumber lastPrepareToGCRevisionNumber;
  chpl::querydetail::RevisionNumber gcCounter;

  Context();
  const char* getOrCreateUniqueString(const char* s);
  bool queryCanUseSavedResult(chpl::querydetail::QueryMapResultBase* resultEntry);
  void saveDependenciesAndErrorsInParent(chpl::querydetail::QueryMapResultBase* resultEntry);
  void endQueryHandleDependency(chpl::querydetail::QueryMapResultBase* result);

  template<typename ResultType, typename... ArgTs>
  chpl::querydetail::QueryMapResult<ResultType>*
  updateResultForQuery(
      const std::tuple<ArgTs...>& tupleOfArgs,
      ResultType result,
      bool& changedOut,
      chpl::querydetail::QueryMap<ResultType,ArgTs...>* queryMap);

  template<typename ResultType, typename... ArgTs>
  chpl::querydetail::QueryMapResult<ResultType>*
  updateResultForQuery(const std::tuple<ArgTs...>& tupleOfArgs,
                       ResultType result,
                       bool& changedOut,
                       UniqueString queryName);

  // Future Work: support marking used strings and garbage collecting the rest
  // Could store an atomic uint_8 just after the string for the mark.

  // Future Work: make the context thread-safe

  // Future Work: allow moving some AST to a different context
  //              (or, at least, that can handle the unique strings)

  // Performance: Add fine-grained timing to measure
  //  * the total time spent in each query
  //  * the time spent in each query in Context functions
  //    (i.e. hashtable manipulations)
  //  * the time spent in each query in other queries
  //  * the time spent in each query in other query code

  // Performance: How can we arrange for better locality of reference/
  // cache reuse for the maps from IDs? The IDs within a function could
  // be just stored in a vector, but that would add an indirection to
  // the hashtable. Is there a way to adjust the hashing function and
  // tune the hashtable bucket size, or something? Do we need a custom
  // hashtable?

 public:
  /**
    Create a new AST Context.
   */
  static owned<Context> build();
  ~Context();

  /**
    Get or create a unique string for a NULL-terminated C string
    and return it as a C string. If the passed string is NULL,
    this function will return an empty string.

    Strings returned by this function will always be aligned to 2 bytes.

    The function `UniqueString::build` returns such a string
    with a wrapper type. It should be preferred for type safety
    and to reduce redundant checks.
   */
  const char* uniqueCString(const char* s);

  /**
    Return the name of the module containing this ID.
   */
  UniqueString moduleNameForID(ID id);
  /**
    Return the file path for the file containing this ID.
   */
  UniqueString filePathForID(ID id);

  /**
    Query to get a file path given a module name
   */
  UniqueString filePathForModuleName(UniqueString modName);

  /**
    This function increments the current revision number stored
    in the context. After it is called, the setters below can
    be used to provide the input at that revision.

    If the prepareToGC argument is true, when processing queries
    in that revision, will prepare to garbage collect (by marking
    elements appropriately).
   */
  void advanceToNextRevision(bool prepareToGC);

  /**
    This function runs garbage collection, but it only has an effect
    if the last call to advanceToNextRevision passed
    prepareToGC=true.
   */
  void collectGarbage();

  // setters for named queries.

  /**
    Sets the file path for the given toplevel module name. It does not bump
    the current revision counter so is suitable for calling from
    a parse query.
    Returns 'true' if this function caused a new result to be saved
    in the context.
   */
  bool setFilePathForModuleName(UniqueString modName, UniqueString path);

  /**
    setFileText will set the text for a particular file path.
    Returns 'true' if this function caused a new result to be saved
    in the context. In that event, the revision updated for the
    file text result will be updated.
   */
  bool setFileText(UniqueString path, std::string data);

  // the following functions are called by the macros defined in QueryImpl.h
  // and should not be called directly

  /// \cond DO_NOT_DOCUMENT
  template<typename... ArgTs>
  void queryTraceBegin(UniqueString queryName, const char* func,
                       const std::tuple<ArgTs...>& tupleOfArg);

  template<typename... ArgTs>
  void queryTraceEnd(UniqueString queryName, const char* func,
                     const std::tuple<ArgTs...>& tupleOfArg,
                     bool changed);


  template<typename ResultType, typename... ArgTs>
  chpl::querydetail::QueryMap<ResultType,ArgTs...>*
    queryGetMap(UniqueString queryName, const std::tuple<ArgTs...>& tupleOfArgs);

  // queryFunc is only used for tracing
  bool queryCanUseSavedResultAndPushIfNot(UniqueString queryName,
      const char* queryFunc,
      chpl::querydetail::QueryMapResultBase* resultEntry);

  template<typename ResultType>
  const ResultType& queryGetSavedResult(chpl::querydetail::QueryMapResult<ResultType>* resultEntry);

  void queryNoteError(ErrorMessage error);
  template<typename ResultType, typename... ArgTs>
  ResultType& queryEnd(UniqueString queryName, const char* func,
                      ResultType result,
                      const std::tuple<ArgTs...>& tupleOfArgs,
                      chpl::querydetail::QueryMap<ResultType,ArgTs...>* queryMap);
  /// \endcond
};

} // end namespace chpl

#endif
