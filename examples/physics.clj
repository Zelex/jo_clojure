
(def entities (atom {}))

(defn new-entity [type]
    {:type type
     :position [0 0]
     :velocity [0 0]
     :acceleration [0 0]
     :radius 0.5
     :mass 1
     :color [0.5 0.5 0.5]
     :life 1
     :dead? false
     :collided? false})

(defn new-ball [x y vx vy]
    (into (new-entity :ball)
    {:position [x y]
     :velocity [vx vy]}))

    

