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

(defn pow [x n] (apply * (repeat n x)))
(defn sq [y] (pow y 2))
(defn qb [y] (pow y 3))

(when-not (= 4 (sq 2)) (println "FAIL sq"))
(when-not (= 8 (qb 2)) (println "FAIL qb"))
(when-not (= 16 (pow 2,4)) (println "FAIL pow"))

(dotimes (n 5) (println "n is " n))

(def my-delay (delay (println "this only happens once") 100))
(my-delay)
(my-delay)

(when-not (= "Hello world!" ((fn (message) (str message))  "Hello world!")) (println "FAIL Hello world!"))
(when-not (= "Hello world!" (#(str %)  "Hello world!")) (println "FAIL Hello world!"))

(when-not (= 10 (apply + '(1 2 3 4))) (println "FAILED 1"))
(when-not (= 10 (apply + 1 '(2 3 4))) (println "FAILED 2"))
(when-not (= 10 (apply + 1 2 '(3 4))) (println "FAILED 3"))
(when-not (= 10 (apply + 1 2 3 '(4))) (println "FAILED 4"))

(when-not (= 3 (let (a 1 b 2) (+ a b))) (println "FAILED let"))

; defs are not variables, don't do this:
(defn factorial-using-do-dotimes [n]
  (do
    (def a 1)
    (dotimes (i n) ; dotimes here creates a sub environment... thus defines in here are not global
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

;(doall (map println (range 1 4)))

(println "time to fac(10) x 100 (list):             " (time (dotimes [i 100] (* 2 3 4 5 6 7 8 9 10))))
(println "time to fac(10) x 100 (apply-take-range): " (time (dotimes [i 100] (apply * (take 9 (range 2 11))))))
(println "time to fac(10) x 100 (apply-range):      " (time (dotimes [i 100] (apply * (range 2 11)))))
(println "time to fac(10) x 100 (apply-list):       " (time (dotimes [i 100] (apply * '(2 3 4 5 6 7 8 9 10)))))

;(println "end")
;(while (not (System/kbhit)) (System/sleep 0.1))
;(doall (range 1 6))

