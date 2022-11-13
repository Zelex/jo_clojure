(def num-coefs 6)

; Fill A with poly elems
; i^5 i^4 i^3 i^2 i^1 1 where i is == row / 1089.0
(def A (atom (matrix num-coefs 1090)))
(doall (for [i (range 0 1090) 
        :let [ii (/ i 1089.0)
              row [(Math/pow ii 5) (Math/pow ii 4) (Math/pow ii 3) (Math/pow ii 2) ii 1]]]
            (swap! A matrix/set-row i row)))

; Fill b with colums of values from poly solve data file
(def b (atom (matrix 1 1090)))
(def rows (map float (split-lines (slurp 'poly_solve.csv))))
(swap! b matrix/set-col 0 rows)

; Invert A
(def inv-A (matrix/pinv @A))

; multiply by 'b' to get 'x'
(def x (matrix/mul inv-A @b))

; output results
(println x)



