
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
        :position [(rand 128) (rand 128)]
        :velocity [(rand -1 1) (rand -1 1)]
        :acceleration [0 0]
        :mass 1
        :radius 0.5
     })
     :color 0xFFFFFFFF
     :life 1
     :dead? false
     :tick (fn [entity timestep]
             (swap! (@entity :physics) tick-physics timestep)
             )
    })


; Make a bunch of balls
(println "Make balls...")
(doseq [i (range 128)]
    (swap! entities conj (atom (new-entity :ball))))

; Create an output gif file
(println "Render gif...")
(let [gif-file (gif/open "physics.gif" 128 128 0 8)]
    ; Simulate N frames
    (doseq [frame (range 4)]
        (println "Frame: " frame)
        (let [canvas (atom (vector2d 128 128))]
            (doseq [entity @entities]
                ; run the tick function
                ;((@entity :tick) entity 0.01)
                (let [physics (@entity :physics)]
                    ;(swap! canvas assoc (@physics :position) (@entity :color)))
                    (swap! canvas assoc (@physics :position) 0xFFFFFFFF)))
            ; Write frame to gif-file
            (gif/frame gif-file @canvas 0 true)))
    ; Write gif footer to gif-file
    (gif/close gif-file))



    

