(define sq (lambda (x) (* x x)))
(write sq)     		; <lambda: ( x ) : ( begin ( * x x ) ) >
(write (sq 10))  	; "100"

(set! a (map sq '(1 2 3 4 5 6 7 8 9)))
(write a)           ; ( "1" "4" "9" "16" "25" "36" "49" "64" "81" )

(set! b (apply + '(1 2 3)))
(write b) ; "6"

(set! c (map (lambda (x) (* x 11)) '(1 2 3 4 5 6 7 8 9)))
(write c) ; ( "11" "22" "33" "44" "55" "66" "77" "88" "99" )

(set! d (apply (lambda (x y z) (+ (* 100 x) (* 10 y) z)) '(1 2 3)))
(write d) ; "123"