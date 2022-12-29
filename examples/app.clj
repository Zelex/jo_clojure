(def WIDTH (atom 640))
(def HEIGHT (atom 480))

(def sokol-desc {
    :width @WIDTH
    :height @HEIGHT
    :window_title "Basic App"
    :init_cb (fn [] 
    )
    :frame_cb (fn [] 
        (sgl/viewport 0 0 @WIDTH @HEIGHT false)
        (sgl/defaults)
        (sgl/begin-triangles)
        (sgl/v2f-c3b  0.0  0.5 255 0 0)
        (sgl/v2f-c3b -0.5 -0.5 0 255 0)
        (sgl/v2f-c3b  0.5 -0.5 0 0 255)
        (sgl/end)
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
