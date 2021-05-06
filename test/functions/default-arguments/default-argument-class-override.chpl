class C {
  proc foo(x = 10) { writeln("C.foo:", x); }
}

class D : C {
  override proc foo(x = 20) { writeln("D.foo:", x); }
}

var d = new borrowed D();
d.foo();

var c: borrowed C = new borrowed D();
c.foo();

var c2: borrowed C = new borrowed C();
c2.foo();
