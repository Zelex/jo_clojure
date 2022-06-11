
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
     :tick (fn [entity timestep]
             (swap! entity
                     {:position (*+ timestep (get @entity :velocity) (get @entity :position))
                      :velocity (*+ timestep (get @entity :acceleration) (get @entity :velocity))
                      :acceleration [0 0]
                      :dead? (= (get @entity :life) 0)
                      :collided? false})
             entity)})

(defn new-ball [x y vx vy]
    (into (new-entity :ball)
          {:position [x y]
           :velocity [vx vy]}))



    

