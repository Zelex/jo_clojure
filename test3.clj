
(def x 5)

  
(load-string "(defn doub3 [x] (* x 2))")
(is (= (doub3 15) 30))


