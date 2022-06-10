(defn neighbors [[x y]]
  (for [dx [-1 0 1] dy (if (zero? dx) [-1 1] [-1 0 1])] 
    [(+ dx x) (+ dy y)]))

(def board #{[1 1] [1 2] [1 0]})

(frequencies (mapcat neighbors board))



