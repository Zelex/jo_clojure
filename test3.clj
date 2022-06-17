(mapcat (fn [[k v]] (for [[k2 v2] v] (concat [k k2] v2))) {:b {:z (5 6), :x (1 2)}, :a {:y (3 4), :x (1 2)}})

