(def a {:a 1 :b 2 :c 3 :d 4 :e 5 :f 6 :g 7 :h 8 :i 9 :j 10 :k 11 :l 12 :m 13 :n 14 :o 15 :p 16 :q 17 :r 18 :s 19 :t 20 :u 21 :v 22 :w 23 :x 24 :y 25 :z 26})
(a :z)
({:a 1, :b 2} :a) ; = 1
({:a 1, :b 2} :qwerty) ; = nil
({:a 1, :b 2} :qwerty :uiop) ; = :uiop
(hash-map :key1 1, :key1 2)
(assoc a :a 3)
(get a :a)

(defn arity-test [n] (+ n n))
(println (arity-test 1))
(println (arity-test 1 1))

;; Tests for pr, prn, pr-str, and prn-str functions

;; Testing basic string printing differences between print and pr
(println "--- Testing print vs pr with strings ---")
(print "Hello") (print " ") (println "World") ; prints without quotes
(pr "Hello") (pr " ") (prn "World")           ; prints with quotes

;; Testing pr with different data types
(println "--- Testing pr with different data types ---")
(pr "a string" 42 true false nil [1 2 3] {:a 1 :b 2} #{1 2 3}) (println)

;; Testing pr-str and prn-str return values
(println "--- Testing pr-str and prn-str ---")
(def str-result (pr-str "test" 123 [4 5 6]))
(println "pr-str result:") (println str-result)
(println "Type of result:" (type str-result))

;; Testing prn-str (includes newline)
(def strn-result (prn-str {:x 10 :y 20} [1 2 3]))
(println "prn-str result:") (print strn-result)
(println "Type of result:" (type strn-result))

;; Practical example - creating readable representation of data
(println "--- Practical example ---")
(def data {:name "Alice" :age 30 :skills ["Clojure" "Java"]})
(def data-str (pr-str data))
(println "Data as string:" data-str)
(println "Data can be read back:" (= data (read-string data-str)))

(defn my-function [a b & rest]
  (println "Fixed args:" a b)
  (println "Rest args:" rest))

;; Can be called with 2 or more arguments:
(my-function 1 2)
(my-function 1 2 3 4 5)
