;(def num-elems 1090)
(def data-file 'poly_solve_2.csv)
(def num-elems 4096)
(def num-coefs 8)

; Fill A with poly elems
; i^5 i^4 i^3 i^2 i^1 1 where i is == row / 1089.0
(def A (atom (matrix num-coefs num-elems)))
(doall (for [i (range 0 num-elems) 
        :let [ii (/ i (- num-elems 1.0))
              row [(Math/pow ii 7) (Math/pow ii 6) (Math/pow ii 5) (Math/pow ii 4) (Math/pow ii 3) (Math/pow ii 2) ii 1]]]
            (swap! A matrix/set-row i row)))

; Fill b with colums of values from poly solve data file
(def b (atom (matrix 1 num-elems)))
(def rows (map float (split-lines (slurp data-file))))
(swap! b matrix/set-col 0 rows)

; Invert A
(def inv-A (matrix/pinv @A))

; multiply by 'b' to get 'x'
(def x (matrix/mul inv-A @b))

; output results
(println x)



