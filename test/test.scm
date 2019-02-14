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
(display "Test 6 ........................:" (if (equal? (apply + b) 10) "PASS" "FAIL"))

; quotes
(display "Test 7: Should display (1 2 3 4):")
(write '(1 2 3 4))
(display "Test 7 ........................:" (if (equal? b '(1 2 3 4)) "PASS" "FAIL"))
