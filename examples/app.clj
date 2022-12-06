(def sokol-desc {
    :width 640
    :height 480
    :window_title "Basic App"
    :init_cb (fn [] 
    )
    :frame_cb (fn [] 
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
