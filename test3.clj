
(spit "tmp.clj" "(defn doub [x] (* x 2))")
(load-file "tmp.clj")
(is (= (doub 15) 30))
(doub 15)


