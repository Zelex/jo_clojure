
(def dim 128) ; dimension of the world
(def num-balls 256) ; to bounce around
(def num-frames 128) ; to simulate
(def collision-buckets (* num-balls 4)) ; More reduces data dependencies, at the cost of more memory

; Make the collision hash
(def collision-hash (atom []))

(defn get-pos-hash [x y] (mod (hash (Math/quantize x 1) (Math/quantize y 1)) collision-buckets))

(defn add-collision-hash [entity] 
    (let [phys (@entity :physics)
          [x y] (@phys :position)]
    (swap! (@collision-hash (get-pos-hash x y)) conj entity)
    ))

; Function to update the collision hash for old/new positions
(defn update-collision-hash [entity x y nx ny] 
    (let [old-h (get-pos-hash x y)
          new-h (get-pos-hash nx ny)]
         (when (not= old-h new-h)
            ; Remove entity from prev location
            (swap! (@collision-hash old-h) disj entity)
            ; Add entity to new location
            (swap! (@collision-hash new-h) conj entity))))

; Function to check for collisions with other entities
(defn check-collision [entity x y]
    (take 1 (for [other @(@collision-hash (get-pos-hash x y))
            :when (not= entity other)
            :let [phys   (@entity :physics)
                  r      (@phys :radius)
                  o-phys (@other :physics)
                  [o-x o-y] (@o-phys :position)
                  o-r    (@o-phys :radius)
                  dx     (- x o-x)
                  dy     (- y o-y)
                  d      (Math/sqrt (+ (* dx dx) (* dy dy)))
                 ]
            :when (< d (+ o-r r))
            ]
            {:other other :norm [dx dy]})))

; Reflect the walls of the box
(defn physics-reflect-walls [x vx] (if (or (< x 0) (>= x dim)) (- vx) vx))

; Run physics, return new state
(defn tick-physics 
    [{:position [x y] :velocity [vx vy] :acceleration [ax ay] :mass m :radius r} entity timestep]
    (let [nx (*+ timestep vx x)
          ny (*+ timestep vy y)
          nvx1 (physics-reflect-walls nx (*+ timestep ax vx))
          nvy1 (physics-reflect-walls ny (*+ timestep ay vy))
          bumpy (check-collision entity nx ny)
          bump-norm (Math/normalize ((first bumpy) :norm))
          [nvx nvy] (if (not-empty? bumpy) 
            (Math/reflect [nvx1 nvy1] bump-norm) 
            [nvx1 nvy1])
          nx (*+ timestep nvx x)
          ny (*+ timestep nvy y)]
          (update-collision-hash entity x y nx ny)
          ; Add acceleration to bumpy object
          (when (not-empty? bumpy) 
            ((@entity impulse) entity (Math/refract [nvx1 nvy1] bump-norm 1)))
          {:position [nx ny] :velocity [nvx nvy] :acceleration [0 0] :mass m :radius r}))

(defn impulse-physics 
    [{:position p :velocity v :acceleration [ax ay] :mass m :radius r} ix iy]
     {:position p :velocity v :acceleration [(+ ix ax) (+ iy ay)] :mass m :radius r})

; Construct a new entity 
(defn new-entity [type]
    {:type type
     :physics (atom {
        :position [(rand dim) (rand dim)]
        :velocity (Math/normalize [(rand -1 1) (rand -1 1)])
        :acceleration [0 0]
        :mass 1
        :radius 0.5})
     :color 0xFFFFFFFF
     :life 1
     :dead? false
     :tick (fn [entity timestep] 
        (swap! (@entity :physics) tick-physics entity timestep))
     :impulse (fn [entity [x y]] 
        (swap! (@entity :physics) impulse-physics x y))})

; Create an output gif file
(let [gif-file (gif/open "physics.gif" dim dim 0 4)]
    (print "Making gif...")
    ; Simulate N frames
    (let [entities (atom [])]
        (Math/srand 0)
        (reset! collision-hash [])
        (doseq [_ (range collision-buckets)] 
            (swap! collision-hash conj (atom #{})))
        (doseq [_ (range num-balls)]
            (let [entity (atom (new-entity :ball))]
                (swap! entities conj entity)
                (add-collision-hash entity)))
        (doseq [frame (range num-frames)]
            (print ".")
            (let [canvas (atom (jo/matrix dim dim))]
                (->> @entities
                    (pmap (fn [entity] 
                        (dosync ((@entity :tick) entity 1))
                        (swap! canvas assoc (@(@entity :physics) :position) (@entity :color))))
                    (doall)
                    (map deref)
                    (dorun))
                ; Write frame to gif-file
                (gif/frame gif-file @canvas 0 false))))
    ; Write gif footer to gif-file
    (gif/close gif-file)
    (println "Done!"))

(println "Simulate...")
(->> 
    (for [num-cores (range 1 (+ 1 *hardware-concurrency*))] (do
        (Thread/workers num-cores)
        (Thread/atom-retries-reset)
        (Thread/stm-retries-reset)
        (println "Testing with" num-cores "cores")
        (let [entities (atom [])]
            (Math/srand 0)
            (reset! collision-hash [])
            (doseq [_ (range collision-buckets)] 
                (swap! collision-hash conj (atom #{})))
            (doseq [_ (range num-balls)]
                (let [entity (atom (new-entity :ball))]
                    (swap! entities conj entity)
                    (add-collision-hash entity)))
            [
                num-cores
                (time (doseq [_ (range num-frames)]
                        (->> @entities
                            (pmap (fn [entity] (dosync ((@entity :tick) entity 1))))
                            (doall)
                            (map deref)
                            (dorun))
                        ))
                (Thread/atom-retries)
                (Thread/stm-retries)
            ])))
    (map (fn [x] (str (apply str (interpose ", " x)) "\n")))
    (reduce str)
    (str "num-cores, time, atom-retries, stm-retries\n")
    (spit "physics.csv"))


    

