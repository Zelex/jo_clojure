(def sokol-desc {
    :width 640
    :height 480
    :window_title "Basic App"
    :init_cb (fn [] 
    )
    :frame_cb (fn [] 
        (sgl/viewport 0 0 640 480)
        (sgl/defaults)
        (sgl/begin-triangles)
        (sgl/v2f-c3b 0.0 0.5 255 0 0)
        (sgl/v2f-c3b -0.5 -0.5 0 255 0)
        (sgl/v2f-c3b 0.5 -0.5 0 0 255)
        (sgl/end)
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
