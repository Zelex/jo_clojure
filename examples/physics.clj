
; List of entities
(def entities (atom []))

; Reflex the walls of the box
(defn physics-reflect-walls [x vx] (if (or (< x 0) (> x 128)) -vx vx))

; Run physics, return new state
(defn tick-physics [{:position [x y] :velocity [vx vy] :acceleration [ax ay] :mass m :radius r} timestep]
    (let [nx (*+ timestep vx x)
          ny (*+ timestep vy y)
          nvx (physics-reflect-walls nx (*+ timestep ax vx))
          nvy (physics-reflect-walls ny (*+ timestep ay vy))]
          {:position [nx ny] :velocity [nvx nvy] :acceleration [0 0] :mass m :radius r}))

; Construct a new entity 
(defn new-entity [type]
    {:type type
     :physics (atom {
        :position [(rand 0 128) (rand 0 128)]
        :velocity [(rand -1 1) (rand -1 1)]
        :acceleration [0 0]
        :mass 1
        :radius 0.5
     })
     :color [0.5 0.5 0.5]
     :life 1
     :dead? false
     :tick (fn [entity timestep]
             (swap! (get @entity :physics) tick-physics timestep))
    })

; Make a bunch of balls
(doseq [i (range 128)]
    (swap! entities conj (atom (new-entity :ball))))

; Create an output gif file

; Simulate N frames
(doseq [frame (range 128)]
    (let [canvas (vector2d 128 128)]
        (doseq [entity @entities]
            ; run the tick function
            ((@entity :tick) 0.01)
            ; render dot to canvas at entity location
            )
        ; output canvas as frame to gif
    ))

; Close output gif file



    

