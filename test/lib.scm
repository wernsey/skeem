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
            
; `map`, not tail recursing:
; (define map (lambda (f L) (if (null? L) '() (cons (f (car L)) (map f (cdr L))))))

(define (range a b) (if (= a b) (list b) (cons a (range (+ a 1) b)))))

(define (nth n L) (if (or (null? L) (< n 0)) '() (if (= n 1) (car L) (nth (- n 1) (cdr L))))))

; http://community.schemewiki.org/?fold
; https://en.wikibooks.org/wiki/Write_Yourself_a_Scheme_in_48_Hours/Towards_a_Standard_Library
(define (fold f i L) (if (null? L) i (fold f (f (car L) i) (cdr L))))
(define (fold-right f i L) (if (null? L) i (f (car L) (fold-right f i (cdr L)))))
(define (reverse l) (fold cons '() l))
