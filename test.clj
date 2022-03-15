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
(println "neq? " (!= 2,1,2))
(if (= 1 1) (println "A") (println "B"))
(println x)
(def x 5)
(println x)
(defn y [a b] (println a " " b) 2)
(y 'Hello 'Jon)

(def z (fn [a b] (println a b) 3))
(z "Here's " 'Johny)

(defn greet [name] (str "Hello, " name) )
(println (greet 'students))

(defn closure_test [a] (fn [b] (println a b)))
(def closure_test2 (closure_test 'Foo))
(closure_test2 'Bar)

(defn pow [x n] (apply * (repeat n x)))
(defn sq [y] (pow y 2))
(defn qb [y] (pow y 3))

(println (sq 2))
(println (qb 2))
(println (pow 2,4))

(dotimes (n 5) (println "n is " n))

(def my-delay (delay (println "this only happens once") 100))
(println (my-delay))
(println (my-delay))

((fn (message) (println message))  "Hello world!")

(unless (= 10 (apply + '(1 2 3 4))) (println "FAILED 1"))
(unless (= 10 (apply + 1 '(2 3 4))) (println "FAILED 2"))
(unless (= 10 (apply + 1 2 '(3 4))) (println "FAILED 3"))
(unless (= 10 (apply + 1 2 3 '(4))) (println "FAILED 4"))

(println (let (a 1 b 2) (+ a b)))

; defs are not variables, don't do this:
(defn factorial-using-do-dotimes [n]
  (do
    (def a 1)
    (dotimes (i n) ; dotimes here creates a sub environment... thus defines in here are not global
      (def a (* a (inc i))) a))
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

(defn make-fac-function [n]
  (fn () (reduce * (range 1 (inc n)))))
(def fac5 (make-fac-function 5))

(defn factorial-using-eval-and-cons [n]
  (eval (cons '* (range 1 (inc n)))))

(unless (= 120 (factorial-using-do-dotimes 5)) (println "FAILED: factorial-using-do-dotimes"))
(unless (= 120 (factorial-using-do-while 5)) (println "FAILED: factorial-using-do-while"))
(unless (= 120 (factorial-using-apply-range 5)) (println "FAILED: factorial-using-apply-range"))
(unless (= 120 (factorial-using-apply-iterate 5)) (println "FAILED: factorial-using-apply-iterate"))
(unless (= 120 (factorial-using-reduce 5)) (println "FAILED: factorial-using-reduce"))
(unless (= 120 (fac5)) (println "FAILED: fac5"))
(unless (= 120 (factorial-using-eval-and-cons 5)) (println "FAILED: factorial-using-eval-and-cons"))
(unless (= 120 (apply * (take 4 (map inc '(1 2 3 4 5))))) (println "FAILED: factorial map"))

(defn padding-right [s width pad] (apply str (take width (concat s (repeat pad)))))
(unless (= "Clojure   " (padding-right "Clojure" 10 " ")) (println "FAILED: padding-right"))

(unless (= 15 (reduce + '(1 2 3 4 5))) (println "reduce 1 failed"))
(unless (= 0 (reduce + '())) (println "reduce 2 failed"))
(unless (= 1 (reduce + '(1))) (println "reduce 3 failed"))
(unless (= 3 (reduce + '(1 2))) (println "reduce 4 failed"))
(unless (= 1 (reduce + 1 '())) (println "reduce 5 failed"))
(unless (= 6 (reduce + 1 '(2 3))) (println "reduce 6 failed"))

(unless (= (distinct '(1 2 1 3 1 4 1 5)) '(1 2 3 4 5)) (println "distinct failed 1"))
(unless (= (distinct '(1 2 1 3 1 4 1 5)) (range 1 6)) (println "distinct failed 2"))

(doall (map println (range 1 4)))

(println "time to fac(10) x 100 (list): " (time (dotimes [i 100] (* 2 3 4 5 6 7 8 9 10))))
(println "time to fac(10) x 100 (range): " (time (dotimes [i 100] (apply * (take 9 (range 2 11))))))

(println "end")


;(while (not (System/kbhit)) (System/sleep 0.1))
;(doall (range 1 6))

