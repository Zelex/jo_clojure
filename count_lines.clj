(println "Number of lines of code: " (count (split-lines (str
    (slurp 'jo_lisp.cpp) 
    (slurp 'jo_lisp_async.h)
    (slurp 'jo_lisp_io.h)
    (slurp 'jo_lisp_lazy.h) 
    (slurp 'jo_lisp_math.h) 
    (slurp 'jo_lisp_string.h) 
    (slurp 'jo_lisp_system.h)
    ))))
