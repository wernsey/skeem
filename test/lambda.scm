;(define x (lambda (a b) (+ a b)))
;(x 3 4)

(define c 9999)
(define a 9999)

 (define x (lambda (a b) (begin
 	(set! c 1000)
 	(+ a b c)
 )))
 (x 3 4)
 

(display "c is " c)

(display "call result: " (x 3 4))

(display "c is " c)
(display "a is " a)

((lambda (x) (display "XXX " x)) 10)

(define hello (lambda () "Hello World"))
(hello)