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
            
; http://community.schemewiki.org/?fold
; https://en.wikibooks.org/wiki/Write_Yourself_a_Scheme_in_48_Hours/Towards_a_Standard_Library
(define (fold f i s) (if (null? s) i (fold f (f (car s) i) (cdr s))))
(define (fold-right f i s) (if (null? s) i (f (car s) (foldr f i (cdr s)))))
(define (reverse l) (fold cons '() l))

