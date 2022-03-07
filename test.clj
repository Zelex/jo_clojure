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
(defn y (a b) (println a b) 2)
(y 'Hello 'Jon)
(def z (fn (a b) (println a b) 3))
(z "Here's " 'Johny)
(defn greet  (name)  (str "Hello, " name) )
(println (greet 'students))
(defn closure_test (a) (fn (b) (println a b)))
(def closure_test2 (closure_test 'Foo))
(closure_test2 'Bar)

;(defn pow (x n) (apply * (repeat x n)))
;(defn sq (y) (pow y 2))
;(defn qb (y) (pow y 3))

;(println (sq 2))
;(println (qb 2))
;(println (pow 2,4))

; (repeat 10 println)

(dotimes (n 5) (println "n is " n))

(def my-delay (delay (println "this only happens once") 100))
(println (my-delay))
(println (my-delay))

((fn (message) (println message))  "Hello world!")

; (apply println (quote 1 2 3 4))    ;; same as  (f 1 2 3 4)
; (apply println 1 (quote 2 3 4))    ;; same as  (f 1 2 3 4)
; (apply println 1 2 (quote 3 4))    ;; same as  (f 1 2 3 4)
; (apply println 1 2 3 (quote 4))    ;; same as  (f 1 2 3 4)

(println (let ((a 1) (b 2)) (+ a b)))


; defs are not variables, don't do this:
(defn factorial-using-do-dotimes (n)
  (do
    (def a 1)
    (dotimes (i n)
      (def a (* a (inc i)))))
  a)

; defs are not variables, don't do this:
(defn factorial-using-do-while (n)
  (do
    (def a 0)
    (def res 1)
    (while (< a n)
      (def a (inc a))
      (def res (* res a)))
      res)
)

(defn factorial-using-apply-range (n)
  (apply * (take n (range 1 (inc n)))))

(println a res)

(println (factorial-using-do-dotimes 5))
(println (factorial-using-do-while 5))
(println (factorial-using-apply-range 5))
(println (apply * (take 5 (range 1 6))))
(println (* 1 2 3 4 5))

(take 5 (range))



