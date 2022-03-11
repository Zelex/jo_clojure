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
(defn y [a b] (println a b) 2)
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

(apply println '(1 2 3 4))    ;; same as  (f 1 2 3 4)
(apply println 1 '(2 3 4))    ;; same as  (f 1 2 3 4)
(apply println 1 2 '(3 4))    ;; same as  (f 1 2 3 4)
(apply println 1 2 3 '(4))    ;; same as  (f 1 2 3 4)

(println (let (a 1 b 2) (+ a b)))

(println "factorials")

; defs are not variables, don't do this:
(defn factorial-using-do-dotimes [n]
  (do
    (def a 1)
    (dotimes (i n)
      (def a (* a (inc i)))))
  a)

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

(println "do-times       " (factorial-using-do-dotimes 5))
(println "do-while       " (factorial-using-do-while 5))
(println "apply-range    " (factorial-using-apply-range 5))
(println "apply-iterate  " (factorial-using-apply-iterate 5))
(println "reduce         " (factorial-using-reduce 5))
(println "make           " (fac5))
(println "eval           " (factorial-using-eval-and-cons 5))
(println "map            " (apply * (take 4 (map inc '(1 2 3 4 5)))))

(defn padding-right [s width pad] 
  (apply str (take width (concat s (repeat pad)))))

(println "'" (padding-right "Clojure" 10 " ") "'")

(println (reduce + '(1 2 3 4 5)))  ;;=> 15
(println (reduce + '()))           ;;=> 0
(println (reduce + '(1)))          ;;=> 1
(println (reduce + '(1 2)))        ;;=> 3
(println (reduce + 1 '()))         ;;=> 1
(println (reduce + 1 '(2 3)))      ;;=> 6

(unless (= (distinct '(1 2 1 3 1 4 1 5)) '(1 2 3 4 5)) (println "distinct failed 1"))
(unless (= (distinct '(1 2 1 3 1 4 1 5)) (range 1 6)) (println "distinct failed 2"))

(doall (map println (range 1 4)))

(println "end")


;(while (not (System/kbhit)) (System/sleep 0.1))
;(doall (range 1 6))

