(use-modules (disguile)
             (oop goops)
             (ice-9 format)
             (ice-9 pretty-print)
             (unit-test))

;; disguile/connect

(define-class test-disguile/connect (<test-case>))

(define-method (test-connect/fail (self test-disguile/connect))
  (assert-equal #f
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
  (assert-equal #f
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

(exit-with-summary (run-all-defined-test-cases))
