/*
This test checks the situation
where a required function is param.

We should update this test when possible
to remove the default implementation, 'return 1' here.
*/

interface MyArray {
  proc Self.rank param: int return 1;
}

implements MyArray(Locales.type);

proc cgFun(arg: MyArray) {
  writeln(arg.rank);
}

cgFun(Locales);
