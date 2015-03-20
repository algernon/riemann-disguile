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

The library follows the usual autotools way of installation, and
requires [guile][guile] and [riemann-c-client][rcc] to be available
prior to building. To run the test suite, one also needs
[guile-lib][guile-lib] installed.

 [guile-lib]: http://www.nongnu.org/guile-lib/

    $ git clone git://github.com/algernon/riemann-disguile.git
    $ cd riemann-disguile
    $ autoreconf -i
    $ ./configure && make && make check && make install

From this point onward, the library is installed and fully functional,
and can be used by Guile programs.

Demo
----

A simple program that sends a static event to [Riemann][riemann] is
included below. More examples can be found in the [test suite][tests].

 [tests]: https://github.com/algernon/riemoon/tree/master/tests

```scheme
(use-modules (riemann disguile))

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
