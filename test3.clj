;; Minimal struct test

;; Define a struct
(println "Defining struct...")
(defstruct person :name :age :city)

;; Try to create a struct instance
(println "Creating instance...")
(def p1 (struct person "John" 30 "New York"))

;; Print the result
(println "p1:" p1)
(println "Done.") 