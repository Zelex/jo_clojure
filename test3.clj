(doall '(0 1 2 3))
(doall (range 4))
(doall (filter even? (range 10)))

(doall (filter 
  (fn [x] (= (count x) 1))
  '("a" "aa" "b" "n" "f" "lisp" "clojure" "q" "")))

(doall (filter 
  (fn [[k v]] (even? k))
  {1 "a", 2 "b", 3 "c", 4 "d"}
))