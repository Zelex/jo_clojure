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
(def vector-of-vectors (quote (1 2 3) () (1) ()))
(def populated-vector? (fn [item] (not= item '())))
(filter populated-vector? vector-of-vectors)
