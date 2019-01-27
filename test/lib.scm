(define (member? x l) 
    (if (null? l) 
        #f 
        (if (equal? x (car l)) 
            #t 
            (member? x (cdr l)) )) )

(define (member x l) 
    (if (null? l) 
        #f 
        (if (equal? x (car l)) 
            l 
            (member x (cdr l)) )) )
            

;(define (map2 f ls acc) 
;    (if (null? ls) 
;        acc 
;        (map2 f (cdr ls) (cons (apply f (list (car ls))) acc))
;    )
;)

(define (reverse ls acc) (if (null? ls)
    acc
    (reverse (cdr ls) (cons (car ls) acc))
))

; (reverse '(1 2 3 4 5 6) '())

(define (map2 f ls) (begin
    (set! (mapr f ls acc) 
        (if (null? ls) 
            acc 
            (mapr f (cdr ls) (cons (f (car ls)) acc))
        )
    )
    (reverse (mapr f ls '()) '())
))

