riemann-disguile
================

This is a [Riemann][riemann] client library for the [Guile][guile]
programming language, built on top of [riemann-c-client][rcc]. For
now, it's a work in progress library.

 [riemann]: http://riemann.io/
 [guile]: http://www.gnu.org/software/guile/
 [rcc]: https://github.com/algernon/riemann-c-client

The library uses [semantic versioning][semver].

 [semver]: http://semver.org/

Installation
------------

No can do yet.

Demo
----

A simple program that sends a static event to [Riemann][riemann] is
included below. More examples can be found in the [test suite][tests].

 [tests]: https://github.com/algernon/riemoon/tree/master/tests

```scheme
(use-modules (disguile))

(define client (disguile/connect #:tcp "127.0.0.1" 5555))

(disguile/send client
  '((host . "localhost")
    (service . "demo-client")
    (state . "ok")
    (tags . ("demo-client" "riemann-disguile"))
    (x-client . "riemann-disguile")))
```

License
-------

Copyright (C) 2015 Gergely Nagy <algernon@madhouse-project.org>,
released under the terms of the
[GNU Lesser General Public License][lgpl], version 3+.

 [lgpl]: http://www.gnu.org/licenses/lgpl.html
