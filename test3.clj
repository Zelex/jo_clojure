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

;(def m-func (fn ([a] 1) ([a b] 2) ([a b c] 3)))
;(defn m-func [] 1)

;(println (m-func 0))
;(println (m-func 0 1))
;(println (m-func 0 1 2))

;(defn fn-test [] 
  ;(let [
      ;f1 (fn [])
      ;f2 (fn [])
      ;m-func (fn ([a] 1) ([a b] 2) ([a b c] 3))
      ;n-func (fn ([] 0) ([x] 1) ([x y] 2))]
  ;(println (m-func 1))
  ;(println (m-func 1 2))
  ;(println (m-func 1 2 3))
  ;(println (n-func))
  ;(println (n-func 1))
  ;(println (n-func 1 2))
  ;))
;(fn-test)

;(defn ints-from [n] (cons n (lazy-seq (ints-from (inc n)))))
;(doall (take 10 (ints-from 10)))
;(doall (lazy-seq (cons 1 (lazy-seq (range 3)))))
;(doall (lazy-seq (cons 1 (lazy-seq '(2 3)))))

;((fn recursive-range [x y] (if (< x y) (cons x (recursive-range (inc x) y)))) 5 10)

;(def fib-seq-iterate (map first (iterate (fn [[a b]] [b (+ a b)]) [0 1])))
;(doall (take 10 fib-seq-iterate))

;(defmacro unless [pred a b] `(if (not ~pred) ~a ~b))
;(unless false (println "Will print") (println "Will not print"))

;(split-at 1 [1 2 3 4 5])

;(split-with (partial > 3) [1 2 3 2 1])

;#{:a :b :c :d}

;(into [] [:c :b :a])

;#{:a :b :c :b}
;(into [] #{:a :b :a :c})

;(into #{} [:a :b :a :c])
;(seq [:c :b :a])
;(disj #{1 2 3})

;(when-let [[_ a]  (list 1 2)]

;(def my-atom (atom []))
;(swap! my-atom conj "test1\n")
;(swap! my-atom conj "test2\n")
;(swap! my-atom conj "test3\n")
;(doall (map print @my-atom))

;(def x 5)
;(for [x [0 1 2 3 4 5] :let [y (* x 3)] :when (even? y)] y)

;(def digits [1 2 3])
;(for [x1 digits x2 digits] (* x1 x2)) 

(for [[x y] '([:a 1] [:b 2] [:c 0]) :when (= y 0)] x)

