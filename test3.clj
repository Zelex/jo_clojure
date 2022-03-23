;(doall '(0 1 2 3))
;(doall (range 4))
;(doall (filter even? (range 10)))

;(filter 
  ;(fn [x] (= (count x) 1))
  ;'("a" "aa" "b" "n" "f" "lisp" "clojure" "q" ""))

;(filter 
  ;(fn [[k v]] (even? k))
  ;{1 "a", 2 "b", 3 "c", 4 "d"})

; remove empty vectors from the root vector
;(def vector-of-vectors ((1 2 3) () (1) ()))
;(def populated-vector? (fn [item] (not= item ())))
;(filter populated-vector? vector-of-vectors)

;((comp str +) 8 8 8)

;(filter (comp not zero?) [0 1 0 2 0 3 0 4])

;((partial + 100) 5)
;((partial - 100) 5)

;(filter (comp #{2 3} last) {:x 1 :y 2 :z 3}) ; don't support sets yet

(filter some? '(1 nil [] :a nil))
