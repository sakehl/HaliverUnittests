(declare-const min Int)
(declare-const extent Int)
(declare-const factor Int)
(declare-const i Int)
(assert
  (and
    (> extent 0)
    (> factor 0)
    (>= i min)
    (< i (+ min extent))
  )
)

(assert (not 
  (<= 
    (* (- i min) factor)
    (* (- extent 1) factor))
  )
)
(check-sat)