; 0'th test: This is a comment and nothing should happen

; First test: display function:
(display "Test 1 .............................: PASS")

; If we can't trust the write function, what can we trust?
(display "Test 2 .............................:")
(write "PASS")

; Second test: if
(display "Test 3 .............................:" (if #t "PASS" "FAIL"))
(display "Test 4 .............................:" (if #f "FAIL" "PASS"))

; Third test: equals
(display "Test 5 .............................:" (if (equal? 1 1) "PASS" "FAIL"))
(display "Test 6 .............................:" (if (equal? 1 2) "FAIL" "PASS"))

(define (test-equal a b) (if (equal? a b) "PASS" "FAIL"))
(define (test-not-equal a b) (if (equal? a b) "FAIL" "PASS"))

; Test assignment
(define a 10)
(display "Test 7 - Should display 10 .........:" a)
(display "Test 8 .............................:" (test-equal a 10))
(display "Test 9 .............................:" (test-not-equal a 20))
(define b a)
(display "Test 10 ............................:" (if (eq? a 10) "FAIL" "PASS"))
(display "Test 11 ............................:" (if (eq? a b) "PASS" "FAIL"))

; Lists
(define b (list 1 2 3 4))
(display "Test 12 - Should display (1 2 3 4) .:")
(write b)
(display "Test 13 ............................:" (if (list? b) "PASS" "FAIL"))
(display "Test 14 ............................:" (test-equal b (list 1 2 3 4)))
(display "Test 15 ............................:" (test-not-equal b (list 1 2 3 5)))
(display "Test 16 ............................:" (test-equal (car b) 1))
(display "Test 17 ............................:" (test-equal (cdr b) (list 2 3 4)))

; Dotted pair
(display "Test 18 - Should display (1 . 2) ...:")
(write (cons 1 2))

; prepend to a list with `cons`:
(display "Test 19 - Should show (10 1 2 3 4) .:")
(write (cons a b))

; Apply
(display "Test 20 ............................:" (test-equal (apply + b) 10))

; quotes
(display "Test 21: Should display (1 2 3 4) ..:")
(write '(1 2 3 4))
(display "Test 22 ............................:" (test-equal b '(1 2 3 4)))

; list functions
(define (sq x) (* x x))
(define (even? x) (= 0 (% x 2)))
(display "Test 23 ............................:" (if (equal? (map sq b) '(1 4 9 16)) "PASS" "FAIL"))
(display "Test 24 ............................:" (if (equal? (filter even? b) '(2 4)) "PASS" "FAIL"))
(display "Test 25 ............................:" (if (equal? (fold + 0 (map sq b)) 30) "PASS" "FAIL"))
(display "Test 26 ............................:" (if (equal? (reverse b) '(4 3 2 1)) "PASS" "FAIL"))
(display "Test 27 ............................:" (if (equal? (length b) 4) "PASS" "FAIL"))
(display "Test 28 ............................:" (if (equal? (append '(1 2) '(3 4)) b) "PASS" "FAIL"))

; Let
(define X 100)
(define Y 200)
(let ((X 500) (Y 600))
    (display "Test 29 ............................:" (test-equal X 500))
    (display "Test 30 ............................:" (test-equal Y 600))
    (let ((X 300) (Y (+ X 1000)))
        (display "Test 31 ............................:" (test-equal X 300))
        (display "Test 32 ............................:" (test-equal Y 1500))
        (display "Test 33 ............................:" (test-not-equal Y 1300))
    )
    (display "Test 34 ............................:" (test-equal X 500))
    (display "Test 35 ............................:" (test-equal Y 600))
)
(display "Test 36 ............................:" (test-equal X 100))
(display "Test 37 ............................:" (test-equal Y 200))


; Because strings are null terminated internally, the following are equal:
(display "Test 98 ............................:" (test-equal "xxxx\0xxxxxx" "xxxx"))
(display "Test 98 ............................:" (test-equal "\65\97" "Aa"))

(display "Test 98 ............................:" (test-equal (substring "Hello World" 6) "World"))
(display "Test 98 ............................:" (test-equal (substring "Hello World" -1) ""))
(display "Test 98 ............................:" (test-equal (substring "Hello World" 0 5) "Hello"))
(display "Test 98 ............................:" (test-equal (substring "Hello World" 1 5) "ello"))
(display "Test 98 ............................:" (test-equal (substring "Hello World" 20) ""))
(display "Test 98 ............................:" (test-equal (substring "Hello World" 11) ""))
(display "Test 98 ............................:" (test-equal (substring "Hello World" 10) "d"))
(display "Test 98 ............................:" (test-equal (substring "Hello World" 6 11) "World"))
(display "Test 98 ............................:" (test-equal (substring "Hello World" 6 20) "World"))

(display "Test 99 ............................:" (test-equal (string-split "a bb ccc dddd") '( "a" "bb" "ccc" "dddd" ) ))
(display "Test 99 ............................:" (test-equal (string-split "a") '( "a" ) ))
(display "Test 99 ............................:" (test-equal (string-split "") '( "" ) ))
(display "Test 99 ............................:" (test-equal (string-split "a  b") '( "a" "" "b" ) ))
(display "Test 99 ............................:" (test-equal (string-split "a-bb;ccc;dddd" ";-") '( "a" "bb" "ccc" "dddd" ) ))
(display "Test 99 ............................:" (test-equal (string-split "axbbxcccxdddd" "x") '( "a" "bb" "ccc" "dddd" ) ))

(display "Test 99 ............................:" (test-equal (string-upcase) ""))
(display "Test 99 ............................:" (test-equal (string-upcase "Hello World!") "HELLO WORLD!"))
(display "Test 99 ............................:" (test-equal (string-downcase) ""))
(display "Test 99 ............................:" (test-equal (string-downcase "Hello World!") "hello world!"))

(display "Test 99 ............................:" (test-equal (string-ascii) 0))
(display "Test 99 ............................:" (test-equal (string-ascii "A") 65))
(display "Test 99 ............................:" (test-equal (string-ascii "a") 97))
(display "Test 99 ............................:" (test-equal (string-ascii "") 0))

(display "Test 99 ............................:" (test-equal (string-char) ""))
(display "Test 99 ............................:" (test-equal (string-char 97) "a"))
(display "Test 99 ............................:" (test-equal (string-char 65) "A"))
(display "Test 99 ............................:" (test-equal (string-char 65.5) "A"))
(display "Test 99 ............................:" (test-equal (string-char 0) ""))
; Only lowest 7 bits are used:
(display "Test 99 ............................:" (test-equal (string-char (+ 128 65)) "A"))

(display "Test 99 ............................:" (test-equal (string-trim) ""))
(display "Test 99 ............................:" (test-equal (string-trim "") ""))
(display "Test 99 ............................:" (test-equal (string-trim "         ") ""))
(display "Test 99 ............................:" (test-equal (string-trim "     X    ") "X"))
(display "Test 99 ............................:" (test-equal (string-trim "\n\n\rX\n\n\r\t") "X"))

(display "Test 99 ............................:" (test-equal (map string-trim (string-split " A; B ; C - D - E   ; \tF " ";-")) '( "A" "B" "C" "D" "E" "F" )))



