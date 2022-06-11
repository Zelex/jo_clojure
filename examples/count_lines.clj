(println "Number of lines of code: " (count (split-lines (str
    (slurp 'jo_clojure.cpp) 
    (slurp 'jo_clojure_async.h)
    (slurp 'jo_clojure_io.h)
    (slurp 'jo_clojure_lazy.h) 
    (slurp 'jo_clojure_math.h) 
    (slurp 'jo_clojure_string.h) 
    (slurp 'jo_clojure_system.h)
    ))))
