; 0'th test: This is a comment and nothing should happen

; First test: display function:
(display "First test (1).................: PASS")
(display "First test (2).................:")

(write "PASS")

; Second test: if
(display "Second test (1) ...............:" (if #t "PASS" "FAIL"))
(display "Second test (2) ...............:" (if #f "FAIL" "PASS"))

; Third test: equals
(display "Test 3 (1) ....................:" (if (equal? 1 1) "PASS" "FAIL"))
(display "Test 3 (2) ....................:" (if (equal? 1 2) "FAIL" "PASS"))

; Test assignment
(define a 10)
(display "Test 4: Should display 10 .....:" a)
(display "Test 4 (1) ....................:" (if (equal? a 10) "PASS" "FAIL"))
(display "Test 4 (2) ....................:" (if (equal? a 20) "FAIL" "PASS"))
(define b a)
(display "Test 4 (3) ....................:" (if (eq? a 10) "FAIL" "PASS"))
(display "Test 4 (4) ....................:" (if (eq? a b) "PASS" "FAIL"))

; Lists
(define b (list 1 2 3 4))
(display "Test 5: Should display (1 2 3 4):")
(write b)
(display "Test 5 (0) ....................:" (if (list? b) "PASS" "FAIL"))
(display "Test 5 (1) ....................:" (if (equal? b (list 1 2 3 4)) "PASS" "FAIL"))
(display "Test 5 (2) ....................:" (if (equal? b (list 1 2 3 5)) "FAIL" "PASS"))
(display "Test 5 (3) ....................:" (if (equal? (car b) 1) "PASS" "FAIL"))
(display "Test 5 (4) ....................:" (if (equal? (cdr b) (list 2 3 4)) "PASS" "FAIL"))

; Dotted pair
(display "Test 6 (a): Should display (1 . 2):")
(write (cons 1 2))

; prepend to a list with `cons`:
(display "Test 6 (b): Should display (10 1 2 3 4):")
(write (cons a b))

; Apply
(display "Test 7 ........................:" (if (equal? (apply + b) 10) "PASS" "FAIL"))

; quotes
(display "Test 8: Should display (1 2 3 4):")
(write '(1 2 3 4))
(display "Test 8 ........................:" (if (equal? b '(1 2 3 4)) "PASS" "FAIL"))

; list functions
(define (sq x) (* x x))
(define (even? x) (= 0 (% x 2)))
(display "Test 8 (1) ....................:" (if (equal? (map sq b) '(1 4 9 16)) "PASS" "FAIL"))
(display "Test 8 (2) ....................:" (if (equal? (filter even? b) '(2 4)) "PASS" "FAIL"))
(display "Test 8 (3) ....................:" (if (equal? (fold + 0 (map sq b)) 30) "PASS" "FAIL"))
(display "Test 8 (4) ....................:" (if (equal? (reverse b) '(4 3 2 1)) "PASS" "FAIL"))
(display "Test 8 (5) ....................:" (if (equal? (length b) 4) "PASS" "FAIL"))
(display "Test 8 (6) ....................:" (if (equal? (append '(1 2) '(3 4)) b) "PASS" "FAIL"))

; Let
(define X 100)
(define Y 200)
(let ((X 500) (Y 600))
    (display "Test 9 (1) ....................:" (if (equal? X 500) "PASS" "FAIL"))
    (display "Test 9 (2) ....................:" (if (equal? Y 600) "PASS" "FAIL"))
    (let ((X 300) (Y (+ X 1000)))
        (display "Test 9 (3) ....................:" (if (equal? X 300) "PASS" "FAIL"))
        (display "Test 9 (4) ....................:" (if (equal? Y 1500) "PASS" "FAIL"))
        (display "Test 9 (4b) ...................:" (if (equal? Y 1300) "FAIL" "PASS"))
    )
    (display "Test 9 (5) ....................:" (if (equal? X 500) "PASS" "FAIL"))
    (display "Test 9 (6) ....................:" (if (equal? Y 600) "PASS" "FAIL"))
)
(display "Test 9 (7) ....................:" (if (equal? X 100) "PASS" "FAIL"))
(display "Test 9 (8) ....................:" (if (equal? Y 200) "PASS" "FAIL"))
