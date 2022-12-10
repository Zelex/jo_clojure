(def img (atom nil))

(def sokol-desc {
    :width 640
    :height 480
    :window_title "Basic App"
    :init_cb (fn [] 
        (reset! img (sg/file-image "kodim01.png"))
        (println @img)
    )
    :frame_cb (fn [] 
        (sgl/viewport 0 0 640 480 false)
        (sgl/defaults)

        (sgl/enable-texture)
        (sgl/texture @img)

        (sgl/begin-triangles)
        (sgl/v2f-t2f-c3b  0.0,0.5  0.5,1,  255,0,0)
        (sgl/v2f-t2f-c3b -0.5,-0.5 0,0     0,255,0)
        (sgl/v2f-t2f-c3b  0.5,-0.5 1,0     0,0,255)
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