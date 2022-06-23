; Solving for Ax=b via the jacobi method

; A is a diagonally dominant matrix
(def A (matrix 4 4 [10 -1  2  0,
                    -1 11 -1  3,
                     2 -1 10 -1,
                     0  3 -1  8]))
; initial value for x is all zeros
(def x (matrix 1 4 [0.0, 0.0, 0.0, 0.0]))
(def b (matrix 1 4 [6., 25., -11., 15.]))

(def D-vec (Math/diag A))
(def D-mat (matrix 1 4 D-vec))
(def LU (Math/matrix-sub A (Math/diag-matrix D-vec)))

(println "System:")
(println "A =" A)
(println "b =" b)

(defn jacobi-iteration [x]
    (Math/matrix-div (Math/matrix-sub b (Math/matrix-mul LU x)) D-mat))

; Take the 50th iteration as the solution
(println "x =" (take 1 (drop 50 (iterate jacobi-iteration x))))



