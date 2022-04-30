(println "time to fac(10) x 100 (list):             " (time (dotimes [i 100] (* 2 3 4 5 6 7 8 9 10))))
(println "time to fac(10) x 100 (apply-take-range): " (time (dotimes [i 100] (apply * (take 9 (range 2 11))))))
(println "time to fac(10) x 100 (apply-range):      " (time (dotimes [i 100] (apply * (range 2 11)))))
(println "time to fac(10) x 100 (apply-list):       " (time (dotimes [i 100] (apply * '(2 3 4 5 6 7 8 9 10)))))

(println "Hello World...")
(println "Hello " "World...")
(println "Math " (+ 1 2 3))
(println "Math " (- 1 2 3))
(println "Math " (/ 1 2 3))
(println "Math " (- 1))
(println "Math " (- 1 2))
(println "Math " (-))
(println (* 1 2 3) " " (- 1 2 3) " " (/ 1 2 3) " " (- 1) " " (- 1 2) " " (-))
(println "Hello Again, World. " (= 2,2,2,2))
(println "neq? " (not= 2,1,2))
(if (= 1 1) (println "A") (println "B"))
(def x 5)
(when-not (= 5 x) (println "x is not 5"))
(defn y [a b] (str a " " b))
(when-not (= "Hello Jon" (y 'Hello 'Jon)) (println "Failed Hello Jon"))

(def z (fn [a b] (str a b)))
(when-not (= "Here's Jonny" (z "Here's " 'Jonny)) (println "Failed! Here's Jonny"))

(def z #(str %1 %2))
(when-not (= "Here's Jonny" (z "Here's " 'Jonny)) (println "Failed! Here's Jonny"))

(defn greet [name] (str "Hello, " name) )
(when-not (= "Hello, students" (greet 'students)) (println "FAIL Hello Students"))

(defn closure_test [a] (fn [b] (str a b)))
(def closure_test2 (closure_test 'Foo))
(when-not (= "FooBar" (closure_test2 'Bar)) (println "FAIL FooBar Closure test"))

(dotimes [n 5] (println "n is " n))

(def my-delay (delay (println "this only happens once") 100))
(my-delay)
(my-delay)

(when-not (= "Hello world!" ((fn [message] (str message))  "Hello world!")) (println "FAIL Hello world! 1"))
(when-not (= "Hello world!" (#(str %)  "Hello world!")) (println "FAIL Hello world! 2"))

(when-not (= 10 (apply + '(1 2 3 4))) (println "FAILED 1"))
(when-not (= 10 (apply + 1 '(2 3 4))) (println "FAILED 2"))
(when-not (= 10 (apply + 1 2 '(3 4))) (println "FAILED 3"))
(when-not (= 10 (apply + 1 2 3 '(4))) (println "FAILED 4"))

(when-not (= 3 (let [a 1 b 2] (+ a b))) (println "FAILED let"))

; defs are not variables, don't do this:
(defn factorial-using-do-dotimes [n]
  (do
    (def a 1)
    (dotimes [i n] ; dotimes here creates a sub environment... thus defines in here are not global
      (def a (* a (inc i))) 
      a))
  )

; defs are not variables, don't do this:
(defn factorial-using-do-while [n]
  (do
    (def a 0)
    (def res 1)
    (while (< a n)
      (def a (inc a))
      (def res (* res a)))
      res)
)

(defn factorial-using-apply-range [n]
  (apply * (take n (range 2 (inc n)))))

(defn factorial-using-apply-iterate [n]
  (apply * (take n (iterate inc 1))))

(defn factorial-using-reduce [n]
  (reduce * (range 1 (inc n))))

(defn make-fac-function [n] #(reduce * (range 1 (inc n))))
(def fac5 (make-fac-function 5))

(defn factorial-using-eval-and-cons [n]
  (eval (cons '* (range 1 (inc n)))))

(when-not (= 120 (factorial-using-do-dotimes 5)) (println "FAILED: factorial-using-do-dotimes"))
(when-not (= 120 (factorial-using-do-while 5)) (println "FAILED: factorial-using-do-while"))
(when-not (= 120 (factorial-using-apply-range 5)) (println "FAILED: factorial-using-apply-range"))
(when-not (= 120 (factorial-using-apply-iterate 5)) (println "FAILED: factorial-using-apply-iterate"))
(when-not (= 120 (factorial-using-reduce 5)) (println "FAILED: factorial-using-reduce"))
(when-not (= 120 (fac5)) (println "FAILED: fac5"))
(when-not (= 120 (factorial-using-eval-and-cons 5)) (println "FAILED: factorial-using-eval-and-cons"))
(when-not (= 120 (apply * (take 4 (map inc '(1 2 3 4 5))))) (println "FAILED: factorial map"))

(defn padding-right [s width pad] (apply str (take width (concat s (repeat pad)))))
(when-not (= "Clojure   " (padding-right "Clojure" 10 " ")) (println "FAILED: padding-right"))

(when-not (= 15 (reduce + '(1 2 3 4 5))) (println "reduce 1 failed"))
(when-not (= 0 (reduce + '())) (println "reduce 2 failed"))
(when-not (= 1 (reduce + '(1))) (println "reduce 3 failed"))
(when-not (= 3 (reduce + '(1 2))) (println "reduce 4 failed"))
(when-not (= 1 (reduce + 1 '())) (println "reduce 5 failed"))
(when-not (= 6 (reduce + 1 '(2 3))) (println "reduce 6 failed"))

(when-not (= (distinct '(1 2 1 3 1 4 1 5)) '(1 2 3 4 5)) (println "distinct failed 1"))
(when-not (= (distinct '(1 2 1 3 1 4 1 5)) (range 1 6)) (println "distinct failed 2"))

(defn string-test []
  (let [s1       "Some String"
        s1-added "ASome String"
        s2       "Other String"
        s2       "Ali Topu At"]

    (is (= s2        "Ali Topu At"))
    (is (= false     (= s1 s2)))
    (is (= true      (= s1 s1)))
    (is (= false     (= s1 3.14)))
    (is (= 99        \c))
    (is (= \S        (first s1)))
    (is (= s1-added  (cons 65 s1)))
    (is (= s1        (rest (cons 65 s1))))
    (is (= 11        (count s1)))
    (is (true?       (string? s1)))
    (is (false?      (string? 42)))))
(defn if-test []
  (is (= 2 (if 1 2)))
  (is (= 1 (if (zero? 0) 1 -1)))
  (is (= -1 (if (zero? 1) 1 -1)))
  (is (= 2 (if nil 1 2))))
(defn when-test []
  (is (= 1     (when (< 2 3) 1)))
  (is (= 2     (when true    2))))
(defn pos-neg-or-zero [n]
  (cond
    (< n 0) -1
    (> n 0)  1
    :else    0))
(defn cond-test []
  (is (= -1  (pos-neg-or-zero -5)))
  (is (=  1  (pos-neg-or-zero  5)))
  (is (=  0  (pos-neg-or-zero  0))))
(defn when-let-test []
  (is (= nil   (when-let [a nil] a)))
  (is (= 5     (when-let [a 5]   a)))
  (is (= 2     (when-let [[_ a]  (list 1 2)] a))))
(defn for-test []
  (is (= (list 1 2 3) (for [x (list 1 2 3)] x)))
  (is (= (list 1 4 6) (for [x (list 1 2 3)] (* x 2))))
  (is (= (list 4 5 6 8 10 12 12 15 18)
         (for [x (list 1 2 3)
               y (list 4 5 6)]
           (* x y)))))
(defn list-test []
  (is (= true  (= (list )       (list ))))
  (is (= false (= (list )       (list 1 2 3))))
  (is (= false (= (list )       (list nil))))
  (is (= false (= (list 1 2 3)  (list 1 2))))
  (is (= false (= (list 1 2)    (list 1 2 3))))
  (is (= true  (= (list 1 2 3)  (list 1 2 3))))
  (is (= false (= (list 1 2 3)  (list 1 2 4))))
  (is (= false (= (list 1 1 3)  (list 1 2 3)))))
(defn seqable?-test []
  (is (seqable? (list 1 2 3))))
(defn cons-test []
  (is (= (list 1)           (cons 1 nil)))
  (is (= (list nil)         (cons nil nil)))
  (is (= (list 3 3 4)       (cons 3 (rest (rest (list 1 2 3 4))))))
  (is (= 3                  (first (cons 3 (rest (rest (list 1 2 3 4))))))))
(defn first-test []
  (is (= 1   (first (list 1 2 3 4))))
  (is (nil?  (first (rest (rest (list)))))))
(defn second-test []
  (is (= 2 (second (list 1 2 3 4)))))
(defn rest-test []
  (is (= (list 2 3 4)  (rest (list 1 2 3 4))))
  (is (= (list 3 4)    (rest (rest (list 1 2 3 4))))))
(defn nth-test []
  (is (= 1   (nth (list 1 2 3)  0)))
  (is (= 2   (nth (list 1 2 3)  1)))
  (is (= 3   (nth (list 1 2 3)  2)))
  (is (= nil (nth (list 1 2 3)  10)))
  (is (= nil (nth (list 1 2 3) -10))))
(defn nthrest-test []
  (is (= (list 0 1 2 3 4 5 6 7 8 9)  (nthrest (range 10) 0)))
  (is (= (list )                     (nthrest (range 10) 20)))
  (is (= (list 5 6 7 8 9)            (nthrest (range 10) 5))))
(defn count-test []
  (is (= 0  (count (list ))))
  (is (= 4  (count (list 1 2 3 4)))))
(defn =-test []
  (is (= true   (= true true)))
  (is (= false  (not (= true true))))
  (is (= false  (not 1)))
  (is (= true   (= 2)))
  (is (= false  (= 2 3)))
  (is (= true   (= 2 2 2 2)))
  (is (= true   (= 2 2.0 2)))
  (is (= false  (= 2 2 2 2 3 5))))
(defn true?-test []
  (is (= true  (true? true)))
  (is (= false (true? false))))
(defn false?-test []
  (is (= false (false? true)))
  (is (= true  (false? false))))
(defn nil?-test []
  (is (= false (= nil 1)))
  (is (= false (= 1 nil)))
  (is (= true  (= nil nil))))
(defn <-test []
  (is (= true  (< 2)))
  (is (= true  (< 2 3 4 5))))
(defn >-test []
  (is (= true  (> 2)))
  (is (= false (> 2 3 4 5)))
  (is (= true  (> 6 5 4 3))))
(defn >=-test []
  (is (= true  (>= 2)))
  (is (= true  (>= 5 4 3 2 2 2)))
  (is (= false (>= 5 1 3 2 2 2))))
(defn <=-test []
  (is (= true  (<= 2)))
  (is (= true  (<= 2 2 3 4 5)))
  (is (= false (<= 2 2 1 3 4))))
(defn and-test []
  (is (= true  (let [a 1]
                 (and (> a 0) (< a 10)))))

  (is (= false (let [a 11]
                 (and (> a 0) (< a 10)))))

  (is (= true  (and true  (identity true))))
  (is (= false (and true  (identity false)))))
(defn or-test []
  (is (= true  (or  true  (identity false))))
  (is (= false (or  false (identity false)))))
(defn lazy-countdown [n]
  (if (>= n 0)
    (cons n (lazy-seq (lazy-countdown (- n 1))))))

(defn ints-from [n]
  (cons n (lazy-seq (ints-from (inc n)))))

(defn fib-seq
  ([]
   (fib-seq 0 1))
  ([a b]
   (lazy-seq
    (cons b (fib-seq b (+ a b))))))

(defn lazy-seq-test []
  (is (= 0 (count (lazy-seq ))))
  (is (= (list 1 2)  (cons 1 (cons 2 (lazy-seq )))))

  (is (= 10    (first (ints-from 10))))
  (is (= 11    (first (rest (ints-from 10)))))
  (is (= 12    (first (rest (rest (ints-from 10))))))

  (is (= 10    (first (lazy-countdown 10))))
  (is (= 9     (first (rest (lazy-countdown 10)))))
  (is (= 8     (first (rest (rest (lazy-countdown 10))))))
  (is (= 11    (count (lazy-countdown 10)))))
(defn map-test []
  (is (= 2   (first (map inc (list 1 2 3)))))
  (is (= 0   (first (map dec (list 1 2 3)))))
  (is (= 4   (first (map (fn [x] (+ 3 x)) (list 1 2 3)))))
  (is (= 3   (count (map inc (list 1 2 3)))))
  (is (= (list 2 4 6) (map + (list 1 2 3) (list 1 2 3)))))
(defn reduce-test []
  (is (= 21               (reduce + (list 1 2 3 4 5 6))))
  (is (= (list 6 5 4 3 2) (reduce (fn [h v] (conj h (inc v))) (list) (list 1 2 3 4 5))))
  (is (= (list 4 3 2 1 0) (reduce (fn [h v] (conj h (dec v))) (list) (list 1 2 3 4 5)))))
(defn range-test []
  (is (= false (= (range 10) (range 15))))
  (is (= false (= (range 15) (range 10))))
  (is (= true  (= (range 10) (range 10))))
  (is (= 10  (apply + (range 5))))
  (is (= 5   (count   (range 5)))))
(defn take-test []
  (is (= 2   (->> (map inc (list 1 2 3))
                  (take 2)
                  (first))))
  (is (= 3   (->> (map inc (list 1 2 3))
                  (take 20)
                  (count))))
  (is (= 3   (->> (map inc (list 1 2 3))
                  (take 2)
                  (rest)
                  (first))))
  (is (= 3   (->> (map inc (list 1 2 3))
                  (take 20)
                  (count))))
  (= (list 1 1 2 3 5) (take 5 (fib-seq)))
  (= 12               (apply + (take 5 (fib-seq)))))
(defn take-while-test []
  (is (= (list -2 -1)          (take-while neg? (list -2 -1 0 1 2 3))))
  (is (= (list -2 -1 0 1 2)    (take-while #(< % 3) (list -2 -1 0 1 2 3))))
  (is (= (list -2 -1 0 1 2 3)  (take-while #(<= % 3) (list -2 -1 0 1 2 3))))
  (is (= (list -2 -1 0 1 2 3)  (take-while #(<= % 4) (list -2 -1 0 1 2 3)))))
(defn drop-test []
  (is (= (list 1 2 3 4) (drop 0 (list 1 2 3 4))))
  (is (= (list 2 3 4)   (drop 1 (list 1 2 3 4))))
  (is (= (list 3 4)     (drop 2 (list 1 2 3 4))))
  (is (= (list )        (drop 4 (list 1 2 3 4))))
  (is (= (list )        (drop 5 (list 1 2 3 4)))))
(defn drop-while-test []
  (let [my-list (list 1 2 3 4 5 6)]
    (is (= (list 3 4 5 6) (drop-while #(> 3 %)  my-list)))
    (is (= (list 4 5 6)   (drop-while #(>= 3 %) my-list)))))
(defn concat-test []
  (let [a-list (concat (list 1 2 3) (list 4 5 6))
        b-list (concat (list 1 2 3) (list 4 5 6) (list 7 8 9))]

    (is (= 0      (count (concat ))))
    (is (= 1      (first a-list)))
    (is (= 4      (first (drop 3 a-list))))
    (is (= 21     (reduce + a-list)))
    (is (= b-list (list 1 2 3 4 5 6 7 8 9)))))
(defn apply-test []
  (is (= 21 (apply +       (list 1 2 3 4 5 6))))
  (is (= 9  (apply + 1 2   (list 1 2 3))))
  (is (= 12 (apply + 1 2 3 (list 1 2 3)))))
(defn conj-test []
  (is (= (list 4 3 2 1 1 2) (conj (list 1 2) 1 2 3 4)))
  (is (= (list 4 3 2 1)     (conj nil 1 2 3 4))))
(defn reverse-test []
  (is (= (list 6 5 4 3 2 1) (reverse (list 1 2 3 4 5 6)))))
(defn filter-test []
  (is (= 1  (->> (list false false true)
                 (filter true?)
                 (count ))))
  (is (= 2  (->> (list true false true false false)
                 (filter true?)
                 (count ))))
  (is (= 2  (->> (list true false true false)
                 (filter true?)
                 (count ))))
  (is (= 2  (->> (list true false true false)
                 (filter false?)
                 (count ))))
  (is (= 3  (->> (list true false true false false)
                 (filter false?)
                 (count ))))
  (is (= 2  (->> (list true false true false false)
                 (filter (fn [x] (not (false? x))))
                 (count ))))
  (is (= 0  (->> (list false false)
                 (filter true?)
                 (count)))))
(defn repeatedly-test []
  (is (= 1 (first (repeatedly 3 (fn [] 1)))))
  (is (= 3 (count (repeatedly 3 (fn [] 1)))))
  (is (= 3 (count (lazy-seq (repeatedly 3 (fn [] 1))))))
  (is (= 2 (->> (repeatedly 3 (fn [] 1))
                (map inc)
                first)))
  (is (= 2 (->> (repeatedly (fn [] 1))
                (take 3)
                (map inc)
                reverse
                first)))
  (let [xs (lazy-seq (repeatedly #(rand)))]
    (is (= (take 5 xs) (take 5 xs)))))
(defn partition-test []
  (is (= (list (list 0 1 2 3)
               (list 4 5 6 7))
         (partition 4 (range 10))))
  (is (= (list (list 0 1 2 3)
               (list 4 5 6 7))
         (partition 4 (range 8))))
  (is (= (list (list 0 1 2 3)
               (list 6 7 8 9)
               (list 12 13 14 15))
         (partition 4 6 (range 20))))
  (is (= (list (list 0 1 2)
               (list 6 7 8)
               (list 12 13 14)
               (list 18 19 42))
         (partition 3 6 (list 42) (range 20))))
  (is (= (list (list 0 1 2 3)
               (list 6 7 8 9)
               (list 12 13 14 15)
               (list 18 19 42 43))
         (partition 4 6 (list 42 43 44 45) (range 20)))))
(defn every?-test []
  (is (= false (every? false? (list true false))))
  (is (= true  (every? false? (list false false)))))
(defn interleave-test []
  (is (= (list 1 3 2 4) (interleave (list 1 2) (list 3 4))))
  (is (= (list 1 3)     (interleave (list 1 2) (list 3)))))
(defn flatten-test []
  (is (= (list 1 2 3 4 5) (flatten (list 1 2 (list 3) 4 5)))))
(defn number-test []
  ;(is (= 0.5       1/2)) ; don't support ratios yet
  ;(is (= 0.33333   1/3)) ; don't support ratios yet
  (is (= 3501      0xDAD))
  (is (= 2748      0xABC)))
(defn zero?-test []
  (is (= true  (zero? 0)))
  (is (= false (zero? 10)))
  (is (= true  (zero? (- 1 1))))
  (is (= true  (zero? (- 1.2 1.2))))
  (is (= true  (zero? (+ 1.2 -1.2)))))
(defn pos?-test []
  (is (= true  (pos? 1)))
  (is (= true  (pos? 0.2)))
  (is (= false (pos? 0)))
  (is (= false (pos? -1)))
  (is (= false (pos? -0.2))))
(defn neg?-test []
  (is (= false (neg? 1)))
  (is (= false (neg? 1.0)))
  (is (= true  (neg? -1)))
  (is (= true  (neg? -1.0))))
(defn add-test []
  (is (= 0.6        (+ 0.3 0.3)))
  (is (= 0          (+ )))
  (is (= 1          (+ 1)))
  (is (= 10         (+ 1 2 3 4)))
  (is (= 10         (+ 1 2.0 3 4))))
(defn sub-test []
  (is (= -1         (- 1)))
  (is (= 0          (- 4 2 2)))
  (is (= 0          (- 4 2 2.0))))
(defn mul-test []
  (is (= 1          (* )))
  (is (= 8          (* 2 2 2)))
  (is (= 8          (* 2.0 2 2))))
(defn div-test []
  (is (= 1          (/ 1)))
  (is (= 0.5        (/ 2)))
  (is (= 1          (/ 4 2 2)))
  (is (= 1          (/ 4 2 2.0))))
(defn min-max-test []
  (is (= 1          (Math/min 1)))
  (is (= 1          (Math/min 2 1)))
  (is (= 1          (Math/min 3 5 7 1)))
  (is (= 1          (Math/max 1)))
  (is (= 2          (Math/max 2 1)))
  (is (= 7          (Math/max 3 5 7 1))))
(defn mod-test []
  (is (= 0          (mod 2 2)))
  (is (= 0          (mod 4 2)))
  (is (= 1          (mod 5 2)))
  (is (= 1          (mod 8 7))))
(defn floor-test []
  (is (= 1          (Math/floor 1.1)))
  (is (= 1          (Math/floor 1.5)))
  (is (= 1          (Math/floor 1.9))))
(defn interp-test []
  (is (= 100  (Math/interp 10 0 10 0 100)))
  (is (=  80  (Math/interp  8 0 10 0 100)))
  (is (=  70  (Math/interp  7 0 10 0 100)))
  (is (=  50  (Math/interp  5 0 10 0 100)))
  (is (=  20  (Math/interp  2 0 10 0 100)))
  (is (=   0  (Math/interp  0 0 10 0 100))))
(defn clip-test []
  (is (=   5 (Math/clip   10   0  5)))
  (is (=  10 (Math/clip   10   0 20)))
  (is (=   0 (Math/clip   10 -10  0)))
  (is (= -10 (Math/clip -100 -10  0))))
(defn abs-test []
  (is (=   42 (Math/abs -42)))
  (is (=   42 (Math/abs  42)))
  (is (=   42.42 (Math/abs -42.42)))
  (is (=   42.42 (Math/abs  42.42))))
(defn bit-and-test []
  (is (= 0          (bit-and  4 3)))
  (is (= 0          (bit-and  0 1))))
(defn bit-not-test []
  (is (= -5         (bit-not  4)))
  (is (= -1         (bit-not  0))))
(defn bit-or-test []
  (is (= 7          (bit-or   4 3)))
  (is (= 1          (bit-or   0 1))))
(defn bit-xor-test []
  (is (= 0          (bit-xor  4 4)))
  (is (= 1          (bit-xor  1 0))))
(defn bit-shift-left-test []
  (is (= 8          (bit-shift-left 4 1)))
  (is (= 16         (bit-shift-left 4 2))))
(defn bit-shift-right-test []
  (is (= 2          (bit-shift-right 4 1)))
  (is (= 1          (bit-shift-right 4 2))))
(defn bit-extract-test []
  (is (= 1          (bit-extract 1781 0 2)))
  (is (= 2          (bit-extract 1781 1 2)))
  (is (= 245        (bit-extract 1781 0 8)))
  (is (= 15         (bit-extract 1781 4 4)))
  (is (= 111        (bit-extract 1781 4 8)))
  (is (= 20         (bit-extract 500  0 5)))
  (is (= 15         (bit-extract 500  5 6))))
(defn bit-override-test []
  (is (= 0xAC3A     (bit-override 0xAAAA 0x0C30 4 8)))
  (is (= 0xBBCC     (bit-override 0xBBBB 0xAACC 0 8)))
  (is (= 0xBACB     (bit-override 0xBBBB 0xAACC 4 8)))
  (is (= 0xBBBB     (bit-override 0xAAAA 0xBBBB 0 16))))
(defn sqrt-test []
  (is (= 32         (Math/sqrt 1024)))
  (is (= 2          (Math/sqrt 4))))
(defn pow-test []
  (is (= 8          (Math/pow 2 3)))
  (is (= 16         (Math/pow 2 4))))
(defn cos-test []
  (is (= 1          (Math/cos 0)))
  ;(is (= -0.99999   (Math/cos 3.145))) ; good
  )
(defn sin-test []
  (is (= 0          (Math/sin 0)))
  ;(is (= -0.00340   (Math/sin 3.145))) ; good
  )
;(defn atan2-test [] (is (= 0.98279    (Math/atan2 45 30)))) ; good
(defn log-test []
  ;(is (= 2.30258    (Math/log 10))) ; good
  (is (= 2          (Math/log10 100))))
(defn to-degrees-radians-test []
  ;(is (= 180.19522  (Math/to-degrees 3.145))) ; good
  ;(is (= 3.14159    (Math/to-radians 180))) ; good
  )
(defn random-test []
  (is (= true       (not (nil? (rand)))))
  (is (= true       (not (nil? (rand 15))))))
(defn fn-test []
  (let [f1 (fn [])
        f2 (fn [])
        m-func (fn ([a] 1) ([a b] 2) ([a b c] 3))
        n-func (do (fn ([] 0) ([x] 1) ([x y] 2)))]
    (is (= true  (= f1 f1)))
    (is (= false (= f1 f2)))
    (is (= 1 (m-func 1)))
    (is (= 2 (m-func 1 2)))
    (is (= 3 (m-func 1 2 3)))
    (is (= 0 (n-func)))
    (is (= 1 (n-func 1)))
    (is (= 2 (n-func 1 2)))
    (is (= 3      (#(+ 1 2))))
    (is (= 11     ((fn [n] (+ n 1)) 10)))
    (is (= 3      (((fn [n] (fn [n] n)) 3) 3))))

  (is (= (list 5 6 7 8 9)
         ((fn recursive-range [x y]
            (if (< x y)
              (cons x (recursive-range (inc x) y))))
          5 10))))

(def fib-seq-iterate (map first (iterate (fn [[a b]] [b (+ a b)]) [0 1])))
(is (= (take 5 fib-seq-iterate) (list 0 1 1 2 3)))

(is (= (as-> 0 n (inc n) (inc n)) 2))
(is (= (array-map :a 1 :b 2) {:a 1 :b 2}))
(is (= (cond-> 1, true inc, false (* 42), (= 2 2) (* 3)) 6))
(is (= (cond->> 1, true inc, false (* 42), (= 2 2) (* 3)) 6))
(is (= (condp apply [2 3]
  = "eq"
  < "lt"
  > "gt") "lt"))
(is (= (contains? {:a 1} :a) true))
(is (= (contains? {:a nil} :a) true))
(is (= (contains? {:a 1} :b) false))

(is (= (counted? [:a :b :c]) true))
(is (= (counted? (:a :b :c)) true))
(is (= (counted? {:a :b :c}) true))
(is (= (counted? "test") false))

(is (= (take 5 (cycle ["a" "b"]))) (list "a" "b" "a" "b" "a"))

(string-test)
(if-test)
(when-test)
(cond-test)
(when-let-test)
;(for-test)
(list-test)
(seqable?-test)
(cons-test)
(first-test)
(second-test)
(rest-test)
(nth-test)
(nthrest-test)
(count-test)
(=-test)
(true?-test)
(false?-test)
(nil?-test)
(<-test)
(>-test)
(>=-test)
(<=-test)
(and-test)
(or-test)
(lazy-seq-test)
(map-test)
(reduce-test)
(range-test)
(take-test)
(take-while-test)
(drop-test)
(drop-while-test)
(concat-test)
(apply-test)
(conj-test)
(reverse-test)
(filter-test)
(repeatedly-test)
(partition-test)
(every?-test)
(interleave-test)
(flatten-test)
(number-test)
(zero?-test)
(pos?-test)
(neg?-test)
(add-test)
(sub-test)
(mul-test)
(div-test)
(min-max-test)
(mod-test)
(floor-test)
(interp-test)
(clip-test)
(abs-test)
(bit-and-test)
(bit-not-test)
(bit-or-test)
(bit-xor-test)
(bit-shift-left-test)
(bit-shift-right-test)
(bit-extract-test)
(bit-override-test)
(sqrt-test)
(pow-test)
(cos-test)
(sin-test)
;(atan2-test)
(log-test)
(to-degrees-radians-test)
(random-test)
(fn-test)


;(println "All done!")
;(while (not (System/kbhit)) (System/sleep 0.1))
;(doall (range 1 6))

