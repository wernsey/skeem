(define (fac n) (if (< n 2) 1  (* n (fac (- n 1)))))

(define (sum-to n) (if (= n 0) 0 (+ n (sum-to (- n 1)))))

(define (sum2 n acc) (if (= n 0) acc (sum2 (- n 1) (+ n acc))))


(define (fac2 n acc) (if (< n 2) acc (fac2 (- n 1) (* n acc) )))