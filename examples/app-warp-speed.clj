(def stars (atom []))

(defn add-star! []
    (swap! stars conj [(rand -1 1) (rand -1 1) (rand 0.01 1.5)]))

(defn draw-stars []
    (sgl/begin-points)
    (doseq [[x y z] @stars]
        (sgl/v2f-c3b (/ x z) (/ y z) 255 255 255))
    (sgl/end))

(defn move-stars [] 
    (swap! stars (fn [stars]
        (doall-vec (map 
            (fn [[x y z]]
                (let [z (- z 0.01)]
                 (if (<= z 0.01)
                     [(rand -1 1) (rand -1 1) 1.5]
                     [x y z])))
            stars)))))

(defn print-stars []
    (doseq [[x y z] @stars]
        (println "x:" x "y:" y "z:" z)))

; add 500 stars
(doseq [_ (range 500)] (add-star!))

(def sokol-desc {
    :width 640
    :height 480
    :window_title "Basic Warp Speed App"
    :init_cb (fn [] 
    )
    :frame_cb (fn [] 
        (sgl/viewport 0 0 640 480 false)
        (sgl/defaults)
        (draw-stars)
        (move-stars)
        ;(print-stars)
    )
    :cleanup_cb (fn [] 
    )
    :event_cb (fn [event] 
    )
    :fail_cb (fn [msg] 
        (println "Error/Sokol:" msg)
    )
})

(sokol/run sokol-desc)