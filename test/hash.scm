(define (test t) (if t "PASS" "FAIL *"))
(define (test-not t) (test (not t)))
(define (test-equal a b) (test (equal? a b)))
(define (test-not-equal a b) (test-not (equal? a b)))

(define h (make-hash))
(define not-h '())
(hash-ref h "aaa" "bbb")
(display "Test 8 .............................:" (test (hash? h)))
(display "Test 8 .............................:" (test-not (hash? not-h)))

(display "Test 8 .............................:" (test-equal (hash-ref h "aaa" "x") "x"))
(display "Test 8 .............................:" (test-equal (hash-ref h "aaa" (lambda () (+ 1 3))) "4"))

(display "Test 8 .............................:" (test-equal (hash-set h "aaa" "foo") h))
(display "Test 8 .............................:" (test (hash? h)))
(display "Test 8 .............................:" (test-equal (hash-ref h "aaa" (lambda () (+ 1 3))) "foo"))

(hash-set h "a" "foo")
(hash-set h "b" "bar")
(display "Test 8 .............................:" (test-equal (hash-ref h "a") "foo"))
(display "Test 8 .............................:" (test-equal (hash-ref h "b") "bar"))

(display "Test 8 .............................:" (test (hash-has-key h "b")))
(display "Test 8 .............................:" (test-not (hash-has-key h "c")))