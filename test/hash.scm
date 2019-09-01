(define (test t) (if t "PASS" "FAIL *"))
(define (test-not t) (test (not t)))
(define (test-equal a b) (test (equal? a b)))
(define (test-not-equal a b) (test-not (equal? a b)))

(define h (make-hash))
(define not-h '())
(hash-ref h "aaa" "bbb")
(display "Test 1 .............................:" (test (hash? h)))
(display "Test 2 .............................:" (test-not (hash? not-h)))

(display "Test 3 .............................:" (test-equal (hash-ref h "aaa" "x") "x"))
(display "Test 4 .............................:" (test-equal (hash-ref h "aaa" (lambda () (+ 1 3))) "4"))

(display "Test 5 .............................:" (test-equal (hash-set h "aaa" "foo") h))
(display "Test 6 .............................:" (test (hash? h)))
(display "Test 7 .............................:" (test-equal (hash-ref h "aaa" (lambda () (+ 1 3))) "foo"))

(hash-set h "a" "foo")
(hash-set h "b" "bar")
(display "Test 8 .............................:" (test-equal (hash-ref h "a") "foo"))
(display "Test 9 .............................:" (test-equal (hash-ref h "b") "bar"))

(display "Test 10 .............................:" (test (hash-has-key h "b")))
(display "Test 11 .............................:" (test-not (hash-has-key h "c")))

(define h (make-hash))
(hash-set h "a" "foo")
(hash-set h "b" "bar")
(hash-set h "c" "baz")
(hash-set h "d" "fred")

(define L (hash-map h (lambda (k v) (string-append k " => " v) )))
(display "Test 12 .............................:" (test (member? "d => fred" L) ))
(display "Test 13 .............................:" (test (member? "a => foo" L) ))
(display "Test 14 .............................:" (test-not (member? "b => baz" L) ))

(display "Test 15 .............................:" (test-equal (hash-count h) 4 ))
(display "Test 16 .............................:" (test-not (hash-empty? h) ))
(display "Test 17 .............................:" (test (hash-empty? (make-hash)) ))
