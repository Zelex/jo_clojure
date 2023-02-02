(def img (atom nil))
(def WIDTH (atom 640))
(def HEIGHT (atom 480))

(def sokol-desc {
    :width @WIDTH
    :height @HEIGHT
    :window_title "Basic App"
    :init_cb (fn [] 
        (reset! img (sg/file-image "kodim01.png"))
    )
    :frame_cb (fn [] 
        (sgl/viewport 0 0 @WIDTH @HEIGHT false)
        (sgl/defaults)

        (sgl/enable-texture)
        (sgl/texture @img)

        (sgl/triangles
            (sgl/v2f-t2f-c3b  0.0,0.5  0.5,1,  255,0,0)
            (sgl/v2f-t2f-c3b -0.5,-0.5 0,0     0,255,0)
            (sgl/v2f-t2f-c3b  0.5,-0.5 1,0     0,0,255)
        )
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