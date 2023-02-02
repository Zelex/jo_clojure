(def WIDTH (atom 640))
(def HEIGHT (atom 480))

(def stars (atom []))

(defn add-star! []
    (swap! stars conj [(rand -1 1) (rand -1 1) (rand 0.01 1.5)]))

(defn draw-stars []
    (sgl/points
        (doseq [[x y z] @stars]
            (sgl/v2f-c3b (/ x z) (/ y z) 255 255 255))))

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
    :width @WIDTH
    :height @HEIGHT
    :window_title "Basic Warp Speed App"
    :init_cb (fn [] 
    )
    :frame_cb (fn [] 
        (sgl/viewport 0 0 @WIDTH @HEIGHT false)
        (sgl/defaults)
        (draw-stars)
        (move-stars)
        ;(print-stars)
    )
    :cleanup_cb (fn [] 
    )
    :event_cb (fn [event] 
        (case (:type event)
            :key-down (case (:key event)
                :escape (sokol/quit)
            )
            :resized (do
                (reset! WIDTH (:window-width event))
                (reset! HEIGHT (:window-height event))
            )
        )
    )
    :fail_cb (fn [msg] 
        (println "Error/Sokol:" msg)
    )
})

(sokol/run sokol-desc)