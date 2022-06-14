
(def dim 128) ; dimension of the world
(def num-balls 128) ; to bounce around
(def num-frames 128) ; to simulate

; Make the collision hash
(def collision-hash (atom []))
(def collision-buckets 16)
(doseq [_ (range collision-buckets)] (swap! collision-hash conj (atom #{})))

(defn get-pos-hash [x y] (mod (hash (str (Math/quantize x 8) (Math/quantize y 8))) collision-buckets))
;(defn get-pos-hash [x y] (hash (str (Math/quantize x 8) (Math/quantize y 8))))

(defn add-collision-hash [entity] 
    (let [phys (@entity :physics)
          [x y] (@phys :position)]
    (swap! (@collision-hash (get-pos-hash x y)) conj entity)))

; Function to update the collision hash for old/new positions
(defn update-collision-hash [entity x y nx ny] 
    (let [old-h (get-pos-hash x y)
          new-h (get-pos-hash nx ny)]
         ;(println "old-h: " old-h "x:" x "y:" y)
         ;(println "new-h: " new-h "nx:" nx "ny:" ny)
         (when (not= old-h new-h)
            ; Remove entity from prev location
            (swap! (@collision-hash old-h) disj entity)
            ; Add entity to new location
            (swap! (@collision-hash new-h) conj entity))))

; Function to check for collisions with other entities
(defn check-collision [entity x y]
    (not-empty? (take 1 (drop-while false? (for [other @(@collision-hash (get-pos-hash x y))]
        (if (= entity other) false
            (let [phys   (@entity :physics)
                  r      (@phys :radius)
                  o-phys (@other :physics)
                  [o-x o-y] (@o-phys :position)
                  o-r    (@o-phys :radius)
                  dx     (Math/abs (- o-x x))
                  dy     (Math/abs (- o-y y))
                  d      (Math/sqrt (+ (* dx dx) (* dy dy)))
                 ]
                (< d (+ o-r r)))))))))

; List of entities
(def entities (atom []))

; Reflect the walls of the box, or if we bumped into something else
(defn physics-reflect [x vx bump?] (if (or (< x 0) (>= x dim) bump?) (- vx) vx))

; Run physics, return new state
(defn tick-physics [{:position [x y] :velocity [vx vy] :acceleration [ax ay] :mass m :radius r} entity timestep]
    (let [nx (*+ timestep vx x)
          ny (*+ timestep vy y)
          bump? (check-collision entity nx ny)
          nvx (physics-reflect nx (*+ timestep ax vx) bump?)
          nvy (physics-reflect ny (*+ timestep ay vy) bump?)
          nx (*+ timestep nvx x)
          ny (*+ timestep nvy y)
          ]
          (update-collision-hash entity x y nx ny)
          {:position [nx ny] :velocity [nvx nvy] :acceleration [0 0] :mass m :radius r}))

; Construct a new entity 
(defn new-entity [type]
    {:type type
     :physics (atom {
        :position [(rand dim) (rand dim)]
        :velocity [(rand -1 1) (rand -1 1)]
        :acceleration [0 0]
        :mass 1
        :radius 0.5
     })
     :color 0xFFFFFFFF
     :life 1
     :dead? false
     :tick (fn [entity timestep]
             (swap! (@entity :physics) tick-physics entity timestep)
             )
    })

; Make a bunch of balls
(println "Make balls...")
(doseq [_ (range num-balls)]
    (let [entity (atom (new-entity :ball))]
        (swap! entities conj entity)
        (add-collision-hash entity)))

; Create an output gif file
(println "Render gif...")
(let [gif-file (gif/open "physics.gif" dim dim 0 8)]
    ; Simulate N frames
    (doseq [frame (range num-frames)]
        (println "Frame: " frame)
        (let [canvas (atom (vector2d dim dim))]
            (doseq [entity @entities]
                ; run the tick function
                ((@entity :tick) entity 1)
                (let [physics (@entity :physics)]
                    (swap! canvas assoc (@physics :position) (@entity :color))))
            ; Write frame to gif-file
            (gif/frame gif-file @canvas 0 false)))
    ; Write gif footer to gif-file
    (gif/close gif-file))



    

