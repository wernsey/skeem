; 0'th test: This is a comment and nothing should happen

; First test: display function:
(display "Test 1 .............................: PASS")

; If we can't trust the write function, what can we trust?
(display "Test 2 .............................:")
(write "PASS")

; Second test: if
(display "Test 3 .............................:" (if #t "PASS" "FAIL *"))
(display "Test 4 .............................:" (if #f "FAIL *" "PASS"))

; Third test: equals
(display "Test 5 .............................:" (if (equal? 1 1) "PASS" "FAIL *"))
(display "Test 6 .............................:" (if (equal? 1 2) "FAIL *" "PASS"))

(define (test t) (if t "PASS" "FAIL *"))
(define (test-not t) (if (not t) "PASS" "FAIL *"))
(define (test-equal a b) (test (equal? a b)))
(define (test-not-equal a b) (test-not (equal? a b)))

; Test assignment
(define a 10)
(display "Test 7 - Should display 10 .........:" a)
(display "Test 8 .............................:" (test-equal a 10))
(display "Test 9 .............................:" (test-not-equal a 20))
(define b a)
(display "Test 10 ............................:" (test-not (eq? a 10) ))
(display "Test 11 ............................:" (test (eq? a b) ))

; Lists
(define b (list 1 2 3 4))
(display "Test 12 - Should display (1 2 3 4) .:")
(write b)
(display "Test 13 ............................:" (test (list? b)))
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
(display "Test 23 ............................:" (test-equal (map sq b) '(1 4 9 16) ))
(display "Test 24 ............................:" (test-equal (filter even? b) '(2 4) ))
(display "Test 25 ............................:" (test-equal (fold + 0 (map sq b)) 30 ))
(display "Test 26 ............................:" (test-equal (reverse b) '(4 3 2 1) ))
(display "Test 27 ............................:" (test-equal (length b) 4))
(display "Test 28 ............................:" (test-equal (append '(1 2) '(3 4)) b))

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

(display "Test 38 ............................:" (test (number? '1)))
(display "Test 38 ............................:" (test (number? '+1)))
(display "Test 38 ............................:" (test (number? '-1)))
(display "Test 38 ............................:" (test-not (number? '-)))
(display "Test 38 ............................:" (test-not (number? '+)))
(display "Test 38 ............................:" (test-not (number? 'aaa)))
(display "Test 38 ............................:" (test-not (number? '-1a)))
(display "Test 38 ............................:" (test (number? '-14)))
(display "Test 38 ............................:" (test (number? '-14e-5)))
(display "Test 38 ............................:" (test (number? '-14e1)))
(display "Test 38 ............................:" (test (number? '-14e+5)))
(display "Test 38 ............................:" (test-not (number? '-14e-5-5)))
(display "Test 38 ............................:" (test-not (number? '+)))
(display "Test 38 ............................:" (test-not (number? '-)))
(display "Test 38 ............................:" (test-not (number? 'e)))
(display "Test 38 ............................:" (test-not (number? 'f)))
(display "Test 38 ............................:" (test (number? '1)))
(display "Test 38 ............................:" (test (number? '2)))
(display "Test 38 ............................:" (test (number? '2e3)))
(display "Test 38 ............................:" (test-not (number? '2e3a)))
(display "Test 38 ............................:" (test (number? '2e+3)))
(display "Test 38 ............................:" (test (number? '2e-3)))
(display "Test 38 ............................:" (test-not (number? 'e-3)))
(display "Test 38 ............................:" (test-not (number? '-e-3)))
(display "Test 38 ............................:" (test (number? '-1e-3)))
(display "Test 38 ............................:" (test-not (number? 'a)))
(display "Test 38 ............................:" (test (number? "-1e-3")))
(display "Test 38 ............................:" (test (number? -1e-3)))
(display "Test 38 ............................:" (test-not (number? "")))


(display "Test 98 ............................:" (test-equal "\65\97" "Aa"))
; Because strings are nul-terminated internally, the following are equal:
(display "Test 98 ............................:" (test-equal "xxxx\0xxxxxx" "xxxx"))

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

(display "Test 99 ............................:" (test-equal (string-find "Hello World" "Hello") 0))
(display "Test 99 ............................:" (test-equal (string-find "Hello World" "ll") 2))
(display "Test 99 ............................:" (test-equal (string-find "Hello World" "World") 6))
(display "Test 99 ............................:" (test-equal (string-find "Hello World" "X") '() ))
(display "Test 99 ............................:" (test-equal (string-find "" "Hello") '() ))
(display "Test 99 ............................:" (test (string-contains? "Hello World" "Hello")))
(display "Test 99 ............................:" (test (string-contains? "Hello World" "World")))
(display "Test 99 ............................:" (test-not (string-contains? "Hello World" "XX")))
(display "Test 99 ............................:" (test (string-prefix? "Hello World" "Hello")))
(display "Test 99 ............................:" (test-not (string-prefix? "Hello World" "World")))
(display "Test 99 ............................:" (test-not (string-suffix? "Hello World" "Hello")))
(display "Test 99 ............................:" (test (string-suffix? "Hello World" "World")))

(display "Test 100 ...........................:" (test-equal (serialize string<=?) "(lambda ( a b )  ( begin ( or ( string<? a b ) ( string=? a b ) ) ) ) " ))


(define P '(1 2 3 4))
(define Q '(x y z))
(display "Test 101 ...........................:" (test-equal (append P Q) '( "1" "2" "3" "4" x y z ) ))
; append shouldn't have modified P or Q
(display "Test 101 ...........................:" (test-equal P '( "1" "2" "3" "4" ) ))
(display "Test 101 ...........................:" (test-equal Q '( x y z ) ))

(define space " ")
(display "Test 102 ...........................:" (test-equal (string-append "hello" space "world" space (+ 4 5) ) "hello world 9"))
(display "Test 102 ...........................:" (test-equal (string-append) ""))

(display "Test 103 ...........................:" (test-equal (string-replace "Hello World" "ll" "LL") "HeLLo World"))
(display "Test 103 ...........................:" (test-equal (string-replace "Hello World" "l" "LOL") "HeLOLLOLo WorLOLd"))
(display "Test 103 ...........................:" (test-equal (string-replace "Hello World" "" "LOL") "Hello World"))
(display "Test 103 ...........................:" (test-equal (string-replace "Hello World" "l" "") "Heo Word"))
(display "Test 103 ...........................:" (test-equal (string-replace "Hello World" "o" "0") "Hell0 W0rld"))
(display "Test 103 ...........................:" (test-equal (string-replace "Hello World" "Hello" "Goodbye") "Goodbye World"))
(display "Test 103 ...........................:" (test-equal (string-replace "Hello World" "ld" "LD") "Hello WorLD"))
(display "Test 103 ...........................:" (test-equal (string-replace "Hello World" "xx" "LD") "Hello World"))
(display "Test 103 ...........................:" (test-equal (string-replace "" "hello" "world") ""))
