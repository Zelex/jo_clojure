(def data-file 'poly_solve_1.csv)
(def rows (map float (split-lines (slurp data-file))))
(def num-elems (count rows))
(def num-coefs 8)

; Fill A with poly elems
(def A (atom (matrix num-coefs num-elems)))
(doall (for [i (range 0 num-elems) 
        :let [ii (/ i (- num-elems 1.0))
              row [(Math/pow ii 9) (Math/pow ii 8) (Math/pow ii 7) (Math/pow ii 6) (Math/pow ii 5) (Math/pow ii 4) (Math/pow ii 3) (Math/pow ii 2) ii 0]
              ]]
            (swap! A matrix/set-row i (drop (- 10 num-coefs) row))))

; Fill b with colums of values from poly solve data file
(def b (atom (matrix 1 num-elems)))
(swap! b matrix/set-col 0 rows)

; Invert A
(def inv-A (matrix/pinv @A))

; multiply by 'b' to get 'x'
(def x (matrix/mul inv-A @b))

; output results
(println x)



