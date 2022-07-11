; Solving for Ax=b via the jacobi method

; A is a diagonally dominant matrix
(def A (jo/matrix 4 4 [10 -1  2  0,
                    -1 11 -1  3,
                     2 -1 10 -1,
                     0  3 -1  8]))
; initial value for x is all zeros
(def x (jo/matrix 1 4 [0, 0, 0, 0]))
(def b (jo/matrix 1 4 [6, 25, -11, 15]))

(def D-vec (jo/diag A))
(def D-mat (jo/matrix 1 4 D-vec))
(def LU (jo/matrix-sub A (jo/diag-matrix D-vec)))

(println "System:")
(println "A =" A)
(println "b =" b)

(defn jacobi-iteration [x]
    (jo/matrix-div (jo/matrix-sub b (jo/matrix-mul LU x)) D-mat))

; Take the 50th iteration as the solution
(println "x =" (take 1 (drop 50 (iterate jacobi-iteration x))))



