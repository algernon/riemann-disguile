(define-module (disguile)
  #:export (disguile/connect
            disguile/send
            disguile/query))

(load-extension "libriemann-disguile" "init_disguile")
