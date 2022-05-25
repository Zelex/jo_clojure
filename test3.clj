
(spit "tmp.clj" "(defn doub2 [x] (* x 2))")
(let [p (io/open-file "r" "tmp.clj")]
  (load-reader p)
  (is (= (doub2 15) 30))
  (io/close-file p))


