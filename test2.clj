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

(defrecord Person [name age])
(def john (Person. "John" 30))
(def mary (map->Person {:name "Mary" :age 25}))

; Access fields
(println (:name john)) ; "John"
(println (:age mary))  ; 25

; Check type
(println (Person? john))         ; true
(println (Person? {}))           ; false
(println (record? john))         ; true

(defrecord Point [x y])
(def p1 (Point. 10 20))
(def p2 (map->Point {:x 10 :y 20}))

(println (:x p1)) ; 10
(println (:y p2)) ; 20

; test . syntax
(println "\n--- Record Tests ---")

; Basic Record Tests
(println "Record creation:" p1)
(println "Map->Record creation:" p2)

; Test equality
(println "Equal when same values, different constructor?:" (= p1 p2)) ; should be true
(println "Not equal with different values:" (= p1 (Point. 20 10)))    ; should be false

; Convert to map
(println "Record to map:" (into {} p1))

; Testing assoc with records
(def p3 (assoc p1 :x 100))
(println "After assoc:" p3)
(println "Original unchanged:" p1)
(println "Still a record after assoc?:" (record? p3))

; Add a key not in original record
(def p4 (assoc p1 :z 30))
(println "Added field outside definition:" p4)
(println "Is still a Point record?:" (Point? p4))

; Using update
(def p5 (update p1 :x + 5))
(println "After update:" p5)
(println "Is still a record after update?:" (record? p5))

; Using dissoc
(def p6 (dissoc p1 :x))
(println "After dissoc:" p6)
(println "Is still a record after dissoc?:" (record? p6))

(println "\n--- More Record Tests ---")

; Testing merge behavior with records
(def p7 (merge p1 {:z 30 :x 50}))
(println "Merge with map:" p7)
(println "Is still a record after merge?:" (record? p7))
(println "Type after merge:" (type p7))

; Testing records as map keys
(def record-map {p1 "Point at 10,20"})
(println "Record as map key:" record-map)
(println "Look up by equal record:" (get record-map p2))

; Testing metadata on records
(def p8 (with-meta p1 {:source "test" :created "now"}))
(println "Metadata test:" (meta p8))
(println "Same content with different meta?:" (= p1 p8))

; Testing nested records
(defrecord Rectangle [top-left bottom-right])
(def rect1 (Rectangle. p1 (Point. 50 60)))
(println "Nested record:" rect1)
(println "Access nested record field:" (-> rect1 :top-left :x))

; Testing assoc-in with nested records
(def rect2 (assoc-in rect1 [:top-left :x] 15))
(println "After assoc-in:" rect2)
(println "Original rect unchanged:" rect1)

; Testing records with functions
(defrecord MathPoint [x y]
  Object
  (toString [this] (str "MathPoint(" x "," y ")")))
  
(def mp1 (MathPoint. 5 10))
(println "Custom toString:" mp1)

; Testing select-keys with records
(println "select-keys result:" (select-keys p1 [:x]))
(println "Is result still a record?:" (record? (select-keys p1 [:x])))

; Testing record/map equivalence
(def point-map {:x 10 :y 20})
(println "Record equals equivalent map?:" (= (into {} p1) point-map))
(println "Map lookup vs record lookup:" (= (:x p1) (:x point-map)))

; Testing conj with records
(println "conj result:" (conj p1 [:z 30]))
(println "Is result still a record?:" (record? (conj p1 [:z 30])))

(println "\n--- Metadata Functions ---")
; Simple metadata implementation
(defn with-meta [obj metadata]
  (if (record? obj)
    (assoc obj :meta metadata)
    obj))

(defn meta [obj]
  (if (record? obj)
    (:meta obj)
    nil))

(println "\n--- Additional Record Tests ---")

; Test record equality semantics
(def p10 (Point. 10 20))
(def p11 (Point. 10 20))
(println "Record equality (same values):" (= p10 p11))

; Test maps vs records differences
(def point-map {:x 10 :y 20})
(println "Record to map conversion preserves values:" (= (into {} p1) point-map))
(println "Map to record conversion:" (map->Point point-map))
(println "Record/map comparison:" (= p1 point-map))

; Test record with field accessors
(println "Field access via keyword:" (:x p1))
(println "Field access via property:" (.-x p1))
(println "Count fields in record:" (count p1))

; Testing record constructors with defaults
(defrecord User [username email & {:keys [admin active] :or {admin false active true}}])
(def user1 (->User "johndoe" "john@example.com"))
(def user2 (->User "janedoe" "jane@example.com" :admin true))
(println "Record with defaults:" user1)
(println "Record with overridden defaults:" user2)

; Testing record inheritance behavior
(def p12 (assoc p1 :color "red"))
(println "Record with extra field:" p12)
(println "Still a Point record?:" (Point? p12))
(println "Can access new field:" (:color p12))

; Testing records in sets
(def point-set #{p1 p2})
(println "Records in set:" point-set)
(println "Set contains equal record?:" (contains? point-set p10))

; Testing destructuring with records
(let [{:keys [x y]} p1]
  (println "Destructured fields:" x y))

; Test metadata with our implementation
(def p13 (with-meta p1 {:source "test2" :created "today"}))
(println "Metadata access:" (meta p13))
(println "Original fields still accessible:" (:x p13) (:y p13))
