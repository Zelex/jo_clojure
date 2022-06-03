
(defn myfunc[a] (println "doing some work") (+ a 10))
(def myfunc-memo (memoize myfunc))

(myfunc-memo 1)
(myfunc-memo 1)
(myfunc-memo 20)
(myfunc-memo 20)