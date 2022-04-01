;(doall '(0 1 2 3))
;(doall (range 4))
;(doall (filter even? (range 10)))

;(filter #(= (count %) 1) '("a" "aa" "b" "n" "f" "lisp" "clojure" "q" ""))

;(filter 
  ;(fn [[k v]] (even? k))
  ;{1 "a", 2 "b", 3 "c", 4 "d"})

; remove empty vectors from the root vector
;(def vector-of-vectors ((1 2 3) () (1) ()))
;(def populated-vector? #(not= % ()))
;(filter populated-vector? vector-of-vectors)

;((comp str +) 8 8 8)

;(filter (comp not zero?) [0 1 0 2 0 3 0 4])

;((partial + 100) 5)
;((partial - 100) 5)

;(filter (comp #{2 3} last) {:x 1 :y 2 :z 3}) ; don't support sets yet

;(filter some? '(1 nil [] :a nil))

;(def add-domain (partial str "@clojure.com"))
;(str "info" (add-domain) )

;(#(+ 6 %) 1)
;(#(+ %1 %2) 2 3)

;(filter letter? "hello, world!")
;(filter (partial letter?) "hello, world!")
;(filter #(letter? %) "hello, world!")

;(doall (map + [1 2 3] [4 5 6]))
;(doall (map + [1 2 3] (iterate inc 1)))
;(doall (map + [1 2 3] (range)))
;(doall (range 10))

;(into () '(1 2 3))
;(into {:x 4} '({:a 1} {:b 2} {:c 3}))
;(take-nth 2 (range 10))
;(doall (take 3 (take-nth 0 (range 2))))
;(doall (take 3 (take-nth -10 (range 2))))

;(def food [:ice-cream :steak :apple])
;(rand-nth food)

;(def boring (constantly 10))
;(boring 1 2 3)
;(boring)
;(boring "Is anybody home?")

;(reduce + (map (constantly 1) [:a :b :c]))
;(reduce + (map (constantly 1) (range 3)))

;(rand-nth [])

;(shuffle (1 2 3 4))

;(random-sample 0.5 [1 2 3 4 5])

;(doall (keep even? (range 1 10)))

;(doall 
;(keep #(if (odd? %) %) (range 10))
;;(map #(if (odd? %) %) (range 10))
;(for [ x (range 10) :when (odd? x)] x)
;(filter odd? (range 10))
;)

(defn list-test []
  (is (= true  (= (list )       (list ))))
  (is (= false (= (list )       (list 1 2 3))))
  (is (= false (= (list )       (list nil))))
  (is (= false (= (list 1 2 3)  (list 1 2))))
  (is (= false (= (list 1 2)    (list 1 2 3))))
  (is (= true  (= (list 1 2 3)  (list 1 2 3))))
  (is (= false (= (list 1 2 3)  (list 1 2 4))))
  (is (= false (= (list 1 1 3)  (list 1 2 3)))))
(list-test)
