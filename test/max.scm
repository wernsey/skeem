; This one takes a list as argument
(define (maxn args) 
    (if (null? (cdr args)) 
        (car args) 
        (let ([a (car args)] 
              [b (maxn (cdr args))])
            (if (> a b) a b)
        )
    )
)

; This one is variadic
(define (maxx . args) (fold (lambda (a b) (if (> a b) a b)) (car args) (cdr args)))

; The variadic version could just use the list version:
(define (maxx . args) (maxn args))