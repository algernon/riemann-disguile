(use-modules (disguile)
             (oop goops)
             (ice-9 format)
             (ice-9 pretty-print)
             (unit-test))

;; disguile/connect

(define-class test-disguile/connect (<test-case>))

(define-method (test-connect/fail (self test-disguile/connect))
  (assert-exception
   (disguile/connect #:tcp "127.0.0.1" 6555)))

(define-method (test-connect/tcp (self test-disguile/connect))
  (assert-true
   (not (eq? (disguile/connect #:tcp "127.0.0.1" 5555)
             #f))))

(define-method (test-connect/udp (self test-disguile/connect))
  (assert-true
   (not (eq? (disguile/connect #:udp "127.0.0.1" 5555)
             #f))))

(define-method (test-connect/invalid (self test-disguile/connect))
  (assert-exception
   (disguile/connect #:invalid "127.0.0.1" 6555)))

(define-method (test-connect/display (self test-disguile/connect))
  (assert-equal "#<disguile-client #:tcp 127.0.0.1:5555>"
                (format #f "~a" (disguile/connect #:tcp "127.0.0.1" 5555))))

;; disguile/send

(define-class test-disguile/send (<test-case>)
  (test-connection #:getter test-connection
                   #:init-value (disguile/connect #:tcp "127.0.0.1" 5555)))

(define-method (test-send/no-connection (self test-disguile/send))
  (assert-exception (disguile/send #f
                                   '((service . "disguile unit tests")))))

(define-method (test-send/invalid-events (self test-disguile/send))
  (assert-equal #f
                (disguile/send (test-connection self)
                               '())))

(define-method (test-send/invalid-events-2 (self test-disguile/send))
  (assert-equal #f
                (disguile/send (test-connection self)
                               #f)))

(define-method (test-send/invalid-events-3 (self test-disguile/send))
  (assert-equal #f
                (disguile/send (test-connection self)
                               "foobar")))

(define-method (test-send/single (self test-disguile/send))
  (assert-equal #t
                (disguile/send (test-connection self)
                               '((service . "disguile unit tests")))))

(define-method (test-send/multiple (self test-disguile/send))
  (assert-equal #t
                (disguile/send (test-connection self)
                               '((service . "disguile unit tests"))
                               '((service . "disguile unit tests #2")))))

(define-method (test-send/all-fields (self test-disguile/send))
  (assert-equal #t
                (disguile/send (test-connection self)
                               '((time . 12345)
                                 (state . "ok")
                                 (service . "disguile unit tests - all fields")
                                 (host . "localhost")
                                 (description . "some description")
                                 (tags . ("tag-1" "tag-2"))
                                 (metric . 4.2)
                                 (ttl . 12345.12345)
                                 (x-guile . "yes")))))

;; disguile/query

(define-class test-disguile/query (<test-case>)
  (test-connection #:getter test-connection
                   #:init-value (disguile/connect #:tcp "127.0.0.1" 5555)))

(define-method (test-query/no-connection (self test-disguile/query))
  (assert-exception (disguile/query #f
                                    "true")))

(define-method (test-query/invalid-query (self test-disguile/query))
  (assert-exception (disguile/query (test-connection self)
                                    "error =")))

(define-method (test-query/no-results (self test-disguile/query))
  (assert-equal '()
                (disguile/query (test-connection self)
                                "service = \"no-such-service\"")))

(define-method (test-query/one-result (self test-disguile/query))
  (disguile/send (disguile/connect #:tcp "127.0.0.1" 5555)
                 '((state . "ok")
                   (service . "disguile/query/one")
                   (host . "localhost")
                   (description . "disguile unit test: query")
                   (tags . ("disguile" "query"))
                   (metric . 4.2)
                   (ttl . 60.5)
                   (x-guile . "yes")))
  (let* ((results (disguile/query (test-connection self)
                                  "service = \"disguile/query/one\""))
         (event (car results)))
    (assert-equal 1 (length results))

    (assert-equal "ok"
                  (assoc-ref event 'state))
    (assert-equal "disguile/query/one"
                  (assoc-ref event 'service))
    (assert-equal "localhost"
                  (assoc-ref event 'host))
    (assert-equal "disguile unit test: query"
                  (assoc-ref event 'description))
    (assert-equal 4.2
                  (assoc-ref event 'metric))
    (assert-equal 60.5
                  (assoc-ref event 'ttl))
    (assert-equal "yes"
                  (assoc-ref event 'x-guile))
    (assert-equal '("disguile" "query")
                  (assoc-ref event 'tags))))

(exit-with-summary (run-all-defined-test-cases))
