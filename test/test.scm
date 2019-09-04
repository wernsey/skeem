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
(define (test-not t) (test (not t)))
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
(display "Test 29 ............................:" (test-equal (append '() '(1 2 3 4)) b))
(display "Test 30 ............................:" (test-equal (append '(1 2 3 4) '()) b))

; Let
(define X 100)
(define Y 200)
(let ((X 500) (Y 600))
    (display "Test 31 ............................:" (test-equal X 500))
    (display "Test 32 ............................:" (test-equal Y 600))
    (let ((X 300) (Y (+ X 1000)))
        (display "Test 33 ............................:" (test-equal X 300))
        (display "Test 34 ............................:" (test-equal Y 1500))
        (display "Test 35 ............................:" (test-not-equal Y 1300))
    )
    (display "Test 36 ............................:" (test-equal X 500))
    (display "Test 37 ............................:" (test-equal Y 600))
)
(display "Test 38 ............................:" (test-equal X 100))
(display "Test 39 ............................:" (test-equal Y 200))

; Let*
(display "Test 40 ............................:" (test-equal X 100))
(display "Test 41 ............................:" (test-equal Y 200))
(let* [(X 500) (Y (+ X 100))]
    (display "Test 42 ............................:" (test-equal X 500))
    (display "Test 43 ............................:" (test-equal Y 600))
    (let* [(X (+ Y 100)) (Y (+ X 1000)) (Z (+ X Y))]
        (display "Test 44 ............................:" (test-equal X 700))
        (display "Test 45 ............................:" (test-equal Y 1700))
        (display "Test 46 ............................:" (test-equal Z 2400))
    )
    (let [(X (+ Y 100)) (Y (+ X 1000)) (Z (+ X Y))]
        (display "Test 47 ............................:" (test-equal X 700))
        (display "Test 48 ............................:" (test-equal Y 1500))
        (display "Test 49 ............................:" (test-equal Z 1100))
    )
    (display "Test 50 ............................:" (test-equal X 500))
    (display "Test 51 ............................:" (test-equal Y 600))
)
(display "Test 52 ............................:" (test-equal X 100))
(display "Test 53 ............................:" (test-equal Y 200))

(display "Test 54 ............................:" (test (number? '1)))
(display "Test 55 ............................:" (test (number? '+1)))
(display "Test 56 ............................:" (test (number? '-1)))
(display "Test 57 ............................:" (test-not (number? '-)))
(display "Test 58 ............................:" (test-not (number? '+)))
(display "Test 59 ............................:" (test-not (number? 'aaa)))
(display "Test 60 ............................:" (test-not (number? '-1a)))
(display "Test 61 ............................:" (test (number? '-14)))
(display "Test 62 ............................:" (test (number? '-14e-5)))
(display "Test 63 ............................:" (test (number? '-14e1)))
(display "Test 64 ............................:" (test (number? '-14e+5)))
(display "Test 65 ............................:" (test-not (number? '-14e-5-5)))
(display "Test 66 ............................:" (test-not (number? '+)))
(display "Test 67 ............................:" (test-not (number? '-)))
(display "Test 68 ............................:" (test-not (number? 'e)))
(display "Test 69 ............................:" (test-not (number? 'f)))
(display "Test 70 ............................:" (test (number? '1)))
(display "Test 71 ............................:" (test (number? '2)))
(display "Test 72 ............................:" (test (number? '2e3)))
(display "Test 73 ............................:" (test-not (number? '2e3a)))
(display "Test 74 ............................:" (test (number? '2e+3)))
(display "Test 75 ............................:" (test (number? '2e-3)))
(display "Test 76 ............................:" (test-not (number? 'e-3)))
(display "Test 77 ............................:" (test-not (number? '-e-3)))
(display "Test 78 ............................:" (test (number? '-1e-3)))
(display "Test 79 ............................:" (test-not (number? 'a)))
(display "Test 80 ............................:" (test (number? "-1e-3")))
(display "Test 81 ............................:" (test (number? -1e-3)))
(display "Test 82 ............................:" (test-not (number? "")))

; Some older tests ended up here
(define sq (lambda (x) (* x x)))
(display "Test 83 ............................:" (test-equal (sq 10) 100))
(display "Test 84 ............................:" (test-equal (sq 16) 256))
(display "Test 85 ............................:" (test-equal (map sq '(1 2 3 4 5 6 7 8 9)) '( "1" "4" "9" "16" "25" "36" "49" "64" "81" )))
(display "Test 86 ............................:" (test-equal (apply + '(1 2 3)) 6 ))
(display "Test 87 ............................:" (test-equal (map (lambda (x) (* x 11)) '(1 2 3 4 5 6 7 8 9))  '( "11" "22" "33" "44" "55" "66" "77" "88" "99" ) ))
(display "Test 88 ............................:" (test-equal (apply (lambda (x y z) (+ (* 100 x) (* 10 y) z)) '(1 2 3))  123 ))

; Use \number to input other special characters
(display "Test 89 ............................:" (test-equal "\65\97" "Aa"))
; Because strings are nul-terminated internally, the following are equal:
(display "Test 90 ............................:" (test-equal "xxxx\0xxxxxx" "xxxx"))

(display "Test 91 ............................:" (test-equal (substring "Hello World" 6) "World"))
(display "Test 92 ............................:" (test-equal (substring "Hello World" -1) ""))
(display "Test 93 ............................:" (test-equal (substring "Hello World" 0 5) "Hello"))
(display "Test 94 ............................:" (test-equal (substring "Hello World" 1 5) "ello"))
(display "Test 95 ............................:" (test-equal (substring "Hello World" 20) ""))
(display "Test 96 ............................:" (test-equal (substring "Hello World" 11) ""))
(display "Test 97 ............................:" (test-equal (substring "Hello World" 10) "d"))
(display "Test 98 ............................:" (test-equal (substring "Hello World" 6 11) "World"))
(display "Test 99 ............................:" (test-equal (substring "Hello World" 6 20) "World"))

(display "Test 100 ...........................:" (test-equal (string-split "a bb ccc dddd") '( "a" "bb" "ccc" "dddd" ) ))
(display "Test 101 ...........................:" (test-equal (string-split "a") '( "a" ) ))
(display "Test 102 ...........................:" (test-equal (string-split "") '( "" ) ))
(display "Test 103 ...........................:" (test-equal (string-split "a  b") '( "a" "" "b" ) ))
(display "Test 104 ...........................:" (test-equal (string-split "a-bb;ccc;dddd" ";-") '( "a" "bb" "ccc" "dddd" ) ))
(display "Test 105 ...........................:" (test-equal (string-split "axbbxcccxdddd" "x") '( "a" "bb" "ccc" "dddd" ) ))

(display "Test 106 ...........................:" (test-equal (string-upcase) ""))
(display "Test 107 ...........................:" (test-equal (string-upcase "Hello World!") "HELLO WORLD!"))
(display "Test 108 ...........................:" (test-equal (string-downcase) ""))
(display "Test 109 ...........................:" (test-equal (string-downcase "Hello World!") "hello world!"))

(display "Test 110 ...........................:" (test-equal (string-ascii) 0))
(display "Test 111 ...........................:" (test-equal (string-ascii "A") 65))
(display "Test 112 ...........................:" (test-equal (string-ascii "a") 97))
(display "Test 113 ...........................:" (test-equal (string-ascii "") 0))

(display "Test 114 ...........................:" (test-equal (string-char) ""))
(display "Test 115 ...........................:" (test-equal (string-char 97) "a"))
(display "Test 116 ...........................:" (test-equal (string-char 65) "A"))
(display "Test 117 ...........................:" (test-equal (string-char 65.5) "A"))
(display "Test 118 ...........................:" (test-equal (string-char 0) ""))
; Only lowest 7 bits are used:
(display "Test 119 ...........................:" (test-equal (string-char (+ 128 65)) "A"))

(display "Test 120 ...........................:" (test-equal (string-trim) ""))
(display "Test 121 ...........................:" (test-equal (string-trim "") ""))
(display "Test 122 ...........................:" (test-equal (string-trim "         ") ""))
(display "Test 123 ...........................:" (test-equal (string-trim "     X    ") "X"))
(display "Test 124 ...........................:" (test-equal (string-trim "\n\n\rX\n\n\r\t") "X"))

(display "Test 125 ...........................:" (test-equal (map string-trim (string-split " A; B ; C - D - E   ; \tF " ";-")) '( "A" "B" "C" "D" "E" "F" )))

(display "Test 126 ...........................:" (test-equal (string-find "Hello World" "Hello") 0))
(display "Test 127 ...........................:" (test-equal (string-find "Hello World" "ll") 2))
(display "Test 128 ...........................:" (test-equal (string-find "Hello World" "World") 6))
(display "Test 129 ...........................:" (test-equal (string-find "Hello World" "X") '() ))
(display "Test 130 ...........................:" (test-equal (string-find "" "Hello") '() ))
(display "Test 131 ...........................:" (test (string-contains? "Hello World" "Hello")))
(display "Test 132 ...........................:" (test (string-contains? "Hello World" "World")))
(display "Test 133 ...........................:" (test-not (string-contains? "Hello World" "XX")))
(display "Test 134 ...........................:" (test (string-prefix? "Hello World" "Hello")))
(display "Test 135 ...........................:" (test-not (string-prefix? "Hello World" "World")))
(display "Test 136 ...........................:" (test-not (string-suffix? "Hello World" "Hello")))
(display "Test 137 ...........................:" (test (string-suffix? "Hello World" "World")))

(display "Test 138 ...........................:" (test-equal (serialize string<=?) "(lambda ( a b )  ( begin ( or ( string<? a b ) ( string=? a b ) ) ) ) " ))

(define P '(1 2 3 4))
(define Q '(x y z))
(display "Test 139 ...........................:" (test-equal (append P Q) '( "1" "2" "3" "4" x y z ) ))
; append shouldn't have modified P or Q
(display "Test 140 ...........................:" (test-equal P '( "1" "2" "3" "4" ) ))
(display "Test 141 ...........................:" (test-equal Q '( x y z ) ))

(define space " ")
(display "Test 142 ...........................:" (test-equal (string-append "hello" space "world" space (+ 4 5) ) "hello world 9"))
(display "Test 143 ...........................:" (test-equal (string-append) ""))

(display "Test 144 ...........................:" (test-equal (string-replace "Hello World" "ll" "LL") "HeLLo World"))
(display "Test 145 ...........................:" (test-equal (string-replace "Hello World" "l" "LOL") "HeLOLLOLo WorLOLd"))
(display "Test 146 ...........................:" (test-equal (string-replace "Hello World" "" "LOL") "Hello World"))
(display "Test 147 ...........................:" (test-equal (string-replace "Hello World" "l" "") "Heo Word"))
(display "Test 148 ...........................:" (test-equal (string-replace "Hello World" "o" "0") "Hell0 W0rld"))
(display "Test 149 ...........................:" (test-equal (string-replace "Hello World" "Hello" "Goodbye") "Goodbye World"))
(display "Test 150 ...........................:" (test-equal (string-replace "Hello World" "ld" "LD") "Hello WorLD"))
(display "Test 151 ...........................:" (test-equal (string-replace "Hello World" "xx" "LD") "Hello World"))
(display "Test 152 ...........................:" (test-equal (string-replace "" "hello" "world") ""))

(display "Test 153 ...........................:" (test (member? 6 '(3 4 6 7 3 2 1 8 4 3 7 6))))
(display "Test 154 ...........................:" (test (member? 3 '(3 4 6 7 3 2 1 8 4 3 7 6))))
(display "Test 155 ...........................:" (test-not (member? 9 '(3 4 6 7 3 2 1 8 4 3 7 6))))
(display "Test 156 ...........................:" (test-not (member? 0 '(3 4 6 7 3 2 1 8 4 3 7 6))))
(display "Test 157 ...........................:" (test-equal (member 1 '[3 4 6 7 3 2 1 8 4 3 7 6]) '[1 8 4 3 7 6] ))
(display "Test 158 ...........................:" (test-equal (member 0 '[3 4 6 7 3 2 1 8 4 3 7 6]) #f ))

; Dotted pair notation
(display "Test 159 ...........................:" (test-equal '( 1 . 2 ) (cons 1 2)))
(display "Test 160 ...........................:" (test-equal '( 1 . ( 2 . ( 3 . ( 4 . ())))) (list 1 2 3 4)))

; Variadic lambdas
(define sum (lambda args (apply + args)))
(display "Test 161 ...........................:" (test-equal (sum 1 2 3 4 5 6 7) 28))
(define foo (lambda (a b . c) (* (+ a b) (apply + c))))
(display "Test 162 ...........................:" (test-equal (foo 5 5 1 2 3 4 5 6 7) 280))
(define (bar a b . c) (* (+ a b) (apply + c)))
(display "Test 163 ...........................:" (test-equal (bar 5 5 1 2 3 4 5 6 7) 280))

; min and max are also declared as variadic lambdas
(display "Test 164 ...........................:" (test-equal (max 3 4 6 7 3 2 1 8 4 3 7 6) 8))
(display "Test 165 ...........................:" (test-equal (min 3 4 6 7 3 2 1 8 4 3 7 6) 1))

; This one takes a list as argument
(define (maxn args)
    (if (null? (cdr args))
        (car args)
        (let ([a (car args)]
              [b (maxn (cdr args))])
            (if (> a b) a b)
        )))
; The variadic version could just use the list version:
(define (maxx . args) (maxn args))

(display "Test 166 ...........................:" (test-equal (maxx 3 4 6 7 3 2 1 8 4 3 7 6) 8))
(display "Test 167 ...........................:" (test-equal (maxn '(3 4 6 7 3 2 1 8 4 3 7 6)) 8))

(define (test t) (if t "PASS" "FAIL *"))
(define (test-not t) (test (not t)))
(define (test-equal a b) (test (equal? a b)))
(define (test-not-equal a b) (test-not (equal? a b)))

(define h (make-hash))
(define not-h '())
(hash-ref h "aaa" "bbb")
(display "Test 168 ...........................:" (test (hash? h)))
(display "Test 169 ...........................:" (test-not (hash? not-h)))

(display "Test 170 ...........................:" (test-equal (hash-ref h "aaa" "x") "x"))
(display "Test 171 ...........................:" (test-equal (hash-ref h "aaa" (lambda () (+ 1 3))) "4"))

(display "Test 172 ...........................:" (test-equal (hash-set h "aaa" "foo") h))
(display "Test 173 ...........................:" (test (hash? h)))
(display "Test 174 ...........................:" (test-equal (hash-ref h "aaa" (lambda () (+ 1 3))) "foo"))

(hash-set h "a" "foo")
(hash-set h "b" "bar")
(display "Test 175 ...........................:" (test-equal (hash-ref h "a") "foo"))
(display "Test 176 ...........................:" (test-equal (hash-ref h "b") "bar"))

(display "Test 177 ...........................:" (test (hash-has-key h "b")))
(display "Test 178 ...........................:" (test-not (hash-has-key h "c")))

(define h (make-hash))
(hash-set h "a" "foo")
(hash-set h "b" "bar")
(hash-set h "c" "baz")
(hash-set h "d" "fred")

(define L (hash-map h (lambda (k v) (string-append k " => " v) )))
(display "Test 179 ...........................:" (test (member? "d => fred" L) ))
(display "Test 180 ...........................:" (test (member? "a => foo" L) ))
(display "Test 181 ...........................:" (test-not (member? "b => baz" L) ))

(display "Test 182 ...........................:" (test-equal (hash-count h) 4 ))
(display "Test 183 ...........................:" (test-not (hash-empty? h) ))
(display "Test 184 ...........................:" (test (hash-empty? (make-hash)) ))

(define H (make-hash '[("a" . "FOO") ("b" . "BAR") ( "c" . "BAZ") ("d" . "FRED") ] ))

(define L (hash-map H (lambda (k v) (string-append k " => " v) )))
(display "Test 185 ...........................:" (test (member? "d => FRED" L) ))
(display "Test 186 ...........................:" (test (member? "a => FOO" L) ))
(display "Test 187 ...........................:" (test-not (member? "b => BAZ" L) ))
