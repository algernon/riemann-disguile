(define-module (disguile)
  #:export (disguile/connect
            disguile/send))

(load-extension "libriemann-disguile" "init_disguile")
